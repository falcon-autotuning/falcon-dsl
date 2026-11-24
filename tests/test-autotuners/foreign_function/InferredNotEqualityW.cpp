#include <falcon-typing/FFIHelpers.hpp>
#include <falcon-typing/FalconRuntime.hpp>

using namespace falcon::typing;
using namespace falcon::typing::ffi::wrapper;

struct InferredValue {
  int64_t value;
};

using InferredValueSP = std::shared_ptr<InferredValue>;

static void pack_test_value(InferredValueSP value, FalconResultSlot *out,
                            int32_t *out_count) {
  out[0] = {};
  out[0].tag = FALCON_TYPE_OPAQUE;
  out[0].value.opaque.type_name = "InferredValue";
  out[0].value.opaque.ptr = new InferredValueSP(std::move(value));
  out[0].value.opaque.deleter = [](void *p) {
    delete static_cast<InferredValueSP *>(p);
  };
  *out_count = 1;
}

extern "C" {

void STRUCTInferredValueNew(const FalconParamEntry *params, int32_t count,
                            FalconResultSlot *out, int32_t *out_count) {

  auto pm = unpack_params(params, count);

  auto obj = std::make_shared<InferredValue>();
  obj->value = std::get<int64_t>(pm.at("value"));

  pack_test_value(obj, out, out_count);
}

void STRUCTInferredValueEqual(const FalconParamEntry *params, int32_t count,
                              FalconResultSlot *out, int32_t *out_count) {

  auto lhs = get_opaque<InferredValue>(params, count, "this");
  auto rhs = get_opaque<InferredValue>(params, count, "other");

  pack_single(lhs->value == rhs->value, out, out_count);
}
}
