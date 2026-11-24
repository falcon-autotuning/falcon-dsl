#include <falcon-typing/FFIHelpers.hpp>

using namespace falcon::typing;
using namespace falcon::typing::ffi::wrapper;

struct TestValue {
  int64_t value;
};

using TestValueSP = std::shared_ptr<TestValue>;

static void pack_test_value(TestValueSP value, FalconResultSlot *out,
                            int32_t *out_count) {
  out[0] = {};
  out[0].tag = FALCON_TYPE_OPAQUE;
  out[0].value.opaque.type_name = "TestValue";
  out[0].value.opaque.ptr = new TestValueSP(std::move(value));
  out[0].value.opaque.deleter = [](void *p) {
    delete static_cast<TestValueSP *>(p);
  };
  *out_count = 1;
}

extern "C" {

void STRUCTTestValueNew(const FalconParamEntry *params, int32_t count,
                        FalconResultSlot *out, int32_t *out_count) {

  auto pm = unpack_params(params, count);

  auto obj = std::make_shared<TestValue>();
  obj->value = std::get<int64_t>(pm.at("value"));

  pack_test_value(obj, out, out_count);
}

void STRUCTTestValueEqual(const FalconParamEntry *params, int32_t count,
                          FalconResultSlot *out, int32_t *out_count) {

  auto lhs = get_opaque<TestValue>(params, count, "this");
  auto rhs = get_opaque<TestValue>(params, count, "other");

  pack_single(lhs->value == rhs->value, out, out_count);
}

void STRUCTTestValueNotEqual(const FalconParamEntry *params, int32_t count,
                             FalconResultSlot *out, int32_t *out_count) {

  auto lhs = get_opaque<TestValue>(params, count, "this");
  auto rhs = get_opaque<TestValue>(params, count, "other");

  pack_single(lhs->value != rhs->value, out, out_count);
}

void STRUCTTestValueLessThan(const FalconParamEntry *params, int32_t count,
                             FalconResultSlot *out, int32_t *out_count) {
  auto lhs = get_opaque<TestValue>(params, count, "this");
  auto rhs = get_opaque<TestValue>(params, count, "other");

  pack_single(lhs->value < rhs->value, out, out_count);
}

void STRUCTTestValueGreaterThan(const FalconParamEntry *params, int32_t count,
                                FalconResultSlot *out, int32_t *out_count) {
  auto lhs = get_opaque<TestValue>(params, count, "this");
  auto rhs = get_opaque<TestValue>(params, count, "other");

  pack_single(lhs->value > rhs->value, out, out_count);
}
}
