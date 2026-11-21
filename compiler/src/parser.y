%require "3.8"
%language "c++"

%define api.namespace {falcon::atc}
%define api.parser.class {Parser}
%define api.value.type variant
%define api.token.constructor
%define api.token.prefix {TOK_}
%define parse.error verbose
%define parse.trace
%locations

%code requires {
  #include <string>
  #include <vector>
  #include <memory>
  #include <optional>
  #include <set>
  #include "falcon-atc/AST.hpp"
  #include "falcon-atc/ParseError.hpp"
}

%code {
  #include <iostream>
  
  namespace falcon::atc {
    Parser::symbol_type yylex();
  }
  
  std::unique_ptr<falcon::atc::Program> program_root;

  // ── Scope tracking for variable declarations ─────────────────────────────
  enum class ScopeKind {
    Autotuner,
    State,
    Struct,
    Routine,
    Loop,
    Conditional
  };

  struct VariableInfo {
      bool readable;
      bool writeable;
  };

  struct Scope {
      ScopeKind kind;
      std::unordered_map<std::string, VariableInfo> variables;
  };

  std::vector<Scope> lexical_scopes;
  void push_scope(ScopeKind kind) {
      lexical_scopes.push_back({kind, {}});
  }

  void pop_scope() {
      if (!lexical_scopes.empty()) {
          lexical_scopes.pop_back();
      }
  }

  void declare_variable(
      const std::string& name,
      bool readable = true,
      bool writeable = true)
  {
      if (!lexical_scopes.empty()) {
          lexical_scopes.back().variables.emplace(
              name,
              VariableInfo{
                  readable,
                  writeable
              });
      }
  }

  struct LookupResult {
      VariableInfo variable;
      ScopeKind defining_scope;
  };

  #include <optional>

  std::optional<LookupResult> lookup_variable(const std::string& name) {
      for (auto it = lexical_scopes.rbegin(); it != lexical_scopes.rend(); ++it) {
          auto found = it->variables.find(name);
          if (found != it->variables.end()) {
              return LookupResult{found->second, it->kind};
          }
      }
      return std::nullopt;
  }

  // struct_known_types is DEFINED in ParseError.cpp and DECLARED extern in
  // ParseError.hpp (which is included via %code requires above).
  // Do NOT redefine it here — that would shadow the cross-TU definition and
  // break the Compiler.cpp → struct_known_types linkage.

  // module_known_types is parser-session-only (not needed cross-TU).
  std::set<std::string> module_known_types;

// While parsing a struct body, this map holds the generic param names.
  // e.g. for  struct Box<T>  →  current_generic_params = {"T"}
  // This lets type_spec accept "T" (and other generic names) as valid types
  // inside the struct body without them being in struct_known_types.
  std::set<std::string> current_generic_params;

  // Mapping from base struct name → its generic param names.
  // Used when resolving  Box<int>  in type_spec to validate arg count.
  std::map<std::string, std::vector<std::string>> struct_generic_params_map;

  bool is_variable_declared(const std::string& name) {
      for (auto it = lexical_scopes.rbegin();
          it != lexical_scopes.rend();
          ++it) {
          if (it->variables.count(name)) {
              return true;
          }
      }
      return false;
  }

  bool is_redeclaration(const std::string& name)
  {
      if (lexical_scopes.empty()) {
          return false;
      }

      return lexical_scopes.back().variables.count(name) > 0;
  }

  bool is_visible_variable(const std::string& name)
  {
      for (auto it = lexical_scopes.rbegin();
          it != lexical_scopes.rend();
          ++it) {

          if (it->variables.count(name)) {
              return true;
          }
      }

      return false;
  }

  void set_stmt_location(falcon::atc::Stmt* stmt,
                         const falcon::atc::Parser::location_type& loc) {
    if (stmt) {
      stmt->filename = falcon::atc::current_filename;
      stmt->line     = loc.begin.line;
      stmt->column   = loc.begin.column;
    }
  }

  void set_expr_location(falcon::atc::Expr* expr, const falcon::atc::Parser::location_type& loc) {
    if (expr) {
      expr->filename = falcon::atc::current_filename;
      expr->line   = loc.begin.line;
      expr->column = loc.begin.column;
    }
  }

  std::unique_ptr<falcon::atc::AssignTarget>
  expr_to_assign_target(std::unique_ptr<falcon::atc::Expr> expr)
  {
      if (auto* var = dynamic_cast<falcon::atc::VarExpr*>(expr.get())) {
          return std::make_unique<falcon::atc::AssignTarget>(
              std::move(var->name));
      }

      if (auto* member = dynamic_cast<falcon::atc::MemberExpr*>(expr.get())) {
          return std::make_unique<falcon::atc::AssignTarget>(
              std::move(member->object),
              std::move(member->member));
      }

      throw std::runtime_error(
          "Expression is not assignable");
  }
}

