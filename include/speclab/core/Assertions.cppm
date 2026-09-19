/**
 * @brief Test assertions for medical device testing - C++23 Module
 */

export module speclab.core.assertions;

import std;
import speclab.core.testresult;

export namespace speclab::core {

    /**
     * @brief Exception thrown when assertion fails
     */
    class AssertionFailure : public std::exception {
    public:
        explicit AssertionFailure(std::string message, 
                                 std::source_location loc = std::source_location::current())
            : message_(std::move(message)), location_(loc) {}
        
        const char* what() const noexcept override {
            return message_.c_str();
        }
        
        const std::source_location& location() const noexcept {
            return location_;
        }
        
        /**
         * @brief "file:line:column", with the file reduced to its name
         *
         * A source_location carries the absolute path the compiler was given, which differs
         * between machines and build trees for the same failure. Reporting only the file name keeps
         * failure messages identical wherever they are produced; location() still has the full path.
         */
        std::string formatLocation() const {
            return std::format("{}:{}:{}", fileName(location_.file_name()),
                             location_.line(), location_.column());
        }

        /// The last path component of `path`, for either separator.
        [[nodiscard]] static std::string_view fileName(std::string_view path) noexcept {
            const std::size_t slash = path.find_last_of("/\\");
            return slash == std::string_view::npos ? path : path.substr(slash + 1);
        }
        
    private:
        std::string message_;
        std::source_location location_;
    };

    /**
     * @brief Medical device assertion context for compliance tracking
     */
    struct AssertionContext {
        std::string testId;
        std::string riskLevel = "LOW";
        std::string complianceStandard = "IEC_62304";
        std::string deviceComponent;
        bool requiresAudit = false;
        
        AssertionContext(std::string_view id, std::string_view component = "")
            : testId(id), deviceComponent(component) {}
    };

    /// An integer type other than bool; used by assertEqual to compare mixed integer types safely.
    template<typename T>
    concept Integer = std::integral<T> && !std::same_as<std::remove_cv_t<T>, bool>;

    /**
     * @brief A compile-time-checked format string plus the caller's source location
     *
     * A function cannot take both a variadic argument pack and a defaulted std::source_location
     * after it, so the location is captured by this parameter's consteval constructor instead:
     * `require(cond, "value {}", v)` records the line of the `require` call.
     */
    template<typename... Args>
    struct FormatWithLocation {
        std::format_string<Args...> format;
        std::source_location location;

        template<typename String>
            requires std::convertible_to<const String&, std::string_view>
        consteval FormatWithLocation(const String& text,  // NOLINT(google-explicit-constructor)
                                     std::source_location where = std::source_location::current())
            : format(text), location(where) {}
    };

    /**
     * @brief Core assertion functions for medical device testing
     */
    class Assertions {
    public:
        /**
         * @brief Assert that condition is true
         * @param condition Boolean condition to check
         * @param message Custom failure message
         * @param loc Source location (auto-captured)
         */
        static void assertTrue(bool condition, 
                             std::string_view message = "Assertion failed",
                             std::source_location loc = std::source_location::current()) {
            if (!condition) {
                throw AssertionFailure(std::format("assertTrue failed: {}", message), loc);
            }
        }
        
        /**
         * @brief Assert that condition is false
         * @param condition Boolean condition to check
         * @param message Custom failure message
         * @param loc Source location (auto-captured)
         */
        static void assertFalse(bool condition, 
                              std::string_view message = "Assertion failed",
                              std::source_location loc = std::source_location::current()) {
            if (condition) {
                throw AssertionFailure(std::format("assertFalse failed: {}", message), loc);
            }
        }
        
        /**
         * @brief Assert that two values are equal
         *
         * The two values may have different types, as long as they can be compared: an `int`
         * literal against a `std::size_t`, a string literal against a `std::string`. Two integers
         * are compared with std::cmp_equal, so a negative value never equals a large unsigned one
         * and no sign-conversion warning is emitted in the caller's build.
         *
         * @param expected Expected value
         * @param actual Actual value
         * @param message Custom failure message
         * @param loc Source location (auto-captured)
         */
        template<typename E, typename A>
            requires std::equality_comparable_with<const E&, const A&> || (Integer<E> && Integer<A>)
        static void assertEqual(const E& expected, const A& actual,
                              std::string_view message = "",
                              std::source_location loc = std::source_location::current()) {
            if (!valuesEqual(expected, actual)) {
                std::string fullMessage;
                if constexpr (std::formattable<E, char> && std::formattable<A, char>) {
                    fullMessage = std::format("assertEqual failed: expected '{}', got '{}'. {}", 
                                            expected, actual, message);
                } else {
                    fullMessage = std::format("assertEqual failed. {}", message);
                }
                throw AssertionFailure(fullMessage, loc);
            }
        }
        
