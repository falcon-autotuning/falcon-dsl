#define FALCON_IMPLEMENT_RUNTIME_HOOK

#include <falcon-typing/FFIHelpers.hpp>
#include <falcon-typing/FalconRuntime.hpp>

using namespace falcon::typing;
using namespace falcon::typing::ffi::wrapper;

extern "C" {

void Adjust(const FalconParamEntry *params, int32_t count,
            FalconResultSlot *out, int32_t *out_count) {

  auto pm = unpack_params(params, count);

  auto in = std::get<std::shared_ptr<StructInstance>>(pm.at("input"));

  auto out_value = falcon::typing::ffi::wrapper::Runtime::deep_copy(in);

  pack_single(out_value, out, out_count);
}
}
