#include <unity.h>
#include <chrono>
#include <string>
#include <types/core_types.hpp>

using namespace rheoscape;
using namespace rheoscape::concepts;

// =============================================================================
// Static assertions to verify TimePointAndDurationCompatible concept
// =============================================================================

// Scalar types: unsigned long for both time point and duration
// This is the typical Arduino millis() pattern.
static_assert(
  TimePointAndDurationCompatible<unsigned long, unsigned long>,
  "unsigned long time point should be compatible with unsigned long duration"
);

// Scalar types: int for both time point and duration
static_assert(
  TimePointAndDurationCompatible<int, int>,
  "int time point should be compatible with int duration"
);

// Scalar types: float for both time point and duration
static_assert(
  TimePointAndDurationCompatible<float, float>,
  "float time point should be compatible with float duration"
);

// Mixed scalar types that are implicitly convertible
static_assert(
  TimePointAndDurationCompatible<long, int>,
  "long time point should be compatible with int duration"
);

static_assert(
  TimePointAndDurationCompatible<double, float>,
  "double time point should be compatible with float duration"
);

// std::chrono types: time_point with matching duration
using steady_time_point = std::chrono::steady_clock::time_point;
using steady_duration = std::chrono::steady_clock::duration;

static_assert(
  TimePointAndDurationCompatible<steady_time_point, steady_duration>,
  "chrono time_point should be compatible with its duration type"
);

// std::chrono types: time_point with different but compatible duration
using milliseconds = std::chrono::milliseconds;
using seconds = std::chrono::seconds;

static_assert(
  TimePointAndDurationCompatible<steady_time_point, milliseconds>,
  "chrono time_point should be compatible with milliseconds"
);

static_assert(
  TimePointAndDurationCompatible<steady_time_point, seconds>,
  "chrono time_point should be compatible with seconds"
);

// =============================================================================
// Static assertions for types that should NOT be compatible
// =============================================================================

// Incompatible types: string is not compatible with numeric duration
struct IncompatibleTimePoint {
  int value;
};

struct IncompatibleDuration {
  float value;
};

// This would fail to compile if uncommented, demonstrating the concept catches errors:
// static_assert(
//   TimePointAndDurationCompatible<IncompatibleTimePoint, IncompatibleDuration>,
//   "Should fail - no subtraction operator defined"
// );

// Verify the concept correctly rejects incompatible types
static_assert(
  !TimePointAndDurationCompatible<IncompatibleTimePoint, IncompatibleDuration>,
  "IncompatibleTimePoint should NOT be compatible with IncompatibleDuration"
);

// Incompatible: string cannot be subtracted or compared with numeric duration
static_assert(
  !TimePointAndDurationCompatible<std::string, int>,
  "std::string should NOT be compatible with int duration"
);

// =============================================================================
// Runtime tests (these just verify the file compiles successfully)
// =============================================================================

void test_concept_compiles_for_scalar_types() {
  // If we get here, the static_asserts above passed.
  // This test verifies the concept is usable at runtime too.
  constexpr bool result = TimePointAndDurationCompatible<unsigned long, unsigned long>;
  TEST_ASSERT_TRUE(result);
}

void test_concept_compiles_for_chrono_types() {
  constexpr bool result = TimePointAndDurationCompatible<steady_time_point, steady_duration>;
  TEST_ASSERT_TRUE(result);
}

void test_concept_rejects_incompatible_types() {
  constexpr bool result = TimePointAndDurationCompatible<IncompatibleTimePoint, IncompatibleDuration>;
  TEST_ASSERT_FALSE(result);
}

void setUp() {}
void tearDown() {}

int main(int argc, char **argv) {
  UNITY_BEGIN();
  RUN_TEST(test_concept_compiles_for_scalar_types);
  RUN_TEST(test_concept_compiles_for_chrono_types);
  RUN_TEST(test_concept_rejects_incompatible_types);
  return UNITY_END();
}