        /**
         * @brief Assert that two values are not equal (the types may differ, as for assertEqual)
         * @param unexpected Value that should not match
         * @param actual Actual value
         * @param message Custom failure message
         * @param loc Source location (auto-captured)
         */
        template<typename U, typename A>
            requires std::equality_comparable_with<const U&, const A&> || (Integer<U> && Integer<A>)
        static void assertNotEqual(const U& unexpected, const A& actual,
                                 std::string_view message = "",
                                 std::source_location loc = std::source_location::current()) {
            if (valuesEqual(unexpected, actual)) {
                std::string fullMessage;
                if constexpr (std::formattable<U, char>) {
                    fullMessage = std::format("assertNotEqual failed: values are equal '{}'. {}", 
                                            unexpected, message);
                } else {
                    fullMessage = std::format("assertNotEqual failed: values are equal. {}", message);
                }
                throw AssertionFailure(fullMessage, loc);
            }
        }
        
        /**
         * @brief Assert `condition`, with a std::format message built only when it fails
         *
         *     Assertions::require(diagnostics.size() == 1, "expected 1 diagnostic, got {}", diagnostics.size());
         *
         * The format string is checked at compile time and the caller's source location is
         * captured, so this replaces `if (!c) throw AssertionFailure(std::format(...), current())`.
         */
        template<typename... Args>
        static void require(bool condition, FormatWithLocation<std::type_identity_t<Args>...> message,
                            Args&&... args) {
            if (!condition) {
                throw AssertionFailure(std::format(message.format, std::forward<Args>(args)...),
                                       message.location);
            }
        }

        /**
         * @brief Assert that pointer is null
         * @param ptr Pointer to check
         * @param message Custom failure message
         * @param loc Source location (auto-captured)
         */
        static void assertNull(const void* ptr, 
                             std::string_view message = "Expected null pointer",
                             std::source_location loc = std::source_location::current()) {
            if (ptr != nullptr) {
                throw AssertionFailure(std::format("assertNull failed: {}", message), loc);
            }
        }
        
        /**
         * @brief Assert that pointer is not null
         * @param ptr Pointer to check
         * @param message Custom failure message
         * @param loc Source location (auto-captured)
         */
        static void assertNotNull(const void* ptr, 
                                std::string_view message = "Expected non-null pointer",
                                std::source_location loc = std::source_location::current()) {
            if (ptr == nullptr) {
                throw AssertionFailure(std::format("assertNotNull failed: {}", message), loc);
            }
        }
        
        /**
         * @brief Assert that value is within tolerance of expected
         * @tparam T Numeric type
         * @param expected Expected value
         * @param actual Actual value
         * @param tolerance Acceptable tolerance
         * @param message Custom failure message
         * @param loc Source location (auto-captured)
         */
        template<typename T>
        requires std::is_arithmetic_v<T>
        static void assertNear(T expected, T actual, T tolerance, 
                             std::string_view message = "",
                             std::source_location loc = std::source_location::current()) {
            T diff = (actual > expected) ? (actual - expected) : (expected - actual);
            if (diff > tolerance) {
                std::string fullMessage = std::format(
                    "assertNear failed: expected {} ± {}, got {} (diff: {}). {}", 
                    expected, tolerance, actual, diff, message);
                throw AssertionFailure(fullMessage, loc);
            }
        }
        
        /**
         * @brief Assert that exception is thrown
         * @tparam ExceptionType Expected exception type
         * @tparam Callable Callable type
         * @param callable Function/lambda to execute
         * @param message Custom failure message
         * @param loc Source location (auto-captured)
         */
        template<typename ExceptionType, typename Callable>
        static void assertThrows(Callable&& callable, 
                               std::string_view message = "",
                               std::source_location loc = std::source_location::current()) {
            try {
                std::forward<Callable>(callable)();
                throw AssertionFailure(std::format("assertThrows failed: no exception thrown. {}", message), loc);
            } catch (const ExceptionType&) {
                // Expected exception caught - success
            } catch (...) {
                throw AssertionFailure(std::format("assertThrows failed: wrong exception type. {}", message), loc);
            }
        }
        
