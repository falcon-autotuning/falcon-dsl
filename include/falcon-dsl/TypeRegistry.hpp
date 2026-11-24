#pragma once

#include "falcon-atc/AST.hpp"
#include <boost/container_hash/hash.hpp>
#include <falcon-typing/PrimitiveTypes.hpp>
#include <map>
#include <memory>
#include <string>
#include <utility>

enum class ComparisonOp : uint8_t { Equals, NotEquals, LessThan, GreaterThan };

inline std::string_view primary_method_name(ComparisonOp op) {
  switch (op) {
  case ComparisonOp::Equals:
    return "Equal";
  case ComparisonOp::NotEquals:
    return "NotEqual";
  case ComparisonOp::LessThan:
    return "LessThan";
  case ComparisonOp::GreaterThan:
    return "GreaterThan";
  }

  throw std::runtime_error("Unknown comparison op");
}

inline std::string_view inverse_method_name(ComparisonOp op) {
  switch (op) {
  case ComparisonOp::Equals:
    return "NotEqual";
  case ComparisonOp::NotEquals:
    return "Equal";
  case ComparisonOp::LessThan:
    return "GreaterThan";
  case ComparisonOp::GreaterThan:
    return "LessThan";
  }

  throw std::runtime_error("Unknown comparison op");
}

namespace falcon::dsl {

class TypeRegistry {
public:
  // ── Built-in (non-FFI) default type methods ───────────────────────────────
  //
  // Both built-in methods (Array, etc.) and FFI struct methods use the same
  // TypeMethod signature:
  //
  //   FunctionResult fn(const RuntimeValue& obj, const ParameterMap& params)
  //
  // Built-in methods are registered here via create_default().
  // FFI struct methods are registered at runtime when a shared library is
  // loaded by AutotunerEngine::process_ff_import.

  void register_method(const std::string &type_name,
                       const std::string &method_name,
                       typing::TypeMethod method);

  [[nodiscard]] const typing::TypeMethod *
  lookup_method(const std::string &type_name,
                const std::string &method_name) const;

  // ── FFI struct methods ────────────────────────────────────────────────────
  void register_ffi_method(const std::string &type_name,
                           const std::string &method_name,
                           typing::TypeMethod method);

  [[nodiscard]] const typing::TypeMethod *
  lookup_ffi_method(const std::string &type_name,
                    const std::string &method_name) const;

  // ── Struct declarations ───────────────────────────────────────────────────
  void register_struct(const atc::StructDecl *decl) {
    struct_registry_[decl->name] = decl;
    if (!decl->module_name.empty()) {
      struct_registry_[decl->module_name + "::" + decl->name] = decl;
    }
  }

  [[nodiscard]] const atc::StructDecl *
  lookup_struct(const std::string &type_name) const {
    // First, direct lookup (handles plain names and already-monomorphized keys)
    auto it = struct_registry_.find(type_name);
    if (it != struct_registry_.end()) {
      return it->second;
    }

    // For a generic instantiation "Box<int>", try to find the base "Box"
    // and return the generic template (the evaluator will monomorphize at
    // runtime using the type_args in the TypeDescriptor).
    auto angle = type_name.find('<');
    if (angle != std::string::npos) {
      std::string base = type_name.substr(0, angle);
      auto base_it = struct_registry_.find(base);
      if (base_it != struct_registry_.end()) {
        return base_it->second;
      }
    }
    return nullptr;
  }

  // The key is the monomorphized name, e.g. "Box<int>".
  // Ownership is transferred to this registry.
  void register_monomorphized_struct(std::string mono_name,
                                     std::unique_ptr<atc::StructDecl> decl) {
    const atc::StructDecl *raw = decl.get();
    owned_monomorphized_.push_back(std::move(decl));
    struct_registry_[std::move(mono_name)] = raw;
  }

  [[nodiscard]] bool has_monomorphized(const std::string &mono_name) const {
    return struct_registry_.contains(mono_name);
  }
  static std::shared_ptr<TypeRegistry> create_default();

  [[nodiscard]] bool runtime_equals(const typing::RuntimeValue &lhs,
                                    const typing::RuntimeValue &rhs) const;
  [[nodiscard]] bool runtime_not_equals(const typing::RuntimeValue &lhs,
                                        const typing::RuntimeValue &rhs) const;

  [[nodiscard]] bool runtime_less_than(const typing::RuntimeValue &lhs,
                                       const typing::RuntimeValue &rhs) const;

  [[nodiscard]] bool
  runtime_greater_than(const typing::RuntimeValue &lhs,
                       const typing::RuntimeValue &rhs) const;

  [[nodiscard]] typing::RuntimeValue
  runtime_deep_copy(const typing::RuntimeValue &value) const;

private:
  struct CycleGuard;
  // Built-in methods for default types (Array, etc.)
  std::map<std::string, std::map<std::string, typing::TypeMethod>>
      type_methods_;

  // FFI methods for user-defined structs loaded from shared libraries
  std::map<std::string, std::map<std::string, typing::TypeMethod>> ffi_methods_;

  std::map<std::string, const atc::StructDecl *> struct_registry_;
  // Owns monomorphized StructDecl copies created at runtime.
  std::vector<std::unique_ptr<atc::StructDecl>> owned_monomorphized_;

  [[nodiscard]] std::optional<bool>
  invoke_comparison_override(const std::shared_ptr<typing::StructInstance> &lhs,
                             const std::shared_ptr<typing::StructInstance> &rhs,
                             const std::string &method_name) const;
  [[nodiscard]] std::optional<bool>
  invoke_comparison(const std::shared_ptr<typing::StructInstance> &lhs,
                    const std::shared_ptr<typing::StructInstance> &rhs,
                    ComparisonOp op) const;

  bool runtime_equals_impl(const typing::RuntimeValue &lhs,
                           const typing::RuntimeValue &rhs,
                           CycleGuard &guard) const;

  bool runtime_not_equals_impl(const typing::RuntimeValue &lhs,
                               const typing::RuntimeValue &rhs,
                               CycleGuard &guard) const;

  typing::RuntimeValue runtime_deep_copy_impl(const typing::RuntimeValue &value,
                                              CycleGuard &guard) const;

  bool values_equal(const std::shared_ptr<typing::ArrayValue> &lhs,
                    const std::shared_ptr<typing::ArrayValue> &rhs,
                    CycleGuard &guard) const;
  bool values_equal(const std::shared_ptr<typing::StructInstance> &lhs,
                    const std::shared_ptr<typing::StructInstance> &rhs,
                    CycleGuard &guard) const;
  bool values_equal(const std::shared_ptr<typing::TupleValue> &lhs,
                    const std::shared_ptr<typing::TupleValue> &rhs,
                    CycleGuard &guard) const;

  bool values_not_equal(const std::shared_ptr<typing::ArrayValue> &lhs,
                        const std::shared_ptr<typing::ArrayValue> &rhs,
                        CycleGuard &guard) const;
  bool values_not_equal(const std::shared_ptr<typing::StructInstance> &lhs,
                        const std::shared_ptr<typing::StructInstance> &rhs,
                        CycleGuard &guard) const;
  bool values_not_equal(const std::shared_ptr<typing::TupleValue> &lhs,
                        const std::shared_ptr<typing::TupleValue> &rhs,
                        CycleGuard &guard) const;
};

} // namespace falcon::dsl
