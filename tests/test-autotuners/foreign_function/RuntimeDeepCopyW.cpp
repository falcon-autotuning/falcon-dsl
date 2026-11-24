#define FALCON_IMPLEMENT_RUNTIME_HOOK
#include <falcon-typing/FFIHelpers.hpp>
#include <falcon-typing/FalconRuntime.hpp>

using namespace falcon::typing;
using namespace falcon::typing::ffi::wrapper;

struct NativeCounter {
  int64_t value;
};

using NativeCounterSP = std::shared_ptr<NativeCounter>;

static void pack_counter(NativeCounterSP value, FalconResultSlot *out,
                         int32_t *out_count) {

  out[0] = {};
  out[0].tag = FALCON_TYPE_OPAQUE;
  out[0].value.opaque.type_name = "NativeCounter";

  out[0].value.opaque.ptr = new NativeCounterSP(std::move(value));

  out[0].value.opaque.deleter = [](void *p) {
    delete static_cast<NativeCounterSP *>(p);
  };

  *out_count = 1;
}

extern "C" {

void STRUCTNativeCounterNew(const FalconParamEntry *params, int32_t count,
                            FalconResultSlot *out, int32_t *out_count) {

  auto pm = unpack_params(params, count);

  auto obj = std::make_shared<NativeCounter>();

  obj->value = std::get<int64_t>(pm.at("value"));

  pack_counter(obj, out, out_count);
}

void STRUCTNativeCounterDeepCopy(const FalconParamEntry *params, int32_t count,
                                 FalconResultSlot *out, int32_t *out_count) {

  auto self = get_opaque<NativeCounter>(params, count, "this");

  auto clone = std::make_shared<NativeCounter>();

  clone->value = self->value;

  pack_counter(clone, out, out_count);
}

void STRUCTNativeCounterSet(const FalconParamEntry *params, int32_t count,
                            FalconResultSlot *out, int32_t *out_count) {

  auto self = get_opaque<NativeCounter>(params, count, "this");

  auto pm = unpack_params(params, count);

  self->value = std::get<int64_t>(pm.at("value"));

  *out_count = 0;
}

void STRUCTNativeCounterGet(const FalconParamEntry *params, int32_t count,
                            FalconResultSlot *out, int32_t *out_count) {

  auto self = get_opaque<NativeCounter>(params, count, "this");

  pack_single(self->value, out, out_count);
}
}
