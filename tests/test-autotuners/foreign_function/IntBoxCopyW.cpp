#define FALCON_IMPLEMENT_RUNTIME_HOOK

#include <falcon-typing/FFIHelpers.hpp>
#include <falcon-typing/FalconRuntime.hpp>

using namespace falcon::typing;
using namespace falcon::typing::ffi::wrapper;

struct IntBox {
  RuntimeValue value;
};

using IntBoxSP = std::shared_ptr<IntBox>;

static void pack_box(IntBoxSP box, FalconResultSlot *out, int32_t *out_count) {
  out[0] = {};

  out[0].tag = FALCON_TYPE_OPAQUE;
  out[0].value.opaque.type_name = "IntBox";
  out[0].value.opaque.ptr = new IntBoxSP(std::move(box));

  out[0].value.opaque.deleter = [](void *p) {
    delete static_cast<IntBoxSP *>(p);
  };

  *out_count = 1;
}

extern "C" {

void STRUCTIntBoxNew(const FalconParamEntry *params, int32_t count,
                     FalconResultSlot *out, int32_t *out_count) {

  auto pm = unpack_params(params, count);

  auto box = std::make_shared<IntBox>();

  box->value = pm.at("value");

  pack_box(box, out, out_count);
}

void STRUCTIntBoxUpdateValue(const FalconParamEntry *params, int32_t count,
                             FalconResultSlot *, int32_t *out_count) {

  auto self = get_opaque<IntBox>(params, count, "this");

  auto pm = unpack_params(params, count);

  self->value = pm.at("value");

  *out_count = 0;
}

void STRUCTIntBoxDeepCopy(const FalconParamEntry *params, int32_t count,
                          FalconResultSlot *out, int32_t *out_count) {

  auto self = get_opaque<IntBox>(params, count, "this");

  auto copy = std::make_shared<IntBox>();

  copy->value = Runtime::deep_copy(self->value);

  pack_box(copy, out, out_count);
}

void STRUCTIntBoxEqual(const FalconParamEntry *params, int32_t count,
                       FalconResultSlot *out, int32_t *out_count) {

  auto lhs = get_opaque<IntBox>(params, count, "this");

  auto rhs = get_opaque<IntBox>(params, count, "other");

  bool equal = Runtime::equals(lhs->value, rhs->value);

  pack_single(equal, out, out_count);
}
}
