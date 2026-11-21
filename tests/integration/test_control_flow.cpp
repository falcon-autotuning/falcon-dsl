#include "dsl_test_base.hpp"
using namespace falcon::dsl;
using namespace falcon::dsl::test;
using namespace falcon::typing;
class ControlFlowTest : public DSLTestBase {};

TEST_F(ControlFlowTest, SequentialStates) {
  ParameterMap params;
  SingleCompileEnvironment cenv{
      std::filesystem::path("test-autotuners/control_flow/sequential-test.fal"),
      "SequentialTest", params, true};
  auto [success, outputs] = compile_and_run(cenv);
  ASSERT_TRUE(success);
  ASSERT_FALSE(outputs.empty());
  EXPECT_EQ(std::get<int64_t>(outputs[0]), 3);
}

TEST_F(ControlFlowTest, LoopLikeIteration) {
  ParameterMap params;
  params.emplace("max_iterations", static_cast<int64_t>(10));
  SingleCompileEnvironment cenv{
      std::filesystem::path("test-autotuners/control_flow/iteration-test.fal"),
      "IterationTest", params, true};
  auto [success, outputs] = compile_and_run(cenv);
  ASSERT_TRUE(success);
  ASSERT_FALSE(outputs.empty());
  EXPECT_EQ(std::get<int64_t>(outputs[0]), 10);
}

TEST_F(ControlFlowTest, ConditionalChainNegative) {
  ParameterMap params;
  params.emplace("value", static_cast<int64_t>(-5));
  SingleCompileEnvironment cenv{
      std::filesystem::path(
          "test-autotuners/control_flow/conditional-chain.fal"),
      "ConditionalChain", params, true};
  auto [success, outputs] = compile_and_run(cenv);
  ASSERT_TRUE(success);
  ASSERT_FALSE(outputs.empty());
  EXPECT_EQ(std::get<std::string>(outputs[0]), "negative");
}

TEST_F(ControlFlowTest, ConditionalChainingZero) {
  ParameterMap params;
  params.emplace("value", static_cast<int64_t>(0));
  SingleCompileEnvironment cenv{
      std::filesystem::path(
          "test-autotuners/control_flow/conditional-chain.fal"),
      "ConditionalChain", params, true};
  auto [success, outputs] = compile_and_run(cenv);
  ASSERT_TRUE(success);
  ASSERT_FALSE(outputs.empty());
  EXPECT_EQ(std::get<std::string>(outputs[0]), "zero");
}

TEST_F(ControlFlowTest, ConditionalChainingSmall) {
  ParameterMap params;
  params.emplace("value", static_cast<int64_t>(5));
  SingleCompileEnvironment cenv{
      std::filesystem::path(
          "test-autotuners/control_flow/conditional-chain.fal"),
      "ConditionalChain", params, true};
  auto [success, outputs] = compile_and_run(cenv);
  ASSERT_TRUE(success);
  ASSERT_FALSE(outputs.empty());
  EXPECT_EQ(std::get<std::string>(outputs[0]), "small");
}

TEST_F(ControlFlowTest, ConditionalChainingMedium) {
  ParameterMap params;
  params.emplace("value", static_cast<int64_t>(50));
  SingleCompileEnvironment cenv{
      std::filesystem::path(
          "test-autotuners/control_flow/conditional-chain.fal"),
      "ConditionalChain", params, true};
  auto [success, outputs] = compile_and_run(cenv);
  ASSERT_TRUE(success);
  ASSERT_FALSE(outputs.empty());
  EXPECT_EQ(std::get<std::string>(outputs[0]), "medium");
}

TEST_F(ControlFlowTest, ConditionalChainingLarge) {
  ParameterMap params;
  params.emplace("value", static_cast<int64_t>(200));
  SingleCompileEnvironment cenv{
      std::filesystem::path(
          "test-autotuners/control_flow/conditional-chain.fal"),
      "ConditionalChain", params, true};
  auto [success, outputs] = compile_and_run(cenv);
  ASSERT_TRUE(success);
  ASSERT_FALSE(outputs.empty());
  EXPECT_EQ(std::get<std::string>(outputs[0]), "large");
}