// ============================================================================
// TOKEN DECLARATIONS
// ============================================================================

%token AUTOTUNER ROUTINE STATE IMPORT FFIMPORT START TERMINAL MOCK
%token IF ELIF ELSE STRUCT THIS FOR IN COLON
%token <std::string> IDENTIFIER STRING INTEGER DOUBLE
%token INT_KW FLOAT_KW BOOL_KW STRING_KW ERROR_KW ARRAY_KW
%token TRUE FALSE NIL
%token ARROW ASSIGN SEMICOLON COMMA DOT
%token LBRACE RBRACE LPAREN RPAREN LBRACKET RBRACKET
%token COLONCOLON
%token PLUS MINUS MUL DIV
%token EQ NE LL GG LE GE AND OR NOT

%type <std::unique_ptr<AutotunerDecl>> autotuner_decl
%type <std::vector<std::unique_ptr<ParamDecl>>> input_params output_params param_list state_params
%type <std::unique_ptr<ParamDecl>> param_decl
%type <std::unique_ptr<TypeDescriptor>> type_spec
%type <std::unique_ptr<FFImportDecl>> ffimport_decl
%type <std::vector<std::string>> identifier_list import_list import_stmt import_string_list ffimport_string_list
%type <std::vector<std::string>> generic_param_decl_list
%type <std::vector<std::unique_ptr<Stmt>>> autotuner_var_decls routine_body routine_body_stmts 
%type <std::unique_ptr<VarDeclStmt>> var_decl_stmt struct_field_decl
%type <std::string> entry_state qualified_name 
%type <std::vector<std::unique_ptr<Expr>>> entry_params
%type <std::vector<StateDecl>> state_list
%type <std::unique_ptr<StateDecl>> state_decl
%type <std::vector<std::unique_ptr<Stmt>>> stmt_list elif_chain
%type <std::unique_ptr<Stmt>> stmt struct_routine_stmt for_stmt
%type <std::string> loop_var
%type <std::pair<
    std::unique_ptr<Expr>,
    std::unique_ptr<Expr>>> for_range
%type <std::unique_ptr<Expr>> expr primary_expr postfix_expr
%type <std::vector<std::unique_ptr<Expr>>> expr_list 
%type <std::vector<CallArg>> call_arg_list 
%type <std::unique_ptr<CallArg>> call_arg
%type <std::unique_ptr<RoutineDecl>> routine_decl
%type <std::unique_ptr<StructDecl>>    struct_decl
%type <std::vector<VarDeclStmt>>       struct_field_list
%type <std::vector<RoutineDecl>> struct_routine_list
%type <std::vector<std::unique_ptr<Expr>>> assignment_lhs
%type <std::vector<std::unique_ptr<TypeDescriptor>>> type_arg_list
%type <std::vector<std::unique_ptr<ParamDecl>>>
    param_list_with_optional_trailing_comma
%type <std::vector<std::unique_ptr<TypeDescriptor>>>
    type_arg_list_with_optional_trailing_comma
%type <std::vector<std::unique_ptr<Expr>>>
    expr_list_with_optional_trailing_comma
%type <std::vector<CallArg>>
    call_arg_list_with_optional_trailing_comma
%type <std::vector<std::string>>
    identifier_list_with_optional_trailing_comma
%type <std::vector<std::variant<
    std::unique_ptr<StructDecl>,
    std::unique_ptr<RoutineDecl>,
    std::unique_ptr<AutotunerDecl>,
    std::unique_ptr<FFImportDecl>
>> > program_items
%type <bool> opt_mock 

%type <std::variant<
    std::unique_ptr<StructDecl>,
    std::unique_ptr<RoutineDecl>,
    std::unique_ptr<AutotunerDecl>,
    std::unique_ptr<FFImportDecl>
>> program_item

%left OR
%left AND
%left EQ NE 
%left LL GG LE GE
%left PLUS MINUS
%left MUL DIV
%right NOT UMINUS
%left DOT LBRACKET LPAREN COLONCOLON

%start program

%%

// ============================================================================
// PROGRAM STRUCTURE
// ============================================================================

program
    : import_list[imps] program_items[items]
      {
        lexical_scopes.clear();
        auto prog = std::make_unique<Program>();
        prog->imports = std::move($imps);
        for (auto& item : $items) {
          if (std::holds_alternative<std::unique_ptr<StructDecl>>(item)) {
            prog->structs.push_back(std::move(*std::get<std::unique_ptr<StructDecl>>(item)));
          } else if (std::holds_alternative<std::unique_ptr<RoutineDecl>>(item)) {
            prog->routines.push_back(std::move(*std::get<std::unique_ptr<RoutineDecl>>(item)));
          } else if (std::holds_alternative<std::unique_ptr<AutotunerDecl>>(item)) {
            prog->autotuners.push_back(std::move(*std::get<std::unique_ptr<AutotunerDecl>>(item)));
          } else if (std::holds_alternative<std::unique_ptr<FFImportDecl>>(item)) {
            prog->ff_imports.push_back(std::move(*std::get<std::unique_ptr<FFImportDecl>>(item)));
          }
        }
        prog->build_indexes();
        program_root = std::move(prog);
        // NOTE: do NOT move program_root into $result — parse_file() reads
        // program_root directly via the extern global.  Moving it into $result
        // (which bison discards for the start symbol) would leave program_root null.
      }
    ;

