/**
 * @brief Requirement data model & traceability registry (Phase 1)
 */
export module speclab.core.requirements;

import std;
import speclab.core.testresult;

export namespace speclab::core {

    struct Requirement {
        std::string id;
        std::string description;
        std::string riskLevel{"LOW"};          // ISO 14971 risk level
        std::string safetyClass{"CLASS_A"};     // IEC 62304 safety class
        bool requiresAudit{false};
        bool requiresValidation{false};
        std::string source;                      // e.g. MRD, SRS reference
        std::string version{"1.0"};
        std::unordered_map<std::string,std::string> metadata; // extension point
    };

    class RequirementRegistry {
    public:
        static RequirementRegistry& instance() {
            static RequirementRegistry reg; return reg;
        }

        bool registerRequirement(Requirement req) {
            auto [it, inserted] = requirements_.emplace(req.id, std::move(req));
            return inserted;
        }

        bool hasRequirement(std::string_view id) const {
            return requirements_.contains(std::string(id));
        }

        const Requirement* getRequirement(std::string_view id) const {
            auto it = requirements_.find(std::string(id));
            return it==requirements_.end()? nullptr : &it->second;
        }

        void linkTestToRequirement(std::string_view testId, std::string_view requirementId) {
            testToReq_[std::string(testId)].insert(std::string(requirementId));
            reqToTest_[std::string(requirementId)].insert(std::string(testId));
        }

        // Produce a simple CSV trace matrix (Requirement -> Tests)
        std::string exportTraceMatrixCSV() const {
            std::string out = "RequirementID,RiskLevel,SafetyClass,RequiresValidation,RequiresAudit,TestIDs\n";
            for (const auto& [id, req] : requirements_) {
                auto it = reqToTest_.find(id);
                std::string tests;
                if (it != reqToTest_.end()) {
                    bool first=true; for (auto& t : it->second){ if(!first) tests+=","; tests+=t; first=false; }
                }
                out += std::format("{},{},{},{},{},{}\n", id, req.riskLevel, req.safetyClass,
                                   req.requiresValidation?"true":"false", req.requiresAudit?"true":"false", tests);
            }
            return out;
        }

        // Generate mapping rows for programmatic use
        struct TraceRow { std::string requirementId; std::vector<std::string> testIds; };
        std::vector<TraceRow> getTraceMatrix() const {
            std::vector<TraceRow> rows;
            rows.reserve(requirements_.size());
            for (auto& [id, _] : requirements_) {
                TraceRow row; row.requirementId = id;
                if (auto it = reqToTest_.find(id); it != reqToTest_.end()) {
                    row.testIds.assign(it->second.begin(), it->second.end());
                }
                rows.push_back(std::move(row));
            }
            return rows;
        }

    private:
        std::unordered_map<std::string, Requirement> requirements_;
        std::unordered_map<std::string, std::set<std::string>> testToReq_;
        std::unordered_map<std::string, std::set<std::string>> reqToTest_;
    };

    // Convenience API wrappers (exported)
    inline bool RegisterRequirement(const Requirement& req) {
        return RequirementRegistry::instance().registerRequirement(req);
    }

    inline void LinkTestRequirement(std::string_view testId, std::string_view requirementId) {
        RequirementRegistry::instance().linkTestToRequirement(testId, requirementId);
    }

    inline std::string ExportTraceMatrixCSV() {
        return RequirementRegistry::instance().exportTraceMatrixCSV();
    }

} // namespace speclab::core
