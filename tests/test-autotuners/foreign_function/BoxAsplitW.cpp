#define FALCON_IMPLEMENT_RUNTIME_HOOK

#include <falcon-typing/FFIHelpers.hpp>
#include <falcon-typing/FalconRuntime.hpp>

using namespace falcon::typing;
using namespace falcon::typing::ffi::wrapper;

struct ABox {
  int64_t value;
};

using ABoxSP = std::shared_ptr<ABox>;

static void pack_abox(ABoxSP box, FalconResultSlot *out, int32_t *out_count) {

  out[0] = {};
  out[0].tag = FALCON_TYPE_OPAQUE;
  out[0].value.opaque.type_name = "ABox";
  out[0].value.opaque.ptr = new ABoxSP(std::move(box));

  out[0].value.opaque.deleter = [](void *p) {
    delete static_cast<ABoxSP *>(p);
  };

  *out_count = 1;
}

extern "C" {

void STRUCTABoxNew(const FalconParamEntry *params, int32_t count,
                   FalconResultSlot *out, int32_t *out_count) {

  auto pm = unpack_params(params, count);

  auto box = std::make_shared<ABox>();

  box->value = std::get<int64_t>(pm.at("a"));

  pack_abox(box, out, out_count);
}

void STRUCTABoxDeepCopy(const FalconParamEntry *params, int32_t count,
                        FalconResultSlot *out, int32_t *out_count) {

  auto self = get_opaque<ABox>(params, count, "this");

  auto clone = std::make_shared<ABox>();

  clone->value = self->value;

  pack_abox(clone, out, out_count);
}

void STRUCTABoxEqual(const FalconParamEntry *params, int32_t count,
                     FalconResultSlot *out, int32_t *out_count) {

  auto lhs = get_opaque<ABox>(params, count, "this");

  auto rhs = get_opaque<ABox>(params, count, "other");

  pack_single(lhs->value == rhs->value, out, out_count);
}
}