program_items[result]
    : %empty
      { $result = std::vector<std::variant<std::unique_ptr<StructDecl>,std::unique_ptr<RoutineDecl>,std::unique_ptr<AutotunerDecl>,std::unique_ptr<FFImportDecl>>>(); }
    | program_items[prev] program_item[next]
      {
        $result = std::move($prev);
        $result.push_back(std::move($next));
      }
    ;

program_item[result]
    : struct_decl[s]      { $result = std::move($s); }
    | routine_decl[r]     { $result = std::move($r); }
    | autotuner_decl[a]   { $result = std::move($a); }
    | ffimport_decl[f]    { $result = std::move($f); }
    ;

// ============================================================================
// QUALIFIED NAME  (Module::symbol  OR  plain  symbol)
// ============================================================================
// A qualified_name produces a single string "Module::symbol" that the runtime
// can split on "::" to resolve imports.  Plain IDENTIFIER is also a valid
// qualified_name (no module prefix).

qualified_name[result]
    : IDENTIFIER[ns] COLONCOLON IDENTIFIER[sym]
      { $result = $ns + "::" + $sym; }
    | IDENTIFIER[name]
      { $result = $name; }
    ;

// ============================================================================
// STRUCT DECLARATIONS
// ============================================================================
generic_param_decl_list[result]
    : %empty
      { $result = std::vector<std::string>(); }
    | LL IDENTIFIER[first] GG
      {
        $result = std::vector<std::string>();
        $result.push_back(std::move($first));
      }
    | LL IDENTIFIER[first] COMMA identifier_list_with_optional_trailing_comma[rest] GG
      {
        $result = std::vector<std::string>();
        $result.push_back(std::move($first));
        for (auto& id : $rest) $result.push_back(std::move(id));
      }
    ;

struct_decl[result]
    : STRUCT IDENTIFIER[name] generic_param_decl_list[gparams] LBRACE
      {
        push_scope(ScopeKind::Struct);
        // Register the bare name so we can look up its generic params later.
        struct_known_types.insert($name);
        // Save generic param names so type_spec can accept them inside the body.
        current_generic_params.clear();
        for (const auto& gp : $gparams) current_generic_params.insert(gp);
        // Record generic param names for future instantiation validation.
        struct_generic_params_map[$name] = $gparams;
      }
      struct_field_list[fields] struct_routine_list[routines] RBRACE
      {
        current_generic_params.clear();
        pop_scope();
        $result = std::make_unique<StructDecl>(
            std::move($name),
            std::move($gparams),
            std::move($fields),
            std::move($routines));
      }
    ;

// ---------------------------------------------------------------------------
// Struct field declarations
// ---------------------------------------------------------------------------

struct_field_list[result]
    : %empty
      { $result = std::vector<VarDeclStmt>(); }
    | struct_field_list[prev] struct_field_decl[next]
      {
        $result = std::move($prev);
        $result.push_back(std::move(*$next));
      }
    ;

struct_field_decl[result]
    : type_spec[type] IDENTIFIER[name] SEMICOLON
      {
        declare_variable($name);
        auto decl = std::make_unique<VarDeclStmt>(
            std::move(*$type), std::move($name), std::nullopt);
        decl->decl_scope = VarDeclStmt::DeclScope::StructField;
        $result = std::move(decl);
      }
    | type_spec[type] IDENTIFIER[name] ASSIGN expr[default_val] SEMICOLON
      {
        declare_variable($name);
        auto decl = std::make_unique<VarDeclStmt>(
            std::move(*$type), std::move($name),
            std::make_optional(std::move($default_val)));
        decl->decl_scope = VarDeclStmt::DeclScope::StructField;
        $result = std::move(decl);
      }
    ;

// ---------------------------------------------------------------------------
// Struct routine declarations
// ---------------------------------------------------------------------------

struct_routine_list[result]
    : %empty
      { $result = std::vector<RoutineDecl>(); }
    | struct_routine_list[prev] 
      routine_decl[next]
      {
        $result = std::move($prev);
        $result.push_back(std::move(*$next));
      }
    ;

// ---------------------------------------------------------------------------
// Body of a struct routine
// ---------------------------------------------------------------------------

routine_body[result]
    : LBRACE routine_body_stmts[stmts] RBRACE
      { $result = std::move($stmts); }
    | %empty
      { $result = std::vector<std::unique_ptr<Stmt>>(); }
    ;

