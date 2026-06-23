// scheduler.hpp
//
// A small, header-only system scheduler for a game engine.
//
// Concepts:
//   - Phase   : a named, ordered stage of the frame (e.g. "Input", "Physics",
//               "Gameplay", "Render"). Phases always run in the order they
//               were defined.
//   - System  : a unit of work (a function) registered into a phase. Systems
//               can declare dependencies on other systems *within the same
//               phase* using After()/Before(), and can be tagged with extra
//               Label()s so other systems can depend on a whole group at
//               once (similar to Bevy's system sets).
//   - Build() : resolves all dependency references into a DAG per phase,
//               topologically sorts it (Kahn's algorithm), and detects
//               cycles. It also computes a "wave" index per system, i.e. how
//               many dependency hops deep it is, which is used to run
//               independent systems concurrently.
//   - Run()   : runs every phase, in order, serially.
//   - RunParallel() : like Run(), but systems within a phase that share a
//               wave (no ordering relationship between them) execute on
//               separate threads.
//
// Example:
//
//   ecs::Scheduler sched;
//   auto input   = sched.DefinePhase("Input");
//   auto physics = sched.DefinePhase("Physics");
//
//   sched.AddSystem(input, "ReadInput", [](float dt){ ... });
//   sched.AddSystem(physics, "Integrate", [](float dt){ ... })
//        .After("ReadInput");
//   sched.AddSystem(physics, "ResolveCollisions", [](float dt){ ... })
//        .After("Integrate");
//
//   sched.Build();
//   sched.Run(dt);
//
#pragma once

#include <algorithm>
#include <functional>
#include <future>
#include <initializer_list>
#include <sstream>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace ecs {

using SystemId = std::size_t;
using PhaseId = std::size_t;

class Scheduler;

// ---------------------------------------------------------------------------
// SystemHandle: fluent builder returned by Scheduler::AddSystem so callers
// can chain dependency declarations:
//
//   scheduler.AddSystem(phase, "Foo", fn).After("Bar").Label("Movement");
// ---------------------------------------------------------------------------
class SystemHandle {
public:
    SystemHandle(Scheduler& scheduler, SystemId id) : scheduler_(scheduler), id_(id) {}

    // This system must run after the named system or label.
    SystemHandle& After(const std::string& name);
    SystemHandle& After(std::initializer_list<std::string> names);

    // This system must run before the named system or label.
    SystemHandle& Before(const std::string& name);
    SystemHandle& Before(std::initializer_list<std::string> names);

    // Tags this system with an extra label. Other systems may depend on the
    // label to mean "after/before every system carrying this label", which
    // is handy for depending on a whole group without naming each member.
    SystemHandle& Label(const std::string& label);

    // Optional per-frame predicate. If it returns false the system is
    // skipped that frame (its dependents still run on schedule).
    SystemHandle& RunIf(std::function<bool()> predicate);

    SystemId Id() const { return id_; }

private:
    Scheduler& scheduler_;
    SystemId id_;
};

// ---------------------------------------------------------------------------
// Internal bookkeeping
// ---------------------------------------------------------------------------
namespace detail {

struct SystemNode {
    SystemId id{};
    PhaseId phase{};
    std::string name;
    std::function<void(float)> func;
    std::function<bool()> runIf;

    std::unordered_set<std::string> labels; // extra labels besides `name`
    std::vector<std::string> afterRefs;     // names/labels this must follow
    std::vector<std::string> beforeRefs;    // names/labels this must precede

    int wave = -1; // computed by Build(); used for parallel grouping
};

struct PhaseNode {
    PhaseId id{};
    std::string name;
    std::vector<SystemId> systemsInOrder; // filled in by Build()
};

} // namespace detail

// ---------------------------------------------------------------------------
// Scheduler
// ---------------------------------------------------------------------------
class Scheduler {
public:
    // Defines a new phase. Phases execute in the order they are defined.
    PhaseId DefinePhase(const std::string& name) {
        for (auto& p : phases_) {
            if (p.name == name)
                throw std::runtime_error("Scheduler: phase '" + name + "' already exists");
        }
        PhaseId id = phases_.size();
        phases_.push_back(detail::PhaseNode{id, name, {}});
        return id;
    }

    // Convenience for defining several phases at once, in order.
    void DefinePhases(std::initializer_list<std::string> names) {
        for (auto& n : names) DefinePhase(n);
    }

    PhaseId GetPhase(const std::string& name) const {
        for (auto& p : phases_)
            if (p.name == name) return p.id;
        throw std::runtime_error("Scheduler: unknown phase '" + name + "'");
    }

    std::size_t PhaseCount() const { return phases_.size(); }
    const std::string& PhaseName(PhaseId id) const { return phases_.at(id).name; }

