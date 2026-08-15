#pragma once

#include <unordered_map>
#include <utility>
#include <vector>

namespace sr2ap {
    enum class BaselineUpdateKind {
        Unchanged,
        Created,
        Changed,
        IdentityChanged,
        Invalidated,
    };

    template <class Key, class Value>
    struct BaselineChange {
        Key key;
        Value previous;
        Value current;
    };

    template <class Key, class Value>
    struct BaselineUpdate {
        BaselineUpdateKind kind{BaselineUpdateKind::Unchanged};
        std::vector<BaselineChange<Key, Value>> changes;
    };

    template <class Key, class Value>
    class BaselineTracker {
       public:
        BaselineUpdate<Key, Value> Observe(std::unordered_map<Key, Value> current) {
            if (!valid_) {
                baseline_ = std::move(current);
                valid_ = true;
                return {BaselineUpdateKind::Created, {}};
            }
            if (current.size() != baseline_.size() || !HasSameKeys(current)) {
                baseline_ = std::move(current);
                return {BaselineUpdateKind::IdentityChanged, {}};
            }

            BaselineUpdate<Key, Value> update;
            for (const auto& [key, value] : current) {
                auto& previous = baseline_.at(key);
                if (previous == value) {
                    continue;
                }
                update.changes.push_back({key, previous, value});
                previous = value;
            }
            if (!update.changes.empty()) {
                update.kind = BaselineUpdateKind::Changed;
            }
            return update;
        }

        BaselineUpdate<Key, Value> Invalidate() {
            if (!valid_) {
                return {};
            }
            baseline_.clear();
            valid_ = false;
            return {BaselineUpdateKind::Invalidated, {}};
        }

        bool IsValid() const noexcept {
            return valid_;
        }

       private:
        bool HasSameKeys(const std::unordered_map<Key, Value>& current) const {
            for (const auto& entry : current) {
                if (baseline_.find(entry.first) == baseline_.end()) {
                    return false;
                }
            }
            return true;
        }

        bool valid_{};
        std::unordered_map<Key, Value> baseline_;
    };
}  // namespace sr2ap
