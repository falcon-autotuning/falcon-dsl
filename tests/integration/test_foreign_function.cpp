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

TEST_F(ForeignFunctionTest, FalconStructFfiEqual) {
  ParameterMap params;

  SingleCompileEnvironment cenv{
      std::filesystem::path(
          "test-autotuners/foreign_function/ffi-equal-native-struct.fal"),
      "FalconStructEqualTest", params, true};

  auto [success, outputs] = compile_and_run(cenv);

  ASSERT_TRUE(success);
  ASSERT_EQ(outputs.size(), 1u);

  EXPECT_TRUE(std::get<bool>(outputs[0]));
}

TEST_F(ForeignFunctionTest, FalconStructFfiNotEqual) {
  ParameterMap params;

  SingleCompileEnvironment cenv{
      std::filesystem::path(
          "test-autotuners/foreign_function/ffi-notequal-native-struct.fal"),
      "FalconStructNotEqualTest", params, true};

  auto [success, outputs] = compile_and_run(cenv);

  ASSERT_TRUE(success);
  ASSERT_EQ(outputs.size(), 1u);

  EXPECT_TRUE(std::get<bool>(outputs[0]));
}

TEST_F(ForeignFunctionTest, InferNotEqualFromEqualNativeStruct) {
  ParameterMap params;

  SingleCompileEnvironment cenv{
      std::filesystem::path(
          "test-autotuners/foreign_function/infer-notequal-from-equal.fal"),
      "InferNotEqualFromEqualTest", params, true};

  auto [success, outputs] = compile_and_run(cenv);

  ASSERT_TRUE(success);
  ASSERT_EQ(outputs.size(), 1u);

  EXPECT_TRUE(std::get<bool>(outputs[0]));
}

TEST_F(ForeignFunctionTest, InferEqualFromNotEqualNativeStruct) {
  ParameterMap params;

  SingleCompileEnvironment cenv{
      std::filesystem::path(
          "test-autotuners/foreign_function/infer-equal-from-notequal.fal"),
      "InferEqualFromNotEqualTest", params, true};

  auto [success, outputs] = compile_and_run(cenv);

  ASSERT_TRUE(success);
  ASSERT_EQ(outputs.size(), 1u);

  EXPECT_TRUE(std::get<bool>(outputs[0]));
}

TEST_F(ForeignFunctionTest, RuntimeEqualityFallsBackToStructFields) {
  ParameterMap params;

  SingleCompileEnvironment cenv{
      std::filesystem::path("test-autotuners/foreign_function/"
                            "runtime-native-struct-equality.fal"),
      "RuntimeNativeStructEquality", params, true};

  auto [success, outputs] = compile_and_run(cenv);

  ASSERT_TRUE(success);
  ASSERT_EQ(outputs.size(), 1u);

  EXPECT_TRUE(std::get<bool>(outputs[0]));
}

TEST_F(ForeignFunctionTest, RuntimeNotEqualityFallsBackToStructFields) {
  ParameterMap params;

  SingleCompileEnvironment cenv{
      std::filesystem::path("test-autotuners/foreign_function/"
                            "runtime-native-struct-notequality.fal"),
      "RuntimeNativeStructNotEquality", params, true};

  auto [success, outputs] = compile_and_run(cenv);

  ASSERT_TRUE(success);
  ASSERT_EQ(outputs.size(), 1u);

  EXPECT_TRUE(std::get<bool>(outputs[0]));
}

TEST_F(ForeignFunctionTest, RuntimeNativeStructError) {
  ParameterMap params;

  SingleCompileEnvironment cenv{
      std::filesystem::path("test-autotuners/foreign_function/"
                            "runtime-native-struct-error.fal"),
      "RuntimeNativeStructError", params, false};

  auto [success, outputs] = compile_and_run(cenv);

  ASSERT_FALSE(success);
}

TEST_F(ForeignFunctionTest, RuntimeNativeDeepCopyOverride) {
  ParameterMap params;

  SingleCompileEnvironment cenv{
      std::filesystem::path(
          "test-autotuners/foreign_function/runtime-native-deepcopy.fal"),
      "RuntimeNativeDeepCopyTest", params, true};

  auto [success, outputs] = compile_and_run(cenv);

  ASSERT_TRUE(success);
  ASSERT_EQ(outputs.size(), 2u);

  EXPECT_EQ(std::get<int64_t>(outputs[0]), 25);

  EXPECT_EQ(std::get<int64_t>(outputs[1]), 10);
}

TEST_F(ForeignFunctionTest, RuntimeMixedStructDeepCopyOverride) {
  ParameterMap params;

  SingleCompileEnvironment cenv{
      std::filesystem::path(
          "test-autotuners/foreign_function/mixed-deepcopy.fal"),
      "MixedDeepCopyTest", params, true};

  auto [success, outputs] = compile_and_run(cenv);

  ASSERT_TRUE(success);
  ASSERT_EQ(outputs.size(), 2u);

  EXPECT_EQ(std::get<int64_t>(outputs[0]), 99);

  EXPECT_EQ(std::get<int64_t>(outputs[1]), 5);
}

