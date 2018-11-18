#include "mdcc.h"

typedef struct Scope {
  struct Scope *outer;
  Map *vars;
} Scope;

Scope *scope;
int nfunc_vars;
Vector *func_vars;

static Scope *new_scope(Scope *outer) {
  Scope *scope = malloc(sizeof(Scope));
  scope->vars = new_map();
  scope->outer = outer;
  return scope;
}

static Var *lookup_var(char *name) {
  for (Scope *s = scope; s != NULL; s = s->outer) {
    Var *var = map_get(s->vars, name);
    if (var)
      return var;
  }
  return NULL;
}

static Var *new_var(Type *ty, char *name) {
  Var *var = malloc(sizeof(Var));
  var->ty = ty;
  var->name = name;
  var->offset = ++nfunc_vars;
  map_set(scope->vars, name, (void *)var);
  vec_push(func_vars, (void *)var);
  return var;
}

__attribute__((noreturn)) static void bad_token(Token *t, char *msg) {
  fprintf(stderr, "Error at token %d\n", t->ty);
  fprintf(stderr, "%s\n", msg);
  exit(1);
}

static bool consume(int ty) {
  if (tokens[pos].ty != ty)
    return false;
  pos++;
  return true;
}

static bool consume_str(char *s) {
  for (int i = 0; i < strlen(s); i++) {
    if (tokens[pos + i].ty == TK_EOF || s[i] != tokens[pos + i].ty)
      return false;
  }
  pos += strlen(s);
  return true;
}

static void expect(int ty) {
  Token t = tokens[pos];
  if (t.ty == ty) {
    pos++;
    return;
  }
  bad_token(&t, format("Expected token %d", ty));
}

static Node *new_node(int ty, Node *lhs, Node *rhs) {
  Node *node = malloc(sizeof(Node));
  node->ty = ty;
  node->lhs = lhs;
  node->rhs = rhs;
  return node;
}

static Node *new_node_num(int val) {
  Node *node = malloc(sizeof(Node));
  node->ty = ND_NUM;
  node->val = val;
  return node;
}

static Node *new_node_null() {
  Node *node = malloc(sizeof(Node));
  node->ty = ND_NULL;
  return node;
}

static Node *new_node_ident(char *name, Var *var) {
  Node *node = malloc(sizeof(Node));
  node->ty = ND_IDENT;
  node->name = name;
  node->var = var;
  return node;
}

static Node *expr();

static Node *primary_expr() {
  if (tokens[pos].ty == TK_IDENT) {
    char *name = strdup(tokens[pos].name);
    if (tokens[pos + 1].ty == '(') {
      Node *node = malloc(sizeof(Node));
      node->ty = ND_CALL;
      node->name = name;
      pos += 2;
      expect(')');
      return node;
    } else {
      Var *var;
      if ((var = lookup_var(name)) == NULL)
        bad_token(&tokens[pos],
                  format("Undefined identifier %s", tokens[pos].name));
      pos++;
      return new_node_ident(name, var);
    }
  }
  if (tokens[pos].ty == TK_NUM) {
    return new_node_num(tokens[pos++].val);
  }
  if (tokens[pos].ty == '(') {
    pos++;
    Node *node = expr();
    if (tokens[pos].ty != ')') {
      bad_token(&tokens[pos], "No closing parenthesis.");
    }
    pos++;
    return node;
  }
  return new_node_null();
}

static Node *postfix_expr() { return primary_expr(); }

static Node *cast_expr();

static Node *unary_expr() {
  if (consume('&'))
    return new_node(ND_ADDR, new_node_null(), cast_expr());
  if (consume('*'))
    return new_node(ND_DEREF, new_node_null(), cast_expr());
  return postfix_expr();
}

static Node *cast_expr() { return unary_expr(); }

static void *decl();

static Node *conditional_expr();

static Node *assignment_expr() {
  int prev_pos = pos;
  Node *lhs = unary_expr();

  // If next token is assignment operator, parse assignment-expression as rhs.
  // Otherwise, rollback the position of the token and parse
  // conditional-expression.
  if (consume('=')) {
    return new_node('=', lhs, assignment_expr());
  } else if (consume_str("*=")) {
    return new_node('=', lhs, new_node('*', lhs, assignment_expr()));
  } else if (consume_str("/=")) {
    return new_node('=', lhs, new_node('/', lhs, assignment_expr()));
  } else if (consume_str("+=")) {
    return new_node('=', lhs, new_node('+', lhs, assignment_expr()));
  } else if (consume_str("-=")) {
    return new_node('=', lhs, new_node('-', lhs, assignment_expr()));
  } else {
    pos = prev_pos;
    return conditional_expr();
  }
}

