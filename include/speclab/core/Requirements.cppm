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

    struct RequirementsConfig {
        bool abortOnCriticalGaps = true;      // Phase 3: abort execution if uncovered CRITICAL requirements
        bool riskBasedOrdering = true;        // Enable risk-weighted test ordering
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

        const std::set<std::string>* getRequirementsForTest(std::string_view testId) const {
            auto it = testToReq_.find(std::string(testId));
            return it == testToReq_.end() ? nullptr : &it->second;
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

        std::vector<const Requirement*> getAllRequirements() const {
            std::vector<const Requirement*> out; out.reserve(requirements_.size());
            for (auto& [_, r] : requirements_) out.push_back(&r); return out;
        }
        std::vector<const Requirement*> getUncoveredHighRisk() const {
            std::vector<const Requirement*> missing;
            for (auto& [id, r] : requirements_) {
                if (r.riskLevel == "HIGH" || r.riskLevel == "CRITICAL") {
                    if (!reqToTest_.contains(id) || reqToTest_.at(id).empty()) missing.push_back(&r);
                }
            }
            return missing;
        }
        std::vector<const Requirement*> getUncoveredCritical() const {
            std::vector<const Requirement*> missing;
            for (auto& [id, r] : requirements_) {
                if (r.riskLevel == "CRITICAL") {
                    if (!reqToTest_.contains(id) || reqToTest_.at(id).empty()) missing.push_back(&r);
                }
            }
            return missing;
        }
        const RequirementsConfig& getConfig() const noexcept { return config_; }
        void setConfig(const RequirementsConfig& cfg) { config_ = cfg; }
        bool hasUncoveredCritical() const {
            for (auto& [id, r] : requirements_) {
                if ((r.riskLevel == "CRITICAL") && (!reqToTest_.contains(id) || reqToTest_.at(id).empty())) return true;
            }
            return false;
        }
        int riskScoreForTest(std::string_view testId) const {
            auto it = testToReq_.find(std::string(testId));
            if (it == testToReq_.end()) return 0; // no requirements linked
            int maxScore = 0;
            for (auto& rid : it->second) {
                if (auto rIt = requirements_.find(rid); rIt != requirements_.end()) {
                    const auto& rl = rIt->second.riskLevel;
                    int s = (rl == "CRITICAL") ? 4 : (rl == "HIGH" ? 3 : (rl == "MEDIUM" ? 2 : 1));
                    if (s > maxScore) maxScore = s;
                }
            }
            return maxScore;
        }
        std::string exportTraceMatrixHTML() const {
            std::string html;
            html += "<html><head><meta charset='utf-8'><title>SpecLab Traceability</title>";
            html += "<style>body{font-family:Arial,Helvetica,sans-serif}table{border-collapse:collapse;width:100%}th,td{border:1px solid #ccc;padding:4px;font-size:12px}th{background:#f0f0f0} .risk-HIGH{background:#ffe0cc} .risk-CRITICAL{background:#ffcccc;font-weight:bold}</style></head><body>";
            html += "<h2>Requirement Traceability Matrix</h2>";
            html += "<table><tr><th>ID</th><th>Description</th><th>Risk</th><th>Safety</th><th>Validation</th><th>Audit</th><th>Tests</th></tr>";
            for (const auto& [id, req] : requirements_) {
                auto it = reqToTest_.find(id);
                std::string tests;
                if (it != reqToTest_.end()) { bool first=true; for (auto& t : it->second){ if(!first) tests+=", "; tests+=t; first=false; } }
                html += std::format("<tr class='risk-{}'><td>{}</td><td>{}</td><td>{}</td><td>{}</td><td>{}</td><td>{}</td><td>{}</td></tr>",
                    req.riskLevel, id, req.description, req.riskLevel, req.safetyClass,
                    req.requiresValidation?"Y":"N", req.requiresAudit?"Y":"N", tests);
            }
            html += "</table>";
            // Summary
            size_t total=requirements_.size(), covered=0, high=0, highCovered=0;
            for (auto& [id,r]:requirements_) {
                bool cov = reqToTest_.contains(id) && !reqToTest_.at(id).empty();
                if (cov) ++covered;
                if (r.riskLevel=="HIGH"||r.riskLevel=="CRITICAL") { ++high; if (cov) ++highCovered; }
            }
            html += std::format("<p>Total requirements: {} | Covered: {} | Coverage: {:.1f}%</p>", total, covered, total? (100.0*covered/total):100.0);
            html += std::format("<p>High/Critical: {} | Covered: {} | Coverage: {:.1f}%</p>", high, highCovered, high? (100.0*highCovered/high):100.0);
            html += "</body></html>";
            return html;
        }
    private:
        std::unordered_map<std::string, Requirement> requirements_;
        std::unordered_map<std::string, std::set<std::string>> testToReq_;
        std::unordered_map<std::string, std::set<std::string>> reqToTest_;
        RequirementsConfig config_{}; // Phase 3 config
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

    inline void AugmentResultWithRequirements(TestResult& result) {
        if (result.testId.empty()) return;
        if (auto ids = RequirementRegistry::instance().getRequirementsForTest(result.testId); ids) {
            for (auto& id : *ids) result.addRequirementId(id);
        }
    }

    inline void AddCoverageValidationResult(TestResultCollection& collection) {
        auto missing = RequirementRegistry::instance().getUncoveredHighRisk();
        TestResult coverageResult(TestStatus::Passed, "All high-risk requirements covered");
        coverageResult.testId = "REQUIREMENT_COVERAGE";
        if (!missing.empty()) {
            bool anyCritical = std::any_of(missing.begin(), missing.end(), [](const Requirement* r){ return r->riskLevel == "CRITICAL"; });
            coverageResult.status = anyCritical ? TestStatus::Critical : TestStatus::Failed;
            std::string list;
            for (size_t i=0;i<missing.size();++i){ if(i) list += ","; list += missing[i]->id; }
            coverageResult.message = "Uncovered high-risk requirements";
            coverageResult.errorDetails = list;
            coverageResult.addMetadata("missing_requirements", list);
        }
        collection.addResult(std::move(coverageResult));
    }

    inline void SetRequirementsConfig(const RequirementsConfig& cfg) { RequirementRegistry::instance().setConfig(cfg); }
    inline std::string ExportTraceMatrixHTML() { return RequirementRegistry::instance().exportTraceMatrixHTML(); }
    inline int TestRiskScore(std::string_view testId) { return RequirementRegistry::instance().riskScoreForTest(testId); }
    inline bool HasUncoveredCriticalRequirements() { return RequirementRegistry::instance().hasUncoveredCritical(); }
    inline std::vector<std::string> GetUncoveredCriticalRequirementIds() {
        std::vector<std::string> ids; ids.reserve(8);
        for (auto* r : RequirementRegistry::instance().getUncoveredCritical()) ids.push_back(r->id);
        return ids;
    }

} // namespace speclab::core