TEST_F(ForeignFunctionTest, ModifyDeepCopy) {
  ParameterMap params;

  SingleCompileEnvironment cenv{
      std::filesystem::path(
          "test-autotuners/foreign_function/modify-falcon-deepcopy.fal"),
      "ModifyDeepCopyTest", params, true};

  auto [success, outputs] = compile_and_run(cenv);

  ASSERT_TRUE(success);
  ASSERT_EQ(outputs.size(), 2u);

  EXPECT_EQ(std::get<int64_t>(outputs[0]), 88);

  EXPECT_EQ(std::get<int64_t>(outputs[1]), 11);
}

TEST_F(ForeignFunctionTest, PureFalconStructDeepCopyFallback) {
  ParameterMap params;

  SingleCompileEnvironment cenv{
      std::filesystem::path(
          "test-autotuners/foreign_function/pure-falcon-deepcopy.fal"),
      "PureFalconDeepCopyTest", params, true};

  auto [success, outputs] = compile_and_run(cenv);

  ASSERT_TRUE(success);
  ASSERT_EQ(outputs.size(), 2u);

  EXPECT_EQ(std::get<int64_t>(outputs[0]), 88);

  EXPECT_EQ(std::get<int64_t>(outputs[1]), 11);
}

TEST_F(ForeignFunctionTest, NestedStructFFIEquality) {
  ParameterMap params;

  params["_a"] = static_cast<int64_t>(10);
  params["_b"] = static_cast<int64_t>(20);

  SingleCompileEnvironment cenv{
      std::filesystem::path(
          "test-autotuners/foreign_function/nested-struct-equality.fal"),
      "NestedStructFFIEquality", params, true};

  auto [success, outputs] = compile_and_run(cenv);

  ASSERT_TRUE(success);
  ASSERT_EQ(outputs.size(), 1u);

  EXPECT_TRUE(std::get<bool>(outputs[0]));
}

TEST_F(ForeignFunctionTest, NestedStructNoEqualityError) {
  ParameterMap params;

  params["_a"] = static_cast<int64_t>(10);
  params["_b"] = static_cast<int64_t>(20);

  SingleCompileEnvironment cenv{
      std::filesystem::path("test-autotuners/foreign_function/"
                            "nested-struct-no-equality-error.fal"),
      "NestedStructFFIError", params, false};

  auto [success, outputs] = compile_and_run(cenv);

  ASSERT_FALSE(success);
}

TEST_F(ForeignFunctionTest, NestedStructNonNativeFFIEquality) {
  ParameterMap params;

  params["_a"] = static_cast<int64_t>(10);
  params["_b"] = static_cast<int64_t>(20);

  SingleCompileEnvironment cenv{
      std::filesystem::path("test-autotuners/foreign_function/"
                            "nested-struct-non-native-equality.fal"),
      "NestedStructNonNativeFFIEquality", params, true};

  auto [success, outputs] = compile_and_run(cenv);

  ASSERT_TRUE(success);
  ASSERT_EQ(outputs.size(), 1u);

  EXPECT_TRUE(std::get<bool>(outputs[0]));
}

TEST_F(ForeignFunctionTest, NestedStructSplitEquality) {
  ParameterMap params;

  params["_a"] = static_cast<int64_t>(10);
  params["_b"] = static_cast<int64_t>(20);

  SingleCompileEnvironment cenv{
      std::filesystem::path("test-autotuners/foreign_function/"
                            "nested-struct-split-boxes.fal"),
      "NestedSplitFFIEquality", params, true};

  auto [success, outputs] = compile_and_run(cenv);

  ASSERT_TRUE(success);
  ASSERT_EQ(outputs.size(), 1u);

  EXPECT_TRUE(std::get<bool>(outputs[0]));
}

TEST_F(ForeignFunctionTest, NestedDeepCopy) {
  ParameterMap params;

  params["_a"] = static_cast<int64_t>(10);
  params["_b"] = static_cast<int64_t>(20);

  SingleCompileEnvironment cenv{
      std::filesystem::path(
          "test-autotuners/foreign_function/nested-struct-deepcopy.fal"),
      "NestedDeepCopy", params, true};

  auto [success, outputs] = compile_and_run(cenv);

  ASSERT_TRUE(success);
  ASSERT_EQ(outputs.size(), 1u);

  EXPECT_TRUE(std::get<bool>(outputs[0]));
}

TEST_F(ForeignFunctionTest, NestedDeepCopyMissingInnerCopy) {
  ParameterMap params;

  params["_a"] = static_cast<int64_t>(10);
  params["_b"] = static_cast<int64_t>(20);

  SingleCompileEnvironment cenv{
      std::filesystem::path(
          "test-autotuners/foreign_function/nested-struct-deepcopy-error.fal"),
      "NestedDeepCopyError", params, false};

  auto [success, outputs] = compile_and_run(cenv);

  ASSERT_FALSE(success);
}

TEST_F(ForeignFunctionTest, NestedDeepCopySplitLibraries) {
  ParameterMap params;

  params["_a"] = static_cast<int64_t>(10);
  params["_b"] = static_cast<int64_t>(20);

  SingleCompileEnvironment cenv{
      std::filesystem::path(
          "test-autotuners/foreign_function/nested-struct-deepcopy-split.fal"),
      "NestedDeepCopySplit", params, true};

  auto [success, outputs] = compile_and_run(cenv);

  ASSERT_TRUE(success);
  ASSERT_EQ(outputs.size(), 1u);

  EXPECT_TRUE(std::get<bool>(outputs[0]));
}
