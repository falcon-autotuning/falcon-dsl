#define FALCON_IMPLEMENT_RUNTIME_HOOK

#include <falcon-typing/FFIHelpers.hpp>
#include <falcon-typing/FalconRuntime.hpp>

using namespace falcon::typing;
using namespace falcon::typing::ffi::wrapper;
struct SingleBox {
  RuntimeValue value;
};

using SingleBoxSP = std::shared_ptr<SingleBox>;

struct PairBox {
  RuntimeValue key;
  RuntimeValue value;
};

using PairBoxSP = std::shared_ptr<PairBox>;

template <typename T>
static void pack_opaque(T value, const char *type_name, FalconResultSlot *out,
                        int32_t *out_count) {
  out[0] = {};
  out[0].tag = FALCON_TYPE_OPAQUE;
  out[0].value.opaque.type_name = type_name;
  out[0].value.opaque.ptr = new T(std::move(value));
  out[0].value.opaque.deleter = [](void *p) { delete static_cast<T *>(p); };
  *out_count = 1;
}

extern "C" {

void STRUCTPairBoxNew(const FalconParamEntry *params, int32_t count,
                      FalconResultSlot *out, int32_t *out_count) {
  auto pm = unpack_params(params, count);

  auto box = std::make_shared<PairBox>();

  box->key = pm.at("key");
  box->value = pm.at("value");

  pack_opaque(box, "PairBox", out, out_count);
}

void STRUCTPairBoxUpdate(const FalconParamEntry *params, int32_t count,
                         FalconResultSlot *, int32_t *out_count) {
  auto self = get_opaque<PairBox>(params, count, "this");
  auto pm = unpack_params(params, count);

  self->key = pm.at("key");
  self->value = pm.at("value");

  *out_count = 0;
}

void STRUCTPairBoxDeepCopy(const FalconParamEntry *params, int32_t count,
                           FalconResultSlot *out, int32_t *out_count) {
  auto self = get_opaque<PairBox>(params, count, "this");
  auto copy = std::make_shared<PairBox>();

  copy->key = Runtime::deep_copy(self->key);
  copy->value = Runtime::deep_copy(self->value);

  pack_opaque(copy, "PairBox", out, out_count);
}

void STRUCTPairBoxEqual(const FalconParamEntry *params, int32_t count,
                        FalconResultSlot *out, int32_t *out_count) {
  auto lhs = get_opaque<PairBox>(params, count, "this");
  auto rhs = get_opaque<PairBox>(params, count, "other");

  bool equal = Runtime::equals(lhs->key, rhs->key) &&
               Runtime::equals(lhs->value, rhs->value);

  pack_single(equal, out, out_count);
}

void STRUCTSingleBoxNew(const FalconParamEntry *params, int32_t count,
                        FalconResultSlot *out, int32_t *out_count) {
  auto pm = unpack_params(params, count);

  auto box = std::make_shared<SingleBox>();

  box->value = pm.at("value");

  pack_opaque(box, "SingleBox", out, out_count);
}

void STRUCTSingleBoxUpdate(const FalconParamEntry *params, int32_t count,
                           FalconResultSlot *, int32_t *out_count) {
  auto self = get_opaque<SingleBox>(params, count, "this");
  auto pm = unpack_params(params, count);

  self->value = pm.at("value");

  *out_count = 0;
}

void STRUCTSingleBoxDeepCopy(const FalconParamEntry *params, int32_t count,
                             FalconResultSlot *out, int32_t *out_count) {
  auto self = get_opaque<SingleBox>(params, count, "this");
  auto copy = std::make_shared<SingleBox>();

  copy->value = Runtime::deep_copy(self->value);

  pack_opaque(copy, "SingleBox", out, out_count);
}

void STRUCTSingleBoxEqual(const FalconParamEntry *params, int32_t count,
                          FalconResultSlot *out, int32_t *out_count) {
  auto lhs = get_opaque<SingleBox>(params, count, "this");
  auto rhs = get_opaque<SingleBox>(params, count, "other");

  bool equal = Runtime::equals(lhs->value, rhs->value);

  pack_single(equal, out, out_count);
}
}