routine_body_stmts[result]
    : %empty
      { $result = std::vector<std::unique_ptr<Stmt>>(); }
    | routine_body_stmts[prev] struct_routine_stmt[next]
      {
        $result = std::move($prev);
        $result.push_back(std::move($next));
      }
    ;

loop_var[result]
    : FOR IDENTIFIER[var] IN
      {
          if (is_visible_variable($var)) {
              error(@var,
                    "Loop variable shadows existing variable: " + $var);
              YYABORT;
          }
          push_scope(ScopeKind::Loop);
          declare_variable($var, true, false);
          $result = std::move($var);
      }
    ;

for_range[result]
    : expr[end]
      {
          $result = {
              std::make_unique<LiteralExpr>(int64_t(0)),
              std::move($end)
          };
      }
    | expr[start] COLON expr[end]
      {
          $result = {
              std::move($start),
              std::move($end)
          };
      }
    ;

for_stmt[result]
    : loop_var[var]
      for_range[range]
      LBRACE routine_body_stmts[body] RBRACE
      {
          pop_scope();

          auto stmt =
              std::make_unique<ForStmt>(
                  std::move($var),
                  std::move($range.first),
                  std::move($range.second),
                  std::move($body));

          set_stmt_location(stmt.get(), @1);

          $result = std::move(stmt);
      }

struct_routine_stmt[result]
    : for_stmt[f]
          {
              $result = std::move($f);
          }
    | assignment_lhs[lhs] ASSIGN expr[val] SEMICOLON
      {
        std::vector<AssignTarget> targets;
        for (auto& expr : $lhs) {
            auto target =
                expr_to_assign_target(
                    std::move(expr));
            targets.push_back(
                std::move(*target));
        }
        $result =
            std::make_unique<AssignStmt>(
                std::move(targets),
                std::move($val));
        set_stmt_location($result.get(), @lhs);
      }
    | var_decl_stmt[vd]
      {
        $result = std::move($vd);
        set_stmt_location($result.get(), @vd);
      }
    | IF LPAREN expr[cond] RPAREN LBRACE
      {
        push_scope(ScopeKind::Conditional);
      }
      routine_body[then_b] 
      {
        pop_scope();
      }
      RBRACE elif_chain[else_b]
      {
        $result = std::make_unique<IfStmt>(
            std::move($cond),
            std::move($then_b),
            std::move($else_b));
        set_stmt_location($result.get(), @IF);
      }
    | expr[e] SEMICOLON
      {
        $result = std::make_unique<ExprStmt>(std::move($e));
        set_stmt_location($result.get(), @e);
      }
    ;

// ============================================================================
// IMPORT DECLARATION
// ============================================================================

import_stmt[result]
    : IMPORT STRING[path] SEMICOLON 
      { $result = std::vector<std::string>{std::move($path)}; }
    | IMPORT LPAREN import_string_list[paths] RPAREN 
      { $result = std::move($paths); }
    ;

import_string_list[result]
    : STRING[first] 
      { $result = std::vector<std::string>{std::move($first)}; }
    | import_string_list[existing] STRING[next] 
      { 
        $result = std::move($existing); 
        $result.push_back(std::move($next)); 
      }
    ;

ffimport_decl[result]
    : FFIMPORT STRING[wrapper]
      LPAREN ffimport_string_list[imports] RPAREN
      LPAREN ffimport_string_list[libs] RPAREN
      {
        $result = std::make_unique<FFImportDecl>(
          std::move($wrapper),
          std::move($imports),
          std::move($libs)
        );
      }
    ;

ffimport_string_list[result]
    : STRING[first]
      { $result = std::vector<std::string>{std::move($first)}; }
    | ffimport_string_list[existing] STRING[next]
      {
        $result = std::move($existing);
        $result.push_back(std::move($next));
      }
    ;

import_list[result]
    : %empty
       { $result = std::vector<std::string>(); }
    | import_list[existing] import_stmt[next] 
      { 
        $result = std::move($existing);
        $result.insert($result.end(),
                       std::make_move_iterator($next.begin()),
                       std::make_move_iterator($next.end()));
      }
    ;

// ============================================================================
// AUTOTUNER DECLARATION
// ============================================================================

opt_mock[is_mock]
    : MOCK   { $is_mock = true; }
    | %empty { $is_mock = false; }
    ;

autotuner_decl[result]
    : opt_mock[is_mock] AUTOTUNER IDENTIFIER[name]
      {
        push_scope(ScopeKind::Autotuner);
      }
      input_params[inputs]
      ARROW
      output_params[outputs]
      LBRACE
        autotuner_var_decls[vars]
        entry_state[entry] entry_params[params] SEMICOLON
        state_list[states]
      RBRACE
      {
        pop_scope();
        $result = std::make_unique<AutotunerDecl>(
          std::move($name),
          std::move($inputs),
          std::move($outputs),
          std::move($vars),
          std::move($entry),
          std::move($params),
          std::move($states),
          $is_mock
        );
      }
    ;