    // Registers a system within a phase. `name` must be unique within that
    // phase. Returns a builder for chaining After/Before/Label/RunIf.
    SystemHandle AddSystem(PhaseId phase, const std::string& name, std::function<void(float)> func) {
        if (phase >= phases_.size())
            throw std::runtime_error("Scheduler: invalid phase id for system '" + name + "'");

        for (auto& kv : nodes_) {
            if (kv.second.phase == phase && kv.second.name == name)
                throw std::runtime_error("Scheduler: duplicate system name '" + name +
                                          "' in phase '" + phases_[phase].name + "'");
        }

        SystemId id = nextId_++;
        detail::SystemNode node;
        node.id = id;
        node.phase = phase;
        node.name = name;
        node.func = std::move(func);
        nodes_.emplace(id, std::move(node));
        phaseMembers_[phase].push_back(id);
        built_ = false;
        return SystemHandle(*this, id);
    }

    // Overload taking the phase by name instead of id.
    SystemHandle AddSystem(const std::string& phaseName, const std::string& name, std::function<void(float)> func) {
        return AddSystem(GetPhase(phaseName), name, std::move(func));
    }

    // --- called internally by SystemHandle; not usually called directly ---
    void AddAfter(SystemId id, const std::string& name) { nodes_.at(id).afterRefs.push_back(name); built_ = false; }
    void AddBefore(SystemId id, const std::string& name) { nodes_.at(id).beforeRefs.push_back(name); built_ = false; }
    void AddLabel(SystemId id, const std::string& label) { nodes_.at(id).labels.insert(label); built_ = false; }
    void SetRunIf(SystemId id, std::function<bool()> pred) { nodes_.at(id).runIf = std::move(pred); }

    // Resolves all dependencies, topologically sorts every phase, and
    // computes parallel "waves" (for RunParallel()). Throws
    // std::runtime_error if a reference is unresolved or a cycle exists.
    void Build() {
        for (auto& phase : phases_) SortPhase(phase);
        built_ = true;
    }

    // Runs every phase, in definition order, on the calling thread.
    void Run(float dt) {
        if (!built_) Build();
        for (auto& phase : phases_)
            for (SystemId id : phase.systemsInOrder) RunOne(id, dt);
    }

    // Like Run(), but within each phase, systems in the same dependency
    // "wave" (no ordering relationship between them) execute concurrently.
    void RunParallel(float dt) {
        if (!built_) Build();
        for (auto& phase : phases_) RunPhaseParallel(phase, dt);
    }

    // Debug helper: human-readable execution order and wave numbers, useful
    // for logging at startup to sanity-check the schedule.
    std::string DescribeSchedule() const {
        std::ostringstream out;
        for (auto& phase : phases_) {
            out << "Phase: " << phase.name << "\n";
            for (SystemId id : phase.systemsInOrder) {
                auto& n = nodes_.at(id);
                out << "  [wave " << n.wave << "] " << n.name;
                if (!n.labels.empty()) {
                    out << "  (labels:";
                    for (auto& l : n.labels) out << " " << l;
                    out << ")";
                }
                out << "\n";
            }
        }
        return out.str();
    }

    const std::vector<SystemId>& GetOrder(PhaseId phase) const {
        return phases_.at(phase).systemsInOrder;
    }

private:
    void RunOne(SystemId id, float dt) {
        auto& node = nodes_.at(id);
        if (node.runIf && !node.runIf()) return;
        node.func(dt);
    }

    void RunPhaseParallel(detail::PhaseNode& phase, float dt) {
        std::vector<std::vector<SystemId>> waves;
        for (SystemId id : phase.systemsInOrder) {
            int w = nodes_.at(id).wave;
            if (static_cast<std::size_t>(w) >= waves.size()) waves.resize(w + 1);
            waves[w].push_back(id);
        }
        for (auto& wave : waves) {
            if (wave.empty()) continue;
            if (wave.size() == 1) {
                RunOne(wave[0], dt);
                continue;
            }
            std::vector<std::future<void>> futures;
            futures.reserve(wave.size());
            for (SystemId id : wave)
                futures.push_back(std::async(std::launch::async, [this, id, dt] { RunOne(id, dt); }));
            for (auto& f : futures) f.get();
        }
    }

    // Finds every system in `phase` whose name or labels match `ref`.
    std::vector<SystemId> ResolveRef(PhaseId phase, const std::string& ref) const {
        std::vector<SystemId> result;
        auto it = phaseMembers_.find(phase);
        if (it == phaseMembers_.end()) return result;
        for (SystemId id : it->second) {
            auto& n = nodes_.at(id);
            if (n.name == ref || n.labels.count(ref)) result.push_back(id);
        }
        return result;
    }

