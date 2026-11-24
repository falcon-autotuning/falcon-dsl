#include <falcon-typing/FFIHelpers.hpp>

using namespace falcon::typing;
using namespace falcon::typing::ffi::wrapper;

struct LessEqualValue {
  int64_t value;
};

using LessEqualValueSP = std::shared_ptr<LessEqualValue>;

static void pack_value_obj(LessEqualValueSP value, FalconResultSlot *out,
                           int32_t *out_count) {
  out[0] = {};
  out[0].tag = FALCON_TYPE_OPAQUE;
  out[0].value.opaque.type_name = "LessEqualValue";
  out[0].value.opaque.ptr = new LessEqualValueSP(std::move(value));
  out[0].value.opaque.deleter = [](void *p) {
    delete static_cast<LessEqualValueSP *>(p);
  };
  *out_count = 1;
}

extern "C" {

void STRUCTLessEqualValueNew(const FalconParamEntry *params, int32_t count,
                             FalconResultSlot *out, int32_t *out_count) {

  auto pm = unpack_params(params, count);

  auto value = std::make_shared<LessEqualValue>();
  value->value = std::get<int64_t>(pm.at("value"));

  pack_value_obj(value, out, out_count);
}

void STRUCTLessEqualValueEqual(const FalconParamEntry *params, int32_t count,
                               FalconResultSlot *out, int32_t *out_count) {

  auto lhs = get_opaque<LessEqualValue>(params, count, "this");
  auto rhs = get_opaque<LessEqualValue>(params, count, "other");

  pack_single(lhs->value == rhs->value, out, out_count);
}

void STRUCTLessEqualValueLessThan(const FalconParamEntry *params, int32_t count,
                                  FalconResultSlot *out, int32_t *out_count) {

  auto lhs = get_opaque<LessEqualValue>(params, count, "this");
  auto rhs = get_opaque<LessEqualValue>(params, count, "other");

  pack_single(lhs->value < rhs->value, out, out_count);
}
}
