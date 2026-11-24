#include "dsl_test_base.hpp"
using namespace falcon::dsl;
using namespace falcon::dsl::test;
using namespace falcon::typing;

class ForeignFunctionTest : public DSLTestBase {
protected:
  void TearDown() override {
    // List all possible FFI test folders
    std::vector<std::string> test_dirs = {"test-autotuners/foreign_function"};
    for (const auto &dir : test_dirs) {
      std::filesystem::remove(std::filesystem::path(dir) / "falcon.yml");
      std::filesystem::remove(std::filesystem::path(dir) / "falcon.yaml");
    }
    DSLTestBase::TearDown();
  }
};

TEST_F(ForeignFunctionTest, QuantityStruct) {
  ParameterMap params;
  params["a"] = static_cast<int64_t>(10);
  params["b"] = static_cast<int64_t>(5);
  SingleCompileEnvironment cenv{
      std::filesystem::path(
          "test-autotuners/foreign_function/quantity-struct.fal"),
      "QuantityStruct", params, true};
  auto [success, outputs] = compile_and_run(cenv);
  ASSERT_TRUE(success);
  ASSERT_FALSE(outputs.empty());
  EXPECT_EQ(std::get<int64_t>(outputs[0]), 15);
}

TEST_F(ForeignFunctionTest, ConditionalNestGood) {
  ParameterMap params;
  SingleCompileEnvironment cenv{
      std::filesystem::path("test-autotuners/foreign_function/routine.fal"),
      "ConditionalNest", params, true};
  params.emplace("a", static_cast<int64_t>(5));
  params.emplace("b", static_cast<int64_t>(1));
  auto [success, outputs] = compile_and_run(cenv);
  ASSERT_TRUE(success);
  ASSERT_GE(outputs.size(), 1);
  EXPECT_EQ(std::get<int64_t>(outputs[0]), static_cast<int64_t>(6));
}

TEST_F(ForeignFunctionTest, ConnectionBinding) {
  ParameterMap params;
  SingleCompileEnvironment cenv{
      std::filesystem::path(
          "test-autotuners/foreign_function/connection-binding.fal"),
      "ConnectionBinding", params, true};
  params.emplace("name", static_cast<std::string>("P1"));
  auto [success, outputs] = compile_and_run(cenv);
  ASSERT_TRUE(success);
  ASSERT_GE(outputs.size(), 2);
  EXPECT_EQ(std::get<std::string>(outputs[0]), "PlungerGate");
  EXPECT_TRUE(std::get<bool>(outputs[1]));
}

TEST_F(ForeignFunctionTest, RuntimeEqualityOverride) {
  ParameterMap params;

  SingleCompileEnvironment cenv{
      std::filesystem::path(
          "test-autotuners/foreign_function/runtime-equality.fal"),
      "RuntimeEqualityTest", params, true};

  auto [success, outputs] = compile_and_run(cenv);

  ASSERT_TRUE(success);
  ASSERT_EQ(outputs.size(), 1u);

  EXPECT_TRUE(std::get<bool>(outputs[0]));
}

TEST_F(ForeignFunctionTest, RuntimeNotEqualityOverride) {
  ParameterMap params;

  SingleCompileEnvironment cenv{
      std::filesystem::path(
          "test-autotuners/foreign_function/runtime-notequal.fal"),
      "RuntimeNotEqualTest", params, true};
  {
    params.emplace("_a", static_cast<int64_t>(1));
    params.emplace("_b", static_cast<int64_t>(2));
    auto [success, outputs] = compile_and_run(cenv);

    ASSERT_TRUE(success);
    ASSERT_EQ(outputs.size(), 1u);

    EXPECT_TRUE(std::get<bool>(outputs[0]));
  }
  params.clear();
  {
    params.emplace("_a", static_cast<int64_t>(1));
    params.emplace("_b", static_cast<int64_t>(1));
    auto [success, outputs] = compile_and_run(cenv);

    ASSERT_TRUE(success);
    ASSERT_EQ(outputs.size(), 1u);

    EXPECT_FALSE(std::get<bool>(outputs[0]));
  }
}