// ============================================================================
// PARAMETERS
// ============================================================================

param_list[result]
    : param_decl[first]
      {
        $result = std::vector<std::unique_ptr<ParamDecl>>();
        $result.push_back(std::move($first));
      }
    | param_list[existing] COMMA param_decl[next]
      {
        $result = std::move($existing);
        $result.push_back(std::move($next));
      }
    ;

param_list_with_optional_trailing_comma[result]
    : param_list[params]
      {
          $result = std::move($params);
      }
    | param_list[params] COMMA
      {
          $result = std::move($params);
      }
    ;

input_params[result]
    : LPAREN param_list_with_optional_trailing_comma[params] RPAREN
      {
        for (const auto& p : $params) {
          declare_variable(p->name, true, false);
        }
        $result = std::move($params);
      }
    | LPAREN RPAREN
      { $result = std::vector<std::unique_ptr<ParamDecl>>(); }
    | %empty
      { $result = std::vector<std::unique_ptr<ParamDecl>>(); }
    ;

output_params[result]
    : LPAREN param_list_with_optional_trailing_comma[params] RPAREN
      {
        for (const auto& p : $params) {
          declare_variable(p->name);
        }
        $result = std::move($params);
      }
    | LPAREN RPAREN
      { $result = std::vector<std::unique_ptr<ParamDecl>>(); }
    ;


param_decl[result]
    : type_spec[type] IDENTIFIER[name]
      {
        $result = std::make_unique<ParamDecl>(std::move(*$type), std::move($name));
      }
    | type_spec[type] IDENTIFIER[name] ASSIGN expr[default_val]
      {
        $result = std::make_unique<ParamDecl>(
            std::move(*$type), std::move($name),
            std::make_optional(std::move($default_val)));
      }
    ;

// ============================================================================
// TYPES
// ============================================================================
type_arg_list[result]
    : type_spec[first]
      {
        $result = std::vector<std::unique_ptr<TypeDescriptor>>();
        $result.push_back(std::move($first));
      }
    | type_arg_list[existing] COMMA type_spec[next]
      {
        $result = std::move($existing);
        $result.push_back(std::move($next));
      }
    ;

type_arg_list_with_optional_trailing_comma[result]
    : type_arg_list[args]
      {
          $result = std::move($args);
      }
    | type_arg_list[args] COMMA
      {
          $result = std::move($args);
      }
    ;

type_spec[result]
    : INT_KW
      { $result = std::make_unique<TypeDescriptor>(ParamType::Int); }
    | FLOAT_KW
      { $result = std::make_unique<TypeDescriptor>(ParamType::Float); }
    | BOOL_KW
      { $result = std::make_unique<TypeDescriptor>(ParamType::Bool); }
    | STRING_KW
      { $result = std::make_unique<TypeDescriptor>(ParamType::String); }
    | ERROR_KW
      { $result = std::make_unique<TypeDescriptor>(ParamType::Error, "Error"); }
    | qualified_name[name] LL type_arg_list_with_optional_trailing_comma[args] GG
      {
        // The base name must be a known generic struct.
        std::string base = $name;
        if (struct_known_types.count(base) == 0 &&
            module_known_types.count(base) == 0) {
          error(@name, "Unknown struct type '" + base + "'");
          YYABORT;
        }
        // Validate arity if we have param info.
        auto it = struct_generic_params_map.find(base);
        if (it != struct_generic_params_map.end()) {
          size_t expected = it->second.size();
          if ($args.size() != expected) {
            error(@name, "Struct '" + base + "' expects " +
                  std::to_string(expected) + " type argument(s) but got " +
                  std::to_string($args.size()));
            YYABORT;
          }
        }
        std::vector<TypeDescriptor> arg_descs;
        for (auto& a : $args) arg_descs.push_back(std::move(*a));
        $result = std::make_unique<TypeDescriptor>(
            TypeDescriptor::make_generic_struct(std::move(base), std::move(arg_descs)));
      }
    // ── Plain identifier: either a non-generic struct or a generic type param
    //    used inside the struct body itself (e.g. T, U).
    | qualified_name[name]
      {
        // Accept generic type parameter names (T, U, ...) used inside a struct body.
        if (current_generic_params.count($name) > 0) {
          // This is a generic placeholder. We represent it as a Struct type
          // with the param name so the evaluator can substitute it at runtime.
          $result = std::make_unique<TypeDescriptor>(
              TypeDescriptor::make_struct($name));
        } else if (struct_known_types.count($name) > 0 ||
                   module_known_types.count($name) > 0) {
          $result = std::make_unique<TypeDescriptor>(
              TypeDescriptor::make_struct($name));
        } else {
          error(@name, "Unknown type '" + $name + "' — "
                "did you forget to declare a struct with this name before use?");
          YYABORT;
        }
      }
    ;