static Node *multiplicative_expr() {
  Node *lhs = cast_expr();
  int ty = tokens[pos].ty;
  if (ty == '*' || ty == '/' || ty == '%') {
    pos++;
    return new_node(ty, lhs, multiplicative_expr());
  } else {
    return lhs;
  }
}

static Node *additive_expr() {
  Node *lhs = multiplicative_expr();
  int ty = tokens[pos].ty;
  if (ty == '+' || ty == '-') {
    pos++;
    return new_node(ty, lhs, additive_expr());
  } else {
    return lhs;
  }
}

static Node *shift_expr() { return additive_expr(); }

static Node *relational_expr() { return shift_expr(); }

static Node *equality_expr() { return relational_expr(); }

static Node *and_expr() { return equality_expr(); }

static Node *exclusive_or_expr() { return and_expr(); }

static Node *inclusive_or_expr() { return exclusive_or_expr(); }

static Node *logical_and_expr() { return inclusive_or_expr(); }

static Node *logical_or_expr() { return logical_and_expr(); }

static Node *conditional_expr() { return logical_or_expr(); }

static Node *expr() { return assignment_expr(); }

static Node *expr_stmt() {
  if (consume(';'))
    return new_node_null();
  Node *e = expr();
  expect(';');
  return e;
}

static Type *decl_specifier() {
  if (tokens[pos].ty == TK_INT) {
    pos++;
    Type *type = malloc(sizeof(Type));
    type->ty = TY_INT;
    return type;
  }
  bad_token(&tokens[pos], format("Unknown declaration specifier"));
}

static Type *ptr(Type *ty) {
  Type *newty = malloc(sizeof(Type));
  newty->ty = TY_PTR;
  newty->ptr_to = ty;
  return newty;
}

static Node *declarator(Type *ty);

static Var *param_decl() {
  Type *ty = decl_specifier();
  Node *node = declarator(ty);
  return node->var;
}

static Vector *param_list() {
  Vector *params = new_vec();
  Var *param;
  param = param_decl();
  vec_push(params, param);
  while (consume(',')) {
    param = param_decl();
    vec_push(params, param);
  }
  return params;
}

static Vector *param_type_list() { return param_list(); }
static Node *compound_stmt();

static Node *direct_declarator(Type *ty) {
  if (tokens[pos].ty != TK_IDENT)
    bad_token(&tokens[pos], "Token is not identifier.");
  char *name = tokens[pos].name;
  pos++;

  // Function parameters
  if (consume('(')) {
    Vector *params;
    if (consume(')')) {
      params = new_vec();
    } else {
      params = param_type_list();
      expect(')');
    }
    Node *node = malloc(sizeof(Node));
    node->ty = ND_FUNC;
    node->name = name;
    return node;
    // Variable definition
  } else {
    Node *node = malloc(sizeof(Node));
    node->ty = ND_VARREF;
    node->var = new_var(ty, name);
    return node;
  }
}

static Node *declarator(Type *ty) {
  while (consume('*'))
    ty = ptr(ty);
  return direct_declarator(ty);
}

static Node *init_declarator(Type *ty) {
  Node *node = declarator(ty);
  if (consume('=')) {
    Node *lhs = new_node_ident(node->var->name, node->var);
    Node *rhs = expr();
    return new_node('=', lhs, rhs);
  } else {
    return new_node_null();
  }
}

static void *decl() {
  Type *ty = decl_specifier();
  if (consume(';'))
    return new_node_null();
  Node *node = init_declarator(ty);
  expect(';');
  return node;
}

static Node *stmt() { return expr_stmt(); }

static Node *compound_stmt() {
  expect('{');
  scope = new_scope(scope);
  Node *node = malloc(sizeof(Node));
  node->ty = ND_COMP_STMT;
  node->stmts = new_vec();
  while (!consume('}')) {
    if (istypename(tokens[pos].ty))
      vec_push(node->stmts, decl());
    else
      vec_push(node->stmts, stmt());
  }
  scope = scope->outer;
  return node;
}

static Node *func_def() {
  func_vars = new_vec();
  nfunc_vars = 0;
  Type *ty = decl_specifier();
  Node *node = declarator(ty);
  if (node->ty != ND_FUNC)
    error("expected function definition but got %d", node->ty);
  node->body = compound_stmt();
  node->func_vars = func_vars;
  return node;
}

static Node *root() {
  Node *node = malloc(sizeof(Node));
  node->ty = ND_ROOT;
  node->funcs = new_vec();
  while (tokens[pos].ty != TK_EOF) {
    vec_push(node->funcs, func_def());
  }
  return node;
}

Node *parse() {
  Node *node;
  scope = new_scope(NULL);
  node = root();
  return node;
}