TEST_F(ForeignFunctionTest, RuntimeNotEqualityInferred) {
  ParameterMap params;

  SingleCompileEnvironment cenv{
      std::filesystem::path(
          "test-autotuners/foreign_function/runtime-not-equality-inferred.fal"),
      "RuntimeNotEqualInferredTest", params, true};
  {
    params.emplace("_a", static_cast<int64_t>(1));
    params.emplace("_b", static_cast<int64_t>(2));
    auto [success, outputs] = compile_and_run(cenv);

    ASSERT_TRUE(success);
    ASSERT_EQ(outputs.size(), 1u);

    EXPECT_TRUE(std::get<bool>(outputs[0]));
  }
  params.clear();
  {
    params.emplace("_a", static_cast<int64_t>(1));
    params.emplace("_b", static_cast<int64_t>(1));
    auto [success, outputs] = compile_and_run(cenv);

    ASSERT_TRUE(success);
    ASSERT_EQ(outputs.size(), 1u);

    EXPECT_FALSE(std::get<bool>(outputs[0]));
  }
}

TEST_F(ForeignFunctionTest, RuntimeOrderingOverride) {
  ParameterMap params;

  SingleCompileEnvironment cenv{
      std::filesystem::path(
          "test-autotuners/foreign_function/runtime-ordering.fal"),
      "RuntimeOrderingTest", params, true};

  auto [success, outputs] = compile_and_run(cenv);

  ASSERT_TRUE(success);
  ASSERT_EQ(outputs.size(), 2u);

  EXPECT_TRUE(std::get<bool>(outputs[0]));
  EXPECT_TRUE(std::get<bool>(outputs[1]));
}

TEST_F(ForeignFunctionTest, RuntimeGreaterThanInferred) {
  ParameterMap params;

  SingleCompileEnvironment cenv{
      std::filesystem::path(
          "test-autotuners/foreign_function/runtime-greater-than-inferred.fal"),
      "RuntimeGreaterThanInferredTest", params, true};

  params["a"] = static_cast<int64_t>(5);
  params["b"] = static_cast<int64_t>(1);

  auto [success1, outputs1] = compile_and_run(cenv);

  ASSERT_TRUE(success1);
  ASSERT_EQ(outputs1.size(), 1u);

  EXPECT_TRUE(std::get<bool>(outputs1[0]));

  params["a"] = static_cast<int64_t>(1);
  params["b"] = static_cast<int64_t>(5);

  auto [success2, outputs2] = compile_and_run(cenv);

  ASSERT_TRUE(success2);
  ASSERT_EQ(outputs2.size(), 1u);

  EXPECT_FALSE(std::get<bool>(outputs2[0]));
}

TEST_F(ForeignFunctionTest, RuntimeLessThanInferred) {
  ParameterMap params;

  SingleCompileEnvironment cenv{
      std::filesystem::path(
          "test-autotuners/foreign_function/runtime-less-than-inferred.fal"),
      "RuntimeLessThanInferredTest", params, true};

  params["a"] = static_cast<int64_t>(1);
  params["b"] = static_cast<int64_t>(5);

  auto [success1, outputs1] = compile_and_run(cenv);

  ASSERT_TRUE(success1);
  ASSERT_EQ(outputs1.size(), 1u);

  EXPECT_TRUE(std::get<bool>(outputs1[0]));

  params["a"] = static_cast<int64_t>(5);
  params["b"] = static_cast<int64_t>(1);

  auto [success2, outputs2] = compile_and_run(cenv);

  ASSERT_TRUE(success2);
  ASSERT_EQ(outputs2.size(), 1u);

  EXPECT_FALSE(std::get<bool>(outputs2[0]));
}