// ============================================================================
// AUTOTUNER COMPONENTS
// ============================================================================

identifier_list[result]
    : qualified_name[first_id]
      {
        $result = std::vector<std::string>();
        $result.push_back(std::move($first_id));
      }
    | identifier_list[existing_ids] COMMA qualified_name[next_id]
      {
        $result = std::move($existing_ids);
        $result.push_back(std::move($next_id));
      }
    | %empty
      { $result = std::vector<std::string>(); }
    ;

identifier_list_with_optional_trailing_comma[result]
    : identifier_list[list]
      {
          $result = std::move($list);
      }
    | identifier_list[list] COMMA
      {
          $result = std::move($list);
      }
    ;

autotuner_var_decls[result]
    : %empty
      { $result = std::vector<std::unique_ptr<Stmt>>(); }
    | autotuner_var_decls[existing_vars] stmt[next_var]
      {
        $result = std::move($existing_vars);
        $result.push_back(std::move($next_var));
      }
    ;

entry_state[result]
    : START ARROW IDENTIFIER[state_name]
      { $result = std::move($state_name); }
    ;

entry_params[result]
    : LPAREN expr_list_with_optional_trailing_comma[params] RPAREN
      { $result = std::move($params); }
    | %empty
      { $result = std::vector<std::unique_ptr<Expr>>(); }
    ;

// ============================================================================
// STATES
// ============================================================================

state_list[result]
    : state_decl[first_state]
      {
        $result = std::vector<StateDecl>();
        $result.push_back(std::move(*$first_state));
      }
    | state_list[existing_states] state_decl[next_state]
      {
        $result = std::move($existing_states);
        $result.push_back(std::move(*$next_state));
      }
    ;

state_decl[result]
    : STATE IDENTIFIER[name]
      {
          push_scope(ScopeKind::State);
      }
      state_params[params] LBRACE stmt_list[stmts] RBRACE
      {
        pop_scope();
        $result = std::make_unique<StateDecl>(
          std::move($name),
          std::move($params),
          std::move($stmts)
        );
      }
    ;

state_params[result]
    : LPAREN param_list_with_optional_trailing_comma[params] RPAREN
      {
        for (const auto& p : $params) {
          declare_variable(p->name, true, false);
        }
        $result = std::move($params);
      }
    | LPAREN RPAREN
      { $result = std::vector<std::unique_ptr<ParamDecl>>(); }
    | %empty
      { $result = std::vector<std::unique_ptr<ParamDecl>>(); }
    ;

// ============================================================================
// STATEMENTS
// ============================================================================

stmt_list[result]
    : %empty
      { $result = std::vector<std::unique_ptr<Stmt>>(); }
    | stmt_list[existing] stmt[next]
      {
        $result = std::move($existing);
        $result.push_back(std::move($next));
      }
    ;

assignment_lhs[result]
    : expr[first]
      {
          $result = std::vector<std::unique_ptr<Expr>>();
          $result.push_back(std::move($first));
      }
    | assignment_lhs[existing] COMMA expr[next]
      {
          $result = std::move($existing);
          $result.push_back(std::move($next));
      }
    ;

stmt[result]
    : ARROW IDENTIFIER[target] SEMICOLON
      {
        $result = std::make_unique<TransitionStmt>(
            std::move($target),
            std::vector<std::unique_ptr<Expr>>());
        set_stmt_location($result.get(), @ARROW);
      }
    | ARROW IDENTIFIER[target] LPAREN expr_list_with_optional_trailing_comma[args] RPAREN SEMICOLON
      {
        $result = std::make_unique<TransitionStmt>(
            std::move($target),
            std::move($args));
        set_stmt_location($result.get(), @ARROW);
      }
    | TERMINAL SEMICOLON
      {
        $result = std::make_unique<TerminalStmt>();
        set_stmt_location($result.get(), @TERMINAL);
      }
    | assignment_lhs[lhs] ASSIGN expr[val] SEMICOLON
      {
        std::vector<AssignTarget> targets;
        for (auto& expr : $lhs) {
            auto target =
                expr_to_assign_target(
                    std::move(expr));
            targets.push_back(
                std::move(*target));
        }
        $result =
            std::make_unique<AssignStmt>(
                std::move(targets),
                std::move($val));
        set_stmt_location($result.get(), @lhs);
      }
    | var_decl_stmt[vd]
      {
        $result = std::move($vd);
        set_stmt_location($result.get(), @vd);
      }

    | IF LPAREN expr[cond] RPAREN LBRACE
      {
        push_scope(ScopeKind::Conditional);
      }
      stmt_list[then_b] 
      {
        pop_scope();
      }
      RBRACE elif_chain[else_b]
      {
        $result = std::make_unique<IfStmt>(
            std::move($cond),
            std::move($then_b),
            std::move($else_b));
        set_stmt_location($result.get(), @IF);
      }
    | expr[e] SEMICOLON
      {
        $result = std::make_unique<ExprStmt>(std::move($e));
        set_stmt_location($result.get(), @e);
      }
    ;