        /**
         * @brief Assert that no exception is thrown
         * @tparam Callable Callable type
         * @param callable Function/lambda to execute
         * @param message Custom failure message
         * @param loc Source location (auto-captured)
         */
        template<typename Callable>
        static void assertNoThrow(Callable&& callable, 
                                std::string_view message = "",
                                std::source_location loc = std::source_location::current()) {
            try {
                std::forward<Callable>(callable)();
            } catch (const std::exception& e) {
                throw AssertionFailure(std::format("assertNoThrow failed: exception thrown: {}. {}", 
                                                  e.what(), message), loc);
            } catch (...) {
                throw AssertionFailure(std::format("assertNoThrow failed: unknown exception thrown. {}", message), loc);
            }
        }
        
        /**
         * @brief Force test failure with message
         * @param message Failure message
         * @param loc Source location (auto-captured)
         */
        [[noreturn]] static void fail(std::string_view message = "Test failed",
                                    std::source_location loc = std::source_location::current()) {
            throw AssertionFailure(std::string(message), loc);
        }
        
        // Medical device specific assertions
        
        /**
         * @brief Assert critical safety condition for medical devices
         * @param condition Safety condition to verify
         * @param riskDescription Description of risk if condition fails
         * @param loc Source location (auto-captured)
         */
        static void assertSafety(bool condition, 
                               std::string_view riskDescription,
                               std::source_location loc = std::source_location::current()) {
            if (!condition) {
                throw AssertionFailure(std::format("SAFETY CRITICAL: {}", riskDescription), loc);
            }
        }
        
        /**
         * @brief Assert compliance with medical device standards
         * @param condition Compliance condition
         * @param standard Applicable standard (e.g., "IEC 62304")
         * @param requirement Specific requirement reference
         * @param loc Source location (auto-captured)
         */
        static void assertCompliance(bool condition,
                                   std::string_view standard,
                                   std::string_view requirement,
                                   std::source_location loc = std::source_location::current()) {
            if (!condition) {
                throw AssertionFailure(std::format("COMPLIANCE FAILURE: {} - {}", standard, requirement), loc);
            }
        }
        
        /**
         * @brief Assert performance requirement for real-time medical systems
         * @param duration Measured duration
         * @param maxAllowed Maximum allowed duration
         * @param operation Description of operation
         * @param loc Source location (auto-captured)
         */
        static void assertPerformance(std::chrono::nanoseconds duration,
                                    std::chrono::nanoseconds maxAllowed,
                                    std::string_view operation,
                                    std::source_location loc = std::source_location::current()) {
            if (duration > maxAllowed) {
                auto durationMs = std::chrono::duration<double, std::milli>(duration).count();
                auto maxMs = std::chrono::duration<double, std::milli>(maxAllowed).count();
                throw AssertionFailure(std::format("PERFORMANCE FAILURE: {} took {:.2f}ms, max allowed {:.2f}ms", 
                                                  operation, durationMs, maxMs), loc);
            }
        }

    private:
        template<typename L, typename R>
        static constexpr bool valuesEqual(const L& lhs, const R& rhs) {
            if constexpr (Integer<L> && Integer<R>) {
                return std::cmp_equal(lhs, rhs);
            } else {
                return lhs == rhs;
            }
        }
    };

    /**
     * @brief Collects several failed expectations and reports them together
     *
     * Assertions throw, so the first failure inside a step ends the test. That is right for a step
     * making one claim, and wrong for one checking six properties: Checks reports every one that
     * failed in a single AssertionFailure, so they can all be fixed in one pass.
     *
     *     Checks checks;
     *     checks.expect(vertex.x == 4, "x");
     *     checks.expect(vertex.y == 8, "y");
     *     checks.raise();   // throws only if something failed, naming each failure
     */
    class Checks {
    public:
        /// Records `condition`. `what` names the thing being checked, not the operator.
        void expect(bool condition, std::string_view what,
                    std::source_location where = std::source_location::current()) {
            if (!condition) {
                failures_.push_back(std::format("{} ({}:{})", what,
                                                AssertionFailure::fileName(where.file_name()),
                                                where.line()));
            }
        }

        /// Throws one AssertionFailure listing every failed expectation, or returns if none failed.
        void raise(std::source_location where = std::source_location::current()) const {
            if (failures_.empty()) {
                return;
            }
            std::string message = std::format("{} expectation(s) failed:", failures_.size());
            for (const std::string& failure : failures_) {
                message += "\n    - " + failure;
            }
            throw AssertionFailure(message, where);
        }

        [[nodiscard]] bool anyFailed() const noexcept { return !failures_.empty(); }

    private:
        std::vector<std::string> failures_;
    };

    // No ASSERT_* / FAIL macros: a #define in a module interface unit is never exported, so the
    // ones that used to sit here were unreachable through `import speclab;`. Call the
    // Assertions:: functions directly - they capture std::source_location themselves.

} // namespace speclab::core