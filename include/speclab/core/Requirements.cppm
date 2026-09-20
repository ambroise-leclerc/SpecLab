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
        // Default member initializer, not a bare declaration: without it every designated
        // initializer that omits .metadata trips -Wmissing-field-initializers under -Werror,
        // pushing `.metadata = {}` into every call site for no semantic gain.
        std::unordered_map<std::string,std::string> metadata{}; // extension point
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

        /// Registers `req`, replacing an earlier record with the same id. Returns true when the id
        /// was new. Re-registering updates the metadata: a risk level raised between two
        /// registrations must reach the coverage gate, and keeping the first record silently would
        /// hide it.
        bool registerRequirement(Requirement req) {
            const std::lock_guard lock{mutex_};
            const std::string id = req.id;
            auto [it, inserted] = requirements_.insert_or_assign(id, std::move(req));
            return inserted;
        }

        bool hasRequirement(std::string_view id) const {
            const std::lock_guard lock{mutex_};
            return requirements_.contains(std::string(id));
        }

        /// By value, not by pointer: the registry is shared and mutable, so a pointer into its
        /// maps could dangle as soon as another thread or another Execute() registers anything.
        std::optional<Requirement> getRequirement(std::string_view id) const {
            const std::lock_guard lock{mutex_};
            auto it = requirements_.find(std::string(id));
            return it == requirements_.end() ? std::nullopt : std::optional<Requirement>{it->second};
        }

        void linkTestToRequirement(std::string_view testId, std::string_view requirementId) {
            const std::lock_guard lock{mutex_};
            testToReq_[std::string(testId)].insert(std::string(requirementId));
            reqToTest_[std::string(requirementId)].insert(std::string(testId));
        }

        /// Removes every test link of `requirementId`. Used when a requirement's tests did not
        /// run: a stale link from an earlier execution would keep it looking covered.
        void clearLinksForRequirement(std::string_view requirementId) {
            const std::lock_guard lock{mutex_};
            const std::string id{requirementId};
            auto it = reqToTest_.find(id);
            if (it == reqToTest_.end()) {
                return;
            }
            for (const std::string& testId : it->second) {
                auto testIt = testToReq_.find(testId);
                if (testIt != testToReq_.end()) {
                    testIt->second.erase(id);
                    if (testIt->second.empty()) {
                        testToReq_.erase(testIt);
                    }
                }
            }
            reqToTest_.erase(it);
        }

        std::set<std::string> getRequirementsForTest(std::string_view testId) const {
            const std::lock_guard lock{mutex_};
            auto it = testToReq_.find(std::string(testId));
            return it == testToReq_.end() ? std::set<std::string>{} : it->second;
        }

        // Produce a simple CSV trace matrix (Requirement -> Tests)
        std::string exportTraceMatrixCSV() const {
            const std::lock_guard lock{mutex_};
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
            const std::lock_guard lock{mutex_};
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

        std::vector<Requirement> getAllRequirements() const {
            const std::lock_guard lock{mutex_};
            std::vector<Requirement> out; out.reserve(requirements_.size());
            for (auto& [_, r] : requirements_) {
                out.push_back(r);
            }
            return out;
        }
        std::vector<Requirement> getUncoveredHighRisk() const {
            const std::lock_guard lock{mutex_};
            std::vector<Requirement> missing;
            for (auto& [id, r] : requirements_) {
                if (r.riskLevel == "HIGH" || r.riskLevel == "CRITICAL") {
                    if (!reqToTest_.contains(id) || reqToTest_.at(id).empty()) missing.push_back(r);
                }
            }
            return missing;
        }
        std::vector<Requirement> getUncoveredCritical() const {
            const std::lock_guard lock{mutex_};
            std::vector<Requirement> missing;
            for (auto& [id, r] : requirements_) {
                if (r.riskLevel == "CRITICAL") {
                    if (!reqToTest_.contains(id) || reqToTest_.at(id).empty()) missing.push_back(r);
                }
            }
            return missing;
        }
        RequirementsConfig getConfig() const {
            const std::lock_guard lock{mutex_};
            return config_;
        }
        void setConfig(const RequirementsConfig& cfg) {
            const std::lock_guard lock{mutex_};
            config_ = cfg;
        }
        bool hasUncoveredCritical() const {
            const std::lock_guard lock{mutex_};
            for (auto& [id, r] : requirements_) {
                if ((r.riskLevel == "CRITICAL") && (!reqToTest_.contains(id) || reqToTest_.at(id).empty())) return true;
            }
            return false;
        }
        int riskScoreForTest(std::string_view testId) const {
            const std::lock_guard lock{mutex_};
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
            const std::lock_guard lock{mutex_};
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
            std::size_t total=requirements_.size(), covered=0, high=0, highCovered=0;
            for (auto& [id,r]:requirements_) {
                bool cov = reqToTest_.contains(id) && !reqToTest_.at(id).empty();
                if (cov) ++covered;
                if (r.riskLevel=="HIGH"||r.riskLevel=="CRITICAL") { ++high; if (cov) ++highCovered; }
            }
            html += std::format("<p>Total requirements: {} | Covered: {} | Coverage: {:.1f}%</p>", total, covered, total? (100.0*static_cast<double>(covered)/static_cast<double>(total)):100.0);
            html += std::format("<p>High/Critical: {} | Covered: {} | Coverage: {:.1f}%</p>", high, highCovered, high? (100.0*static_cast<double>(highCovered)/static_cast<double>(high)):100.0);
            html += "</body></html>";
            return html;
        }
    private:
        /// The registry is a process-wide singleton that TestSuite reads from parallel test
        /// threads while Requirement::Execute() writes to it, so every accessor locks. Public
        /// methods never call one another, so a plain mutex cannot deadlock here.
        mutable std::mutex mutex_;
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

    /// Drops every test link of `requirementId`, leaving the requirement itself registered.
    inline void ClearRequirementTestLinks(std::string_view requirementId) {
        RequirementRegistry::instance().clearLinksForRequirement(requirementId);
    }

    /// Canonical spelling of a risk level: LOW, MEDIUM, HIGH or CRITICAL. The builders accept
    /// "Critical" as well as "CRITICAL", while the coverage gate compares against the upper-case
    /// form, so the spelling is normalised before anything is registered.
    inline std::string NormalizeRiskLevel(std::string_view riskLevel) {
        std::string upper;
        upper.reserve(riskLevel.size());
        for (const char character : riskLevel) {
            upper.push_back(static_cast<char>(std::toupper(static_cast<unsigned char>(character))));
        }
        return upper;
    }

    /// Canonical spelling of a safety class: CLASS_A, CLASS_B or CLASS_C ("ClassC" and "class c"
    /// included).
    inline std::string NormalizeSafetyClass(std::string_view safetyClass) {
        std::string upper = NormalizeRiskLevel(safetyClass);
        std::erase(upper, ' ');
        std::erase(upper, '_');
        if (upper.size() == 6 && upper.starts_with("CLASS")) {
            return std::format("CLASS_{}", upper.back());
        }
        return std::string{safetyClass};
    }

    inline std::string ExportTraceMatrixCSV() {
        return RequirementRegistry::instance().exportTraceMatrixCSV();
    }

    inline void AugmentResultWithRequirements(TestResult& result) {
        if (result.testId.empty()) return;
        for (const std::string& id : RequirementRegistry::instance().getRequirementsForTest(result.testId)) {
            result.addRequirementId(id);
        }
    }

    inline void AddCoverageValidationResult(TestResultCollection& collection) {
        auto missing = RequirementRegistry::instance().getUncoveredHighRisk();
        TestResult coverageResult(TestStatus::Passed, "All high-risk requirements covered");
        coverageResult.testId = "REQUIREMENT_COVERAGE";
        if (!missing.empty()) {
            bool anyCritical = std::any_of(missing.begin(), missing.end(), [](const Requirement& r){ return r.riskLevel == "CRITICAL"; });
            coverageResult.status = anyCritical ? TestStatus::Critical : TestStatus::Failed;
            std::string list;
            for (std::size_t i=0;i<missing.size();++i){ 
                if(i) list += ","; 
                list += missing[i].id; 
            }
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
        for (const Requirement& r : RequirementRegistry::instance().getUncoveredCritical()) ids.push_back(r.id);
        return ids;
    }

} // namespace speclab::core