elif_chain[result]
    : %empty
      { $result = std::vector<std::unique_ptr<Stmt>>(); }
    | ELSE LBRACE 
      {
        push_scope(ScopeKind::Conditional);
      }
      stmt_list[else_b] 
      {
        pop_scope();
      }
      RBRACE
      { $result = std::move($else_b); }
    | ELIF LPAREN expr[cond] RPAREN LBRACE 
      {
        push_scope(ScopeKind::Conditional);
      }
      stmt_list[then_b] 
      {
        pop_scope();
      }
      RBRACE elif_chain[rest]
      {
        $result = std::vector<std::unique_ptr<Stmt>>();
        $result.push_back(std::make_unique<IfStmt>(
            std::move($cond),
            std::move($then_b),
            std::move($rest)));
      }
    ;

var_decl_stmt[result]
    : type_spec[type] IDENTIFIER[name] SEMICOLON
      {
        if (is_redeclaration($name)) {
            error(@name, "Redeclaration of variable: " + $name);
            YYABORT;
        }

        declare_variable($name);
        $result = std::make_unique<VarDeclStmt>(
          std::move(*$type), 
          std::move($name), 
          std::nullopt);
        set_stmt_location($result.get(), @name);
      }
    | type_spec[type] IDENTIFIER[name] ASSIGN expr[init] SEMICOLON
      {
        if (is_redeclaration($name)) {
            error(@name, "Redeclaration of variable: " + $name);
            YYABORT;
        }

        declare_variable($name);
        $result = std::make_unique<VarDeclStmt>(
            std::move(*$type), std::move($name),
            std::make_optional(std::move($init)));
        set_stmt_location($result.get(), @name);
      }
    ;

// ============================================================================
// ROUTINE DECLARATION
// ============================================================================

routine_decl[result]
    : ROUTINE IDENTIFIER[name]
      {
        push_scope(ScopeKind::Routine);
      }
      input_params[inputs] ARROW output_params[outputs] routine_body[body]
      {
        pop_scope();
        $result = std::make_unique<RoutineDecl>(
            std::move($name),
            std::move($inputs),
            std::move($outputs),
            std::move($body));
      }
    ;

// ============================================================================
// EXPRESSIONS
// ============================================================================

expr[result]
    : postfix_expr[e]     { $result = std::move($e); }
    | expr[l] PLUS  expr[r] { $result = std::make_unique<BinaryExpr>("+",  std::move($l), std::move($r)); set_expr_location($result.get(), @PLUS); }
    | expr[l] MINUS expr[r] { $result = std::make_unique<BinaryExpr>("-",  std::move($l), std::move($r)); set_expr_location($result.get(), @MINUS); }
    | expr[l] MUL   expr[r] { $result = std::make_unique<BinaryExpr>("*",  std::move($l), std::move($r)); set_expr_location($result.get(), @MUL); }
    | expr[l] DIV   expr[r] { $result = std::make_unique<BinaryExpr>("/",  std::move($l), std::move($r)); set_expr_location($result.get(), @DIV); }
    | expr[l] EQ    expr[r] { $result = std::make_unique<BinaryExpr>("==", std::move($l), std::move($r)); set_expr_location($result.get(), @EQ); }
    | expr[l] NE    expr[r] { $result = std::make_unique<BinaryExpr>("!=", std::move($l), std::move($r)); set_expr_location($result.get(), @NE); }
    | expr[l] LL    expr[r] { $result = std::make_unique<BinaryExpr>("<",  std::move($l), std::move($r)); set_expr_location($result.get(), @LL); }
    | expr[l] GG    expr[r] { $result = std::make_unique<BinaryExpr>(">",  std::move($l), std::move($r)); set_expr_location($result.get(), @GG); }
    | expr[l] LE    expr[r] { $result = std::make_unique<BinaryExpr>("<=", std::move($l), std::move($r)); set_expr_location($result.get(), @LE); }
    | expr[l] GE    expr[r] { $result = std::make_unique<BinaryExpr>(">=", std::move($l), std::move($r)); set_expr_location($result.get(), @GE); }
    | expr[l] AND   expr[r] { $result = std::make_unique<BinaryExpr>("&&", std::move($l), std::move($r)); set_expr_location($result.get(), @AND); }
    | expr[l] OR    expr[r] { $result = std::make_unique<BinaryExpr>("||", std::move($l), std::move($r)); set_expr_location($result.get(), @OR); }
    | NOT  expr[e]  { $result = std::make_unique<UnaryExpr>("!",  std::move($e));  set_expr_location($result.get(), @NOT); }
    | MINUS expr[e] %prec UMINUS { $result = std::make_unique<UnaryExpr>("-", std::move($e)); set_expr_location($result.get(), @MINUS); }
    | LPAREN expr[e] RPAREN { $result = std::move($e); }
    ;