    void SortPhase(detail::PhaseNode& phase) {
        auto memberIt = phaseMembers_.find(phase.id);
        if (memberIt == phaseMembers_.end()) {
            phase.systemsInOrder.clear();
            return;
        }
        auto& members = memberIt->second;

        // adjacency: edge u -> v means "u must run before v"
        std::unordered_map<SystemId, std::unordered_set<SystemId>> adj;
        std::unordered_map<SystemId, int> indegree;
        for (SystemId id : members) {
            adj[id];
            indegree[id] = 0;
        }

        auto addEdge = [&](SystemId u, SystemId v) {
            if (u == v) return;
            if (adj[u].insert(v).second) indegree[v]++;
        };

        for (SystemId id : members) {
            auto& n = nodes_.at(id);
            for (auto& ref : n.afterRefs) {
                auto matches = ResolveRef(phase.id, ref);
                if (matches.empty())
                    throw std::runtime_error("Scheduler: system '" + n.name +
                                              "' depends on unknown system/label '" + ref +
                                              "' in phase '" + phase.name + "'");
                for (SystemId dep : matches) addEdge(dep, id);
            }
            for (auto& ref : n.beforeRefs) {
                auto matches = ResolveRef(phase.id, ref);
                if (matches.empty())
                    throw std::runtime_error("Scheduler: system '" + n.name +
                                              "' must run before unknown system/label '" + ref +
                                              "' in phase '" + phase.name + "'");
                for (SystemId dependent : matches) addEdge(id, dependent);
            }
        }

        // Stable insertion order, used as a tie-breaker so the schedule is
        // deterministic when several systems are equally eligible to run.
        auto orderIndex = [&](SystemId id) {
            return std::find(members.begin(), members.end(), id) - members.begin();
        };

        std::vector<SystemId> ready;
        for (SystemId id : members)
            if (indegree[id] == 0) ready.push_back(id);
        std::sort(ready.begin(), ready.end(), [&](SystemId a, SystemId b) { return orderIndex(a) < orderIndex(b); });

        for (SystemId id : members) nodes_.at(id).wave = -1;

        std::unordered_map<SystemId, int> indegreeWork = indegree;
        std::vector<SystemId> sorted;
        sorted.reserve(members.size());

        while (!ready.empty()) {
            SystemId cur = ready.front();
            ready.erase(ready.begin());
            sorted.push_back(cur);

            int curWave = nodes_.at(cur).wave;
            if (curWave < 0) curWave = 0;
            nodes_.at(cur).wave = curWave;

            std::vector<SystemId> newlyReady;
            for (SystemId next : adj[cur]) {
                int& nextWave = nodes_.at(next).wave;
                nextWave = std::max(nextWave, curWave + 1);
                if (--indegreeWork[next] == 0) newlyReady.push_back(next);
            }
            std::sort(newlyReady.begin(), newlyReady.end(),
                      [&](SystemId a, SystemId b) { return orderIndex(a) < orderIndex(b); });
            ready.insert(ready.end(), newlyReady.begin(), newlyReady.end());
        }

        if (sorted.size() != members.size()) {
            throw std::runtime_error("Scheduler: dependency cycle detected in phase '" + phase.name + "'" +
                                      DescribeRemaining(indegreeWork));
        }

        phase.systemsInOrder = std::move(sorted);
    }

    std::string DescribeRemaining(const std::unordered_map<SystemId, int>& remainingIndegree) const {
        std::ostringstream out;
        out << " involving: ";
        bool first = true;
        for (auto& kv : remainingIndegree) {
            if (kv.second > 0) {
                if (!first) out << ", ";
                out << nodes_.at(kv.first).name;
                first = false;
            }
        }
        return out.str();
    }

    std::vector<detail::PhaseNode> phases_;
    std::unordered_map<SystemId, detail::SystemNode> nodes_;
    std::unordered_map<PhaseId, std::vector<SystemId>> phaseMembers_;
    SystemId nextId_ = 0;
    bool built_ = false;
};

// ---------------------------------------------------------------------------
// SystemHandle method bodies (defined after Scheduler is fully declared)
// ---------------------------------------------------------------------------
inline SystemHandle& SystemHandle::After(const std::string& name) {
    scheduler_.AddAfter(id_, name);
    return *this;
}
inline SystemHandle& SystemHandle::After(std::initializer_list<std::string> names) {
    for (auto& n : names) scheduler_.AddAfter(id_, n);
    return *this;
}
inline SystemHandle& SystemHandle::Before(const std::string& name) {
    scheduler_.AddBefore(id_, name);
    return *this;
}
inline SystemHandle& SystemHandle::Before(std::initializer_list<std::string> names) {
    for (auto& n : names) scheduler_.AddBefore(id_, n);
    return *this;
}
inline SystemHandle& SystemHandle::Label(const std::string& label) {
    scheduler_.AddLabel(id_, label);
    return *this;
}
inline SystemHandle& SystemHandle::RunIf(std::function<bool()> predicate) {
    scheduler_.SetRunIf(id_, std::move(predicate));
    return *this;
}

} // namespace ecs