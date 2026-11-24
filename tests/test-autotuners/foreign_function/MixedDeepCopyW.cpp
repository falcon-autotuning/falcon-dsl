#define FALCON_IMPLEMENT_RUNTIME_HOOK

#include <falcon-typing/FFIHelpers.hpp>
#include <falcon-typing/FalconRuntime.hpp>

using namespace falcon::typing;
using namespace falcon::typing::ffi::wrapper;

extern "C" {

void STRUCTMixedPointDeepCopy(const FalconParamEntry *params, int32_t count,
                              FalconResultSlot *out, int32_t *out_count) {

  auto pm = unpack_params(params, count);

  auto self = std::get<std::shared_ptr<StructInstance>>(pm.at("this"));

  auto copy = std::make_shared<StructInstance>(self->type_name);

  copy->set_field("x", self->get_field("x"));

  copy->set_field("y", self->get_field("y"));

  pack_single(copy, out, out_count);
}
}