primary_expr[result]
    : INTEGER[v]  { $result = std::make_unique<LiteralExpr>(static_cast<int64_t>(std::stoll($v)));
                    set_expr_location($result.get(), @v); }
    | DOUBLE[v]   { $result = std::make_unique<LiteralExpr>(std::stod($v));
                    set_expr_location($result.get(), @v); }
    | STRING[v]   { $result = std::make_unique<LiteralExpr>($v);
                    set_expr_location($result.get(), @v); }
    | TRUE        { $result = std::make_unique<LiteralExpr>(true);
                    set_expr_location($result.get(), @TRUE); }
    | FALSE       { $result = std::make_unique<LiteralExpr>(false);
                    set_expr_location($result.get(), @FALSE); }
    | NIL         { $result = std::make_unique<NilLiteralExpr>();
                    set_expr_location($result.get(), @NIL); }
    | THIS        { $result = std::make_unique<VarExpr>("this");
                    set_expr_location($result.get(), @THIS); }
    | IDENTIFIER[name] { $result = std::make_unique<VarExpr>($name);
                          set_expr_location($result.get(), @name); }
    | IDENTIFIER[ns] COLONCOLON IDENTIFIER[sym]
      { $result = std::make_unique<VarExpr>($ns + "::" + $sym);
        set_expr_location($result.get(), @ns); }
    ;

postfix_expr[result]
    // Member access: expr.field
    : postfix_expr[obj] DOT IDENTIFIER[field]
      { $result = std::make_unique<MemberExpr>(std::move($obj), std::move($field));
        set_expr_location($result.get(), @field); }
    // Method call: expr.method(args)
    | postfix_expr[obj] DOT IDENTIFIER[method] 
      LPAREN
        call_arg_list_with_optional_trailing_comma[args] 
      RPAREN
      {
        std::vector<std::unique_ptr<Expr>> arg_exprs;
        arg_exprs.reserve($args.size());
        for (auto& a : $args) { arg_exprs.push_back(std::move(a.value)); }
        $result = std::make_unique<MethodCallExpr>(std::move($obj), std::move($method), std::move(arg_exprs));
        set_expr_location($result.get(), @method);
      }
    // Index: expr[idx]
    | postfix_expr[obj] LBRACKET expr[idx] RBRACKET
      { $result = std::make_unique<IndexExpr>(std::move($obj), std::move($idx));
        set_expr_location($result.get(), @LBRACKET); }
    // Plain function call: name(args)
    | IDENTIFIER[name] LPAREN call_arg_list_with_optional_trailing_comma[args] RPAREN
      {
        $result = std::make_unique<CallExpr>(std::move($name), std::move($args));
        set_expr_location($result.get(), @name);
      }
    // Module-qualified function call: Module::symbol(args)
    | IDENTIFIER[ns] COLONCOLON IDENTIFIER[sym] LPAREN call_arg_list_with_optional_trailing_comma[args] RPAREN
      {
        // Encode as a CallExpr with the qualified name; the runtime resolves it.
        $result = std::make_unique<CallExpr>($ns + "::" + $sym, std::move($args));
        set_expr_location($result.get(), @ns);
      }
    | primary_expr[e] { $result = std::move($e); }
    ;

expr_list[result]
    : expr[first]
      {
        $result = std::vector<std::unique_ptr<Expr>>();
        $result.push_back(std::move($first));
      }
    | expr_list[existing] COMMA expr[next]
      {
        $result = std::move($existing);
        $result.push_back(std::move($next));
      }
    ;

expr_list_with_optional_trailing_comma[result]
    : expr_list[list]
      {
          $result = std::move($list);
      }
    | expr_list[list] COMMA
      {
          $result = std::move($list);
      }
    ;

call_arg_list[result]
    : %empty
      { $result = std::vector<CallArg>(); }
    | call_arg[first]
      {
        $result = std::vector<CallArg>();
        $result.push_back(std::move(*$first));
      }
    | call_arg_list[existing] COMMA call_arg[next]
      {
        $result = std::move($existing);
        $result.push_back(std::move(*$next));
      }
    ;

call_arg_list_with_optional_trailing_comma[result]
    : call_arg_list[args]
      {
          $result = std::move($args);
      }
    | call_arg_list[args] COMMA
      {
          $result = std::move($args);
      }
    ;

call_arg[result]
    : IDENTIFIER[name] ASSIGN expr[val]
      { $result = std::make_unique<CallArg>(std::move($name), std::move($val)); }
    | expr[val]
      { $result = std::make_unique<CallArg>(std::move($val)); }
    ;

%%

namespace falcon::atc {
  void Parser::error(const location_type& loc, const std::string& msg) {
    current_errors.push_back(ParseError{
        loc.begin.line,
        loc.begin.column,
        loc.end.line,
        loc.end.column,
        msg
    });
  }
}
