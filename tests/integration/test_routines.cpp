#include "dsl_test_base.hpp"
using namespace falcon::dsl;
using namespace falcon::dsl::test;
using namespace falcon::typing;

class RoutineTest : public DSLTestBase {};

TEST_F(RoutineTest, ConditionalNestGood) {
  ParameterMap params;
  SingleCompileEnvironment cenv{
      std::filesystem::path("test-autotuners/routines/simple-routine.fal"),
      "ConditionalNest", params, true};
  params.emplace("a", static_cast<int64_t>(5));
  params.emplace("b", static_cast<int64_t>(0));
  auto [success, outputs] = compile_and_run(cenv);
  ASSERT_TRUE(success);
  ASSERT_GE(outputs.size(), 1);
  EXPECT_EQ(std::get<int64_t>(outputs[0]), static_cast<int64_t>(0));
}

TEST_F(RoutineTest, ForLoopSum) {
  ParameterMap params;
  params.emplace("n", static_cast<int64_t>(5));
  SingleCompileEnvironment cenv{
      std::filesystem::path("test-autotuners/routines/simple-for.fal"),
      "ForLoopSum", params, true};
  auto [success, outputs] = compile_and_run(cenv);
  ASSERT_TRUE(success);
  EXPECT_EQ(std::get<int64_t>(outputs[0]), static_cast<int64_t>(10));
}

TEST_F(RoutineTest, RangeForLoopSum) {
  ParameterMap params;
  params.emplace("n", static_cast<int64_t>(5));
  SingleCompileEnvironment cenv{
      std::filesystem::path("test-autotuners/routines/range-for.fal"),
      "ForLoopSum", params, true};
  auto [success, outputs] = compile_and_run(cenv);
  ASSERT_TRUE(success);
  EXPECT_EQ(std::get<int64_t>(outputs[0]), static_cast<int64_t>(15));
}

TEST_F(RoutineTest, NestedForLoopSum) {
  ParameterMap params;
  params.emplace("n", static_cast<int64_t>(3));
  SingleCompileEnvironment cenv{
      std::filesystem::path("test-autotuners/routines/nested-loop.fal"),
      "ForLoopSum", params, true};
  auto [success, outputs] = compile_and_run(cenv);
  ASSERT_TRUE(success);
  EXPECT_EQ(std::get<int64_t>(outputs[0]), static_cast<int64_t>(9));
}

TEST_F(RoutineTest, ForLoopCallsRoutine) {
  ParameterMap params;
  params.emplace("n", static_cast<int64_t>(5));
  SingleCompileEnvironment cenv{
      std::filesystem::path("test-autotuners/routines/nested-loop-routine.fal"),
      "ForLoopSum", params, true};
  auto [success, outputs] = compile_and_run(cenv);
  ASSERT_TRUE(success);
  EXPECT_EQ(std::get<int64_t>(outputs[0]), static_cast<int64_t>(10));
}

TEST_F(RoutineTest, NoLoopNoError) {
  ParameterMap params;
  params.emplace("n", static_cast<int64_t>(5));
  SingleCompileEnvironment cenv{
      std::filesystem::path("test-autotuners/routines/no-loop.fal"),
      "ForLoopSum", params, true};
  auto [success, outputs] = compile_and_run(cenv);
  ASSERT_TRUE(success);
  EXPECT_EQ(std::get<int64_t>(outputs[0]), static_cast<int64_t>(0));
}

TEST_F(RoutineTest, BadVariableShadowing) {
  falcon::dsl::AutotunerEngine engine;
  bool loaded = engine.load_fal_file(
      "test-autotuners/routines/bad-variable-shadowing.fal");
  EXPECT_FALSE(loaded)
      << "Expected load to fail due to variable shadowing, but it succeeded";
}

TEST_F(RoutineTest, BadNestedReuse) {
  falcon::dsl::AutotunerEngine engine;
  bool loaded =
      engine.load_fal_file("test-autotuners/routines/bad-nested-reuse.fal");
  EXPECT_FALSE(loaded) << "Expected load to fail due to variable nesting "
                          "shadowing, but it succeeded";
}
