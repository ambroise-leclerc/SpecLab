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
        
        std::string formatLocation() const {
            return std::format("{}:{}:{}", location_.file_name(), 
                             location_.line(), location_.column());
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
         * @tparam T Type of values to compare
         * @param expected Expected value
         * @param actual Actual value
         * @param message Custom failure message
         * @param loc Source location (auto-captured)
         */
        template<typename T>
        static void assertEqual(const T& expected, const T& actual, 
                              std::string_view message = "",
                              std::source_location loc = std::source_location::current()) {
            if (!(expected == actual)) {
                std::string fullMessage;
                if constexpr (requires { std::format("{}", expected); }) {
                    fullMessage = std::format("assertEqual failed: expected '{}', got '{}'. {}", 
                                            expected, actual, message);
                } else {
                    fullMessage = std::format("assertEqual failed. {}", message);
                }
                throw AssertionFailure(fullMessage, loc);
            }
        }
        
        /**
         * @brief Assert that two values are not equal
         * @tparam T Type of values to compare
         * @param unexpected Value that should not match
         * @param actual Actual value
         * @param message Custom failure message
         * @param loc Source location (auto-captured)
         */
        template<typename T>
        static void assertNotEqual(const T& unexpected, const T& actual, 
                                 std::string_view message = "",
                                 std::source_location loc = std::source_location::current()) {
            if (unexpected == actual) {
                std::string fullMessage;
                if constexpr (requires { std::format("{}", unexpected); }) {
                    fullMessage = std::format("assertNotEqual failed: values are equal '{}'. {}", 
                                            unexpected, message);
                } else {
                    fullMessage = std::format("assertNotEqual failed: values are equal. {}", message);
                }
                throw AssertionFailure(fullMessage, loc);
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
    };

    // Convenience macros for shorter syntax (optional)
    #define ASSERT_TRUE(cond, ...) speclab::core::Assertions::assertTrue(cond, ##__VA_ARGS__)
    #define ASSERT_FALSE(cond, ...) speclab::core::Assertions::assertFalse(cond, ##__VA_ARGS__)
    #define ASSERT_EQ(exp, act, ...) speclab::core::Assertions::assertEqual(exp, act, ##__VA_ARGS__)
    #define ASSERT_NE(unexp, act, ...) speclab::core::Assertions::assertNotEqual(unexp, act, ##__VA_ARGS__)
    #define ASSERT_NULL(ptr, ...) speclab::core::Assertions::assertNull(ptr, ##__VA_ARGS__)
    #define ASSERT_NOT_NULL(ptr, ...) speclab::core::Assertions::assertNotNull(ptr, ##__VA_ARGS__)
    #define ASSERT_NEAR(exp, act, tol, ...) speclab::core::Assertions::assertNear(exp, act, tol, ##__VA_ARGS__)
    #define ASSERT_THROWS(type, call, ...) speclab::core::Assertions::assertThrows<type>(call, ##__VA_ARGS__)
    #define ASSERT_NO_THROW(call, ...) speclab::core::Assertions::assertNoThrow(call, ##__VA_ARGS__)
    #define FAIL(...) speclab::core::Assertions::fail(__VA_ARGS__)
    
    // Medical device specific macros
    #define ASSERT_SAFETY(cond, risk, ...) speclab::core::Assertions::assertSafety(cond, risk, ##__VA_ARGS__)
    #define ASSERT_COMPLIANCE(cond, std, req, ...) speclab::core::Assertions::assertCompliance(cond, std, req, ##__VA_ARGS__)
    #define ASSERT_PERFORMANCE(dur, max, op, ...) speclab::core::Assertions::assertPerformance(dur, max, op, ##__VA_ARGS__)

} // namespace speclab::core