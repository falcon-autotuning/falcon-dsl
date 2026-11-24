#include <falcon-typing/FFIHelpers.hpp>

using namespace falcon::typing;
using namespace falcon::typing::ffi::wrapper;

struct GreaterNotEqualValue {
  int64_t value;
};

using GreaterNotEqualValueSP = std::shared_ptr<GreaterNotEqualValue>;

static void pack_value_obj(GreaterNotEqualValueSP value, FalconResultSlot *out,
                           int32_t *out_count) {

  out[0] = {};
  out[0].tag = FALCON_TYPE_OPAQUE;
  out[0].value.opaque.type_name = "GreaterNotEqualValue";
  out[0].value.opaque.ptr = new GreaterNotEqualValueSP(std::move(value));
  out[0].value.opaque.deleter = [](void *p) {
    delete static_cast<GreaterNotEqualValueSP *>(p);
  };

  *out_count = 1;
}

extern "C" {

void STRUCTGreaterNotEqualValueNew(const FalconParamEntry *params,
                                   int32_t count, FalconResultSlot *out,
                                   int32_t *out_count) {

  auto pm = unpack_params(params, count);

  auto value = std::make_shared<GreaterNotEqualValue>();
  value->value = std::get<int64_t>(pm.at("value"));

  pack_value_obj(value, out, out_count);
}

void STRUCTGreaterNotEqualValueNotEqual(const FalconParamEntry *params,
                                        int32_t count, FalconResultSlot *out,
                                        int32_t *out_count) {

  auto lhs = get_opaque<GreaterNotEqualValue>(params, count, "this");
  auto rhs = get_opaque<GreaterNotEqualValue>(params, count, "other");

  pack_single(lhs->value != rhs->value, out, out_count);
}

void STRUCTGreaterNotEqualValueGreaterThan(const FalconParamEntry *params,
                                           int32_t count, FalconResultSlot *out,
                                           int32_t *out_count) {

  auto lhs = get_opaque<GreaterNotEqualValue>(params, count, "this");
  auto rhs = get_opaque<GreaterNotEqualValue>(params, count, "other");

  pack_single(lhs->value > rhs->value, out, out_count);
}
}
