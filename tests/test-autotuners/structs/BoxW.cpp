#include <falcon-typing/FFIHelpers.hpp>

using namespace falcon::typing;
using namespace falcon::typing::ffi::wrapper;

constexpr int FALCON_RESULT_SLOT_COUNT = 16;

extern "C" {

// ForeignBox<T>.Get() -> (T v)
void STRUCTForeignBoxGet(const FalconParamEntry *params, int32_t param_count,
                         FalconResultSlot *out, int32_t *out_count) {
  auto pm = unpack_params(params, param_count);
  auto self = std::get<std::shared_ptr<StructInstance>>(pm.at("this"));

  RuntimeValue value = self->get_field("value");
  pack_results(FunctionResult{value}, out, FALCON_RESULT_SLOT_COUNT, out_count);
}

} // extern "C"