TEST_F(ControlFlowTest, SimpleSweep) {
  ParameterMap params;
  params.emplace("begin", 0.0);
  params.emplace("end", 1.0);
  params.emplace("step", 0.1);
  SingleCompileEnvironment cenv{
      std::filesystem::path("test-autotuners/control_flow/simple-sweep.fal"),
      "SimpleSweep", params, true};
  auto [success, outputs] = compile_and_run(cenv);
  ASSERT_TRUE(success);
  ASSERT_GE(outputs.size(), 2);
  EXPECT_EQ(std::get<int64_t>(outputs[0]), 11);
  EXPECT_NEAR(std::get<double>(outputs[1]), 1.0, 0.01);
}

TEST_F(ControlFlowTest, MultiStageWorkflow) {
  ParameterMap params;
  SingleCompileEnvironment cenv{
      std::filesystem::path(
          "test-autotuners/control_flow/multi-stage-workflow.fal"),
      "MultiStageWorkflow", params, true};
  auto [success, outputs] = compile_and_run(cenv);
  ASSERT_TRUE(success);
  ASSERT_GE(outputs.size(), 2);
  EXPECT_TRUE(std::get<bool>(outputs[1]));
  EXPECT_EQ(std::get<std::string>(outputs[0]), "completed");
}

TEST_F(ControlFlowTest, StateForLoopRejected) {
  falcon::dsl::AutotunerEngine engine;

  bool loaded = engine.load_fal_file(
      "test-autotuners/control_flow/bad-state-for-loop.fal");

  EXPECT_FALSE(loaded) << "Expected load to fail because for-loops are "
                          "not allowed inside autotuner states.";
}

TEST_F(ControlFlowTest, TrailingCommaTransitionArgs) {
  ParameterMap params;
  params["x"] = static_cast<int64_t>(42);

  SingleCompileEnvironment cenv{
      std::filesystem::path(
          "test-autotuners/control_flow/trailing-comma-transition.fal"),
      "TrailingCommaTransition", params, true};

  auto [success, outputs] = compile_and_run(cenv);

  ASSERT_TRUE(success);
  EXPECT_EQ(std::get<int64_t>(outputs[0]), 42);
}

TEST_F(ControlFlowTest, IfStatementDoesNotExecuteFalseBranch) {
  ParameterMap params;

  SingleCompileEnvironment cenv{
      std::filesystem::path(
          "test-autotuners/control_flow/short-circuit-if.fal"),
      "ShortCircuitIf", params, true};

  auto [success, outputs] = compile_and_run(cenv);

  ASSERT_TRUE(success);
  ASSERT_EQ(outputs.size(), 1u);

  EXPECT_EQ(std::get<int64_t>(outputs[0]), static_cast<int64_t>(1));
}

TEST_F(ControlFlowTest, ElseBranchDoesNotExecuteWhenConditionTrue) {
  ParameterMap params;

  SingleCompileEnvironment cenv{
      std::filesystem::path(
          "test-autotuners/control_flow/short-circuit-if-else.fal"),
      "ShortCircuitElse", params, true};

  auto [success, outputs] = compile_and_run(cenv);

  ASSERT_TRUE(success);
  ASSERT_EQ(outputs.size(), 1u);

  EXPECT_EQ(std::get<int64_t>(outputs[0]), static_cast<int64_t>(1));
}

TEST_F(ControlFlowTest, ConditionalExpressionsDoNotEvaluateFalseBranches) {
  ParameterMap params;

  SingleCompileEnvironment cenv{
      std::filesystem::path(
          "test-autotuners/control_flow/conditional-expression-evaluation.fal"),
      "ConditionalExpressionEvaluation", params, true};

  auto [success, outputs] = compile_and_run(cenv);

  ASSERT_TRUE(success);
  ASSERT_EQ(outputs.size(), 1u);

  EXPECT_EQ(std::get<int64_t>(outputs[0]), static_cast<int64_t>(1));
}
