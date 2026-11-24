#define FALCON_IMPLEMENT_RUNTIME_HOOK

#include <falcon-typing/FFIHelpers.hpp>
#include <falcon-typing/FalconRuntime.hpp>

using namespace falcon::typing;
using namespace falcon::typing::ffi::wrapper;

struct BBox {
  int64_t b;
  RuntimeValue a;
};
struct ABox {
  int64_t value;
};

using ABoxSP = std::shared_ptr<ABox>;

using BBoxSP = std::shared_ptr<BBox>;

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

static void pack_bbox(BBoxSP box, FalconResultSlot *out, int32_t *out_count) {

  out[0] = {};
  out[0].tag = FALCON_TYPE_OPAQUE;
  out[0].value.opaque.type_name = "BBox";
  out[0].value.opaque.ptr = new BBoxSP(std::move(box));

  out[0].value.opaque.deleter = [](void *p) {
    delete static_cast<BBoxSP *>(p);
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

void STRUCTBBoxNew(const FalconParamEntry *params, int32_t count,
                   FalconResultSlot *out, int32_t *out_count) {

  auto pm = unpack_params(params, count);

  auto box = std::make_shared<BBox>();

  box->b = std::get<int64_t>(pm.at("b"));
  box->a = pm.at("a");

  pack_bbox(box, out, out_count);
}

void STRUCTBBoxUpdateB(const FalconParamEntry *params, int32_t count,
                       FalconResultSlot *, int32_t *out_count) {

  auto self = get_opaque<BBox>(params, count, "this");

  auto pm = unpack_params(params, count);

  self->b = std::get<int64_t>(pm.at("b"));

  *out_count = 0;
}

void STRUCTBBoxUpdateA(const FalconParamEntry *params, int32_t count,
                       FalconResultSlot *, int32_t *out_count) {

  auto self = get_opaque<BBox>(params, count, "this");

  auto pm = unpack_params(params, count);

  self->a = pm.at("a");

  *out_count = 0;
}
}
