// ============================================================
// Grape Compiler - Main Entry Point
// Native, fast compiler for Grape language
// ============================================================

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>
#include <ctype.h>
#include <stdarg.h>

// ============================================================
// CONSTANTS
// ============================================================

#define MAX_TOKENS 10000
#define MAX_IDENTIFIERS 5000
#define MAX_STRING_LEN 4096
#define MAX_LINE_LEN 8192
#define MAX_SCOPE_DEPTH 64
#define MAX_FIELDS 128
#define MAX_PARAMS 32

// Helper function for string duplication
static char* my_strndup(const char* s, size_t n) {
    if (!s) return NULL;
    size_t len = strlen(s);
    if (n < len) len = n;
    char* dup = malloc(len + 1);
    if (dup) {
        memcpy(dup, s, len);
        dup[len] = '\0';
    }
    return dup;
}

// ============================================================
// TOKEN TYPES
// ============================================================

typedef enum {
    // Literals
    TOK_INT,
    TOK_FLOAT,
    TOK_STRING,
    TOK_CHAR,
    TOK_BOOL,
    
    // Identifiers & Keywords
    TOK_IDENT,
    TOK_USE,
    TOK_CLASS,
    TOK_THIS,
    TOK_FN,
    TOK_IF,
    TOK_ELSE,
    TOK_WHILE,
    TOK_FOR,
    TOK_IN,
    TOK_RETURN,
    TOK_VAR,
    TOK_PUBLIC,
    TOK_PRIVATE,
    TOK_ANY,
    TOK_VOID,
    
    // Types
    TOK_INT8,
    TOK_INT16,
    TOK_INT32,
    TOK_INT64,
    TOK_UINT8,
    TOK_UINT16,
    TOK_UINT32,
    TOK_UINT64,
    TOK_FLOAT32,
    TOK_FLOAT64,
    TOK_STRING_TYPE,
    TOK_BOOL_TYPE,
    
    // Operators
    TOK_PLUS,
    TOK_MINUS,
    TOK_STAR,
    TOK_SLASH,
    TOK_PERCENT,
    TOK_EQ,
    TOK_NEQ,
    TOK_LT,
    TOK_GT,
    TOK_LTE,
    TOK_GTE,
    TOK_AND,
    TOK_OR,
    TOK_NOT,
    TOK_ASSIGN,
    TOK_PLUS_ASSIGN,
    TOK_MINUS_ASSIGN,
    TOK_STAR_ASSIGN,
    TOK_SLASH_ASSIGN,
    
    // Delimiters
    TOK_LPAREN,
    TOK_RPAREN,
    TOK_LBRACKET,
    TOK_RBRACKET,
    TOK_LBRACE,
    TOK_RBRACE,
    TOK_SEMICOLON,
    TOK_COLON,
    TOK_COMMA,
    TOK_DOT,
    TOK_ARROW,
    TOK_DOLLAR,
    TOK_AT,
    
    // Special
    TOK_EOF,
    TOK_ERROR,
    TOK_NEWLINE,
    TOK_COMMENT
} TokenType;

// ============================================================
// TOKEN STRUCTURE
// ============================================================

typedef struct {
    TokenType type;
    char* start;
    int length;
    int line;
    int column;
    union {
        int64_t int_val;
        double float_val;
        bool bool_val;
        char* string_val;
    } value;
} Token;

// ============================================================
// SYMBOL TABLE
// ============================================================

typedef enum {
    SYM_VAR,
    SYM_CONST,
    SYM_FUNC,
    SYM_PARAM,
    SYM_FIELD,
    SYM_CLASS,
    SYM_LOCAL
} SymbolKind;

typedef struct Symbol {
    char* name;
    SymbolKind kind;
    char* type_name;
    int scope_depth;
    int offset;
    bool is_mutable;
    struct Symbol* next;
} Symbol;

typedef struct {
    Symbol* symbols[MAX_IDENTIFIERS];
    int count;
    int scope_depth;
} SymbolTable;

// ============================================================
// AST NODES
// ============================================================

typedef enum {
    NODE_PROGRAM,
    NODE_FUNCTION,
    NODE_CLASS,
    NODE_BLOCK,
    NODE_VAR_DECL,
    NODE_ASSIGN,
    NODE_BINARY,
    NODE_UNARY,
    NODE_CALL,
    NODE_INDEX,
    NODE_MEMBER,
    NODE_LITERAL,
    NODE_IDENTIFIER,
    NODE_IF,
    NODE_WHILE,
    NODE_FOR,
    NODE_RETURN,
    NODE_EXPRESSION,
    NODE_LAMBDA,
    NODE_ARRAY,
    NODE_TEMPLATE
} NodeType;

typedef struct ASTNode {
    NodeType type;
    int line;
    int column;
    
    union {
        // Literals
        struct {
            int64_t int_val;
            double float_val;
            char* string_val;
            bool bool_val;
        } literal;
        
        // Identifier
        struct {
            char* name;
        } identifier;
        
        // Binary expression
        struct {
            struct ASTNode* left;
            struct ASTNode* right;
            Token op;
        } binary;
        
        // Unary expression
        struct {
            struct ASTNode* operand;
            Token op;
        } unary;
        
        // Call/Invocation
        struct {
            struct ASTNode* callee;
            struct ASTNode** args;
            int arg_count;
            bool use_brackets;  // [] vs ()
        } call;
        
        // Member access
        struct {
            struct ASTNode* object;
            char* member;
        } member;
        
        // Index access
        struct {
            struct ASTNode* object;
            struct ASTNode* index;
        } index;
        
        // Variable declaration
        struct {
            char* name;
            char* type_name;
            struct ASTNode* initializer;
            bool is_const;
        } var_decl;
        
        // Assignment
        struct {
            char* name;
            struct ASTNode* value;
            bool is_compound;
            Token op;
        } assign;
        
        // If statement
        struct {
            struct ASTNode* condition;
            struct ASTNode* then_branch;
            struct ASTNode* else_branch;
        } if_stmt;
        
        // While loop
        struct {
            struct ASTNode* condition;
            struct ASTNode* body;
        } while_stmt;
        
        // For loop
        struct {
            char* var_name;
            char* type_name;
            struct ASTNode* iterable;
            struct ASTNode* body;
        } for_stmt;
        
        // Return statement
        struct {
            struct ASTNode* value;
        } return_stmt;
        
        // Function
        struct {
            char* name;
            char* return_type;
            struct ASTNode** params;
            int param_count;
            struct ASTNode* body;
            bool is_method;
            bool is_constructor;
            bool is_nested;
            int nesting_depth;
        } function;
        
        // Lambda
        struct {
            struct ASTNode** params;
            int param_count;
            struct ASTNode* body;
            char* signature;
        } lambda;
        
        // Class
        struct {
            char* name;
            char* template_param;
            struct ASTNode** members;
            int member_count;
        } class_def;
        
        // Block
        struct {
            struct ASTNode** statements;
            int statement_count;
        } block;
        
        // Array literal
        struct {
            struct ASTNode** elements;
            int element_count;
        } array;
        
        // Template instantiation
        struct {
            char* name;
            char* template_arg;
            struct ASTNode** args;
            int arg_count;
        } template_inst;
    } data;
    
    struct ASTNode** children;
    int child_count;
} ASTNode;

// ============================================================
// PARSER STATE
// ============================================================

typedef struct {
    Token* tokens;
    int current;
    int count;
    int line;
    int column;
    bool had_error;
    bool panic_mode;
} Parser;

// ============================================================
// CODE GENERATOR
// ============================================================

typedef struct {
    char* buffer;
    int capacity;
    int length;
    int indent;
    bool needs_semicolon;
} CodeGenerator;

// ============================================================
// COMPILER CONTEXT
// ============================================================

typedef struct {
    Parser parser;
    SymbolTable sym_table;
    CodeGenerator generator;
    ASTNode* ast;
    char* source;
    int source_len;
    int scope_depth;
    int class_depth;
    char* current_class;
    char* current_function;
    bool in_lambda;
} Compiler;

// ============================================================
// UTILITY FUNCTIONS
// ============================================================

static char* str_dup(const char* s) {
    if (!s) return NULL;
    int len = strlen(s);
    char* dup = malloc(len + 1);
    strcpy(dup, s);
    return dup;
}

static void* allocate(size_t size) {
    void* ptr = malloc(size);
    if (!ptr) {
        fprintf(stderr, "Memory allocation failed\n");
        exit(1);
    }
    return ptr;
}

static void report_error(int line, const char* message) {
    fprintf(stderr, "[Error at line %d] %s\n", line, message);
}

// ============================================================
// LEXER
// ============================================================

static Compiler* g_compiler;

static bool is_alpha(char c) {
    return (c >= 'a' && c <= 'z') ||
           (c >= 'A' && c <= 'Z') ||
           c == '_';
}

static bool is_digit(char c) {
    return c >= '0' && c <= '9';
}

static bool is_at_end() {
    return g_compiler->parser.current >= g_compiler->parser.count;
}

static char peek() {
    if (is_at_end()) return '\0';
    return g_compiler->source[g_compiler->parser.current];
}

static char peek_next() {
    if (g_compiler->parser.current + 1 >= g_compiler->parser.count) return '\0';
    return g_compiler->source[g_compiler->parser.current + 1];
}

static char advance() {
    g_compiler->parser.current++;
    return g_compiler->source[g_compiler->parser.current - 1];
}

static bool match(char expected) {
    if (is_at_end()) return false;
    if (g_compiler->source[g_compiler->parser.current] != expected) return false;
    g_compiler->parser.current++;
    return true;
}

static Token make_token(TokenType type, int line, int column, int start, int length) {
    Token token;
    token.type = type;
    token.line = line;
    token.column = column;
    token.start = &g_compiler->source[start];
    token.length = length;
    return token;
}

static Token error_token(const char* message, int line, int column) {
    Token token;
    token.type = TOK_ERROR;
    token.line = line;
    token.column = column;
    token.start = (char*)message;
    token.length = strlen(message);
    return token;
}

static void skip_whitespace() {
    for (;;) {
        char c = peek();
        switch (c) {
            case ' ':
            case '\r':
            case '\t':
                advance();
                break;
            case '\n':
                g_compiler->parser.line++;
                g_compiler->parser.column = 0;
                advance();
                break;
            case '/':
                if (peek_next() == '/') {
                    while (peek() != '\n' && !is_at_end()) advance();
                } else if (peek_next() == '*') {
                    advance();
                    advance();
                    while (!is_at_end() && !(peek() == '*' && peek_next() == '/')) {
                        if (peek() == '\n') g_compiler->parser.line++;
                        advance();
                    }
                    if (!is_at_end()) {
                        advance();
                        advance();
                    }
                } else {
                    return;
                }
                break;
            default:
                return;
        }
    }
}

static TokenType check_keyword(int start, int length, const char* keyword, TokenType type) {
    if (length != strlen(keyword)) return TOK_IDENT;
    if (strncmp(&g_compiler->source[start], keyword, length) != 0) return TOK_IDENT;
    return type;
}

static TokenType identifier_type(int start, int length) {
    switch (g_compiler->source[start]) {
        case 'a':
            if (length == 3) return check_keyword(start, length, "any", TOK_ANY);
            break;
        case 'b':
            if (length == 4) return check_keyword(start, length, "bool", TOK_BOOL_TYPE);
            break;
        case 'c':
            if (length == 5) return check_keyword(start, length, "class", TOK_CLASS);
            break;
        case 'e':
            if (length == 4) return check_keyword(start, length, "else", TOK_ELSE);
            break;
        case 'f':
            if (length == 2) return check_keyword(start, length, "fn", TOK_FN);
            if (length == 3) return check_keyword(start, length, "for", TOK_FOR);
            break;
        case 'i':
            if (length == 2) return check_keyword(start, length, "if", TOK_IF);
            if (length == 2) return check_keyword(start, length, "in", TOK_IN);
            if (length == 4) return check_keyword(start, length, "int8", TOK_INT8);
            if (length == 5) return check_keyword(start, length, "int16", TOK_INT16);
            if (length == 5) return check_keyword(start, length, "int32", TOK_INT32);
            if (length == 5) return check_keyword(start, length, "int64", TOK_INT64);
            break;
        case 'p':
            if (length == 6) return check_keyword(start, length, "public", TOK_PUBLIC);
            if (length == 7) return check_keyword(start, length, "private", TOK_PRIVATE);
            break;
        case 'r':
            if (length == 6) return check_keyword(start, length, "return", TOK_RETURN);
            break;
        case 's':
            if (length == 6) return check_keyword(start, length, "string", TOK_STRING_TYPE);
            break;
        case 't':
            if (length == 4) return check_keyword(start, length, "this", TOK_THIS);
            break;
        case 'u':
            if (length == 3) return check_keyword(start, length, "use", TOK_USE);
            if (length == 5) return check_keyword(start, length, "uint8", TOK_UINT8);
            if (length == 6) return check_keyword(start, length, "uint16", TOK_UINT16);
            if (length == 6) return check_keyword(start, length, "uint32", TOK_UINT32);
            if (length == 6) return check_keyword(start, length, "uint64", TOK_UINT64);
            break;
        case 'v':
            if (length == 3) return check_keyword(start, length, "var", TOK_VAR);
            if (length == 4) return check_keyword(start, length, "void", TOK_VOID);
            break;
        case 'w':
            if (length == 5) return check_keyword(start, length, "while", TOK_WHILE);
            break;
    }
    return TOK_IDENT;
}

static Token identifier() {
    int start = g_compiler->parser.current - 1;
    while (is_alpha(peek()) || is_digit(peek())) advance();
    int length = g_compiler->parser.current - 1 - start;
    TokenType type = identifier_type(start, length);
    return make_token(type, g_compiler->parser.line, g_compiler->parser.column, start, length);
}

static Token number() {
    int start = g_compiler->parser.current - 1;
    bool is_float = false;
    
    while (is_digit(peek())) advance();
    
    if (peek() == '.' && is_digit(peek_next())) {
        is_float = true;
        advance();
        while (is_digit(peek())) advance();
    }
    
    int length = g_compiler->parser.current - 1 - start;
    TokenType type = is_float ? TOK_FLOAT : TOK_INT;
    return make_token(type, g_compiler->parser.line, g_compiler->parser.column, start, length);
}

static Token string() {
    int start = g_compiler->parser.current - 1;
    while (peek() != '"' && !is_at_end()) {
        if (peek() == '\\' && peek_next() != '\0') advance();
        advance();
    }
    
    if (is_at_end()) {
        return error_token("Unterminated string", g_compiler->parser.line, g_compiler->parser.column);
    }
    
    advance();  // closing quote
    int length = g_compiler->parser.current - 1 - start;
    return make_token(TOK_STRING, g_compiler->parser.line, g_compiler->parser.column, start, length);
}

static Token scan_token() {
    skip_whitespace();
    
    g_compiler->parser.column = g_compiler->parser.current;
    
    char c = advance();
    
    if (is_alpha(c)) return identifier();
    if (is_digit(c)) return number();
    
    switch (c) {
        case '\0':
            return make_token(TOK_EOF, g_compiler->parser.line, g_compiler->parser.column, g_compiler->parser.current - 1, 0);
        
        case '(': return make_token(TOK_LPAREN, g_compiler->parser.line, g_compiler->parser.column, g_compiler->parser.current - 1, 1);
        case ')': return make_token(TOK_RPAREN, g_compiler->parser.line, g_compiler->parser.column, g_compiler->parser.current - 1, 1);
        case '{': return make_token(TOK_LBRACE, g_compiler->parser.line, g_compiler->parser.column, g_compiler->parser.current - 1, 1);
        case '}': return make_token(TOK_RBRACE, g_compiler->parser.line, g_compiler->parser.column, g_compiler->parser.current - 1, 1);
        case ';': return make_token(TOK_SEMICOLON, g_compiler->parser.line, g_compiler->parser.column, g_compiler->parser.current - 1, 1);
        case ',': return make_token(TOK_COMMA, g_compiler->parser.line, g_compiler->parser.column, g_compiler->parser.current - 1, 1);
        case '.': return make_token(TOK_DOT, g_compiler->parser.line, g_compiler->parser.column, g_compiler->parser.current - 1, 1);
        case ':': return make_token(TOK_COLON, g_compiler->parser.line, g_compiler->parser.column, g_compiler->parser.current - 1, 1);
        case '@': return make_token(TOK_AT, g_compiler->parser.line, g_compiler->parser.column, g_compiler->parser.current - 1, 1);
        
        case '[': return make_token(TOK_LBRACKET, g_compiler->parser.line, g_compiler->parser.column, g_compiler->parser.current - 1, 1);
        case ']': return make_token(TOK_RBRACKET, g_compiler->parser.line, g_compiler->parser.column, g_compiler->parser.current - 1, 1);
        
        case '$': return make_token(TOK_DOLLAR, g_compiler->parser.line, g_compiler->parser.column, g_compiler->parser.current - 1, 1);
        
        case '+':
            if (match('=')) return make_token(TOK_PLUS_ASSIGN, g_compiler->parser.line, g_compiler->parser.column, g_compiler->parser.current - 2, 2);
            return make_token(TOK_PLUS, g_compiler->parser.line, g_compiler->parser.column, g_compiler->parser.current - 1, 1);
        
        case '-':
            if (match('>')) return make_token(TOK_ARROW, g_compiler->parser.line, g_compiler->parser.column, g_compiler->parser.current - 2, 2);
            if (match('=')) return make_token(TOK_MINUS_ASSIGN, g_compiler->parser.line, g_compiler->parser.column, g_compiler->parser.current - 2, 2);
            return make_token(TOK_MINUS, g_compiler->parser.line, g_compiler->parser.column, g_compiler->parser.current - 1, 1);
        
        case '*':
            if (match('=')) return make_token(TOK_STAR_ASSIGN, g_compiler->parser.line, g_compiler->parser.column, g_compiler->parser.current - 2, 2);
            return make_token(TOK_STAR, g_compiler->parser.line, g_compiler->parser.column, g_compiler->parser.current - 1, 1);
        
        case '/':
            if (match('=')) return make_token(TOK_SLASH_ASSIGN, g_compiler->parser.line, g_compiler->parser.column, g_compiler->parser.current - 2, 2);
            return make_token(TOK_SLASH, g_compiler->parser.line, g_compiler->parser.column, g_compiler->parser.current - 1, 1);
        
        case '%': return make_token(TOK_PERCENT, g_compiler->parser.line, g_compiler->parser.column, g_compiler->parser.current - 1, 1);
        
        case '=':
            if (match('=')) return make_token(TOK_EQ, g_compiler->parser.line, g_compiler->parser.column, g_compiler->parser.current - 2, 2);
            return make_token(TOK_ASSIGN, g_compiler->parser.line, g_compiler->parser.column, g_compiler->parser.current - 1, 1);
        
        case '!':
            if (match('=')) return make_token(TOK_NEQ, g_compiler->parser.line, g_compiler->parser.column, g_compiler->parser.current - 2, 2);
            return make_token(TOK_NOT, g_compiler->parser.line, g_compiler->parser.column, g_compiler->parser.current - 1, 1);
        
        case '<':
            if (match('=')) return make_token(TOK_LTE, g_compiler->parser.line, g_compiler->parser.column, g_compiler->parser.current - 2, 2);
            return make_token(TOK_LT, g_compiler->parser.line, g_compiler->parser.column, g_compiler->parser.current - 1, 1);
        
        case '>':
            if (match('=')) return make_token(TOK_GTE, g_compiler->parser.line, g_compiler->parser.column, g_compiler->parser.current - 2, 2);
            return make_token(TOK_GT, g_compiler->parser.line, g_compiler->parser.column, g_compiler->parser.current - 1, 1);
        
        case '&':
            if (match('&')) return make_token(TOK_AND, g_compiler->parser.line, g_compiler->parser.column, g_compiler->parser.current - 2, 2);
            break;
        
        case '|':
            if (match('|')) return make_token(TOK_OR, g_compiler->parser.line, g_compiler->parser.column, g_compiler->parser.current - 2, 2);
            break;
        
        case '"': return string();
    }
    
    return error_token("Unexpected character", g_compiler->parser.line, g_compiler->parser.column);
}

// ============================================================
// TOKENIZER (Public API)
// ============================================================

static Token* tokenize(const char* source, int* out_count) {
    g_compiler = allocate(sizeof(Compiler));
    memset(g_compiler, 0, sizeof(Compiler));
    
    g_compiler->source = (char*)source;
    g_compiler->source_len = strlen(source);
    g_compiler->parser.line = 1;
    g_compiler->parser.column = 0;
    g_compiler->parser.current = 0;
    
    // First pass: count tokens
    int count = 0;
    while (!is_at_end()) {
        scan_token();
        count++;
    }
    
    // Allocate token array
    Token* tokens = allocate(sizeof(Token) * (count + 1));
    
    // Second pass: collect tokens
    g_compiler->parser.current = 0;
    g_compiler->parser.line = 1;
    g_compiler->parser.column = 0;
    
    int i = 0;
    while (!is_at_end()) {
        tokens[i++] = scan_token();
    }
    tokens[i] = make_token(TOK_EOF, g_compiler->parser.line, g_compiler->parser.column, g_compiler->parser.current, 0);
    
    *out_count = i;
    return tokens;
}

// ============================================================
// PARSER IMPLEMENTATION
// ============================================================

static void init_parser(Compiler* comp, Token* tokens, int count) {
    comp->parser.tokens = tokens;
    comp->parser.current = 0;
    comp->parser.count = count;
    comp->parser.line = 1;
    comp->parser.column = 0;
    comp->parser.had_error = false;
    comp->parser.panic_mode = false;
    comp->scope_depth = 0;
    comp->class_depth = 0;
    comp->current_class = NULL;
    comp->current_function = NULL;
    comp->in_lambda = false;
}

static Token* current_token(Compiler* comp) {
    return &comp->parser.tokens[comp->parser.current];
}

static Token* previous_token(Compiler* comp) {
    return &comp->parser.tokens[comp->parser.current - 1];
}

static Token* advance_token(Compiler* comp) {
    if (!is_at_end()) comp->parser.current++;
    return previous_token(comp);
}

static bool check(Compiler* comp, TokenType type) {
    if (is_at_end()) return false;
    return current_token(comp)->type == type;
}

static bool match_token(Compiler* comp, TokenType type) {
    if (!check(comp, type)) return false;
    advance_token(comp);
    return true;
}

static bool expect(Compiler* comp, TokenType type, const char* message) {
    if (check(comp, type)) {
        advance_token(comp);
        return true;
    }
    report_error(current_token(comp)->line, message);
    comp->parser.had_error = true;
    return false;
}

// Forward declarations
static ASTNode* parse_expression(Compiler* comp);
static ASTNode* parse_statement(Compiler* comp);
static ASTNode* parse_block(Compiler* comp);

// ============================================================
// AST NODE CREATION
// ============================================================

static ASTNode* new_node(NodeType type, int line, int column) {
    ASTNode* node = allocate(sizeof(ASTNode));
    memset(node, 0, sizeof(ASTNode));
    node->type = type;
    node->line = line;
    node->column = column;
    node->children = NULL;
    node->child_count = 0;
    return node;
}

static ASTNode* new_literal_int(int64_t val, int line, int column) {
    ASTNode* node = new_node(NODE_LITERAL, line, column);
    node->data.literal.int_val = val;
    node->data.literal.float_val = 0;
    node->data.literal.string_val = NULL;
    node->data.literal.bool_val = false;
    return node;
}

static ASTNode* new_literal_string(char* val, int line, int column) {
    ASTNode* node = new_node(NODE_LITERAL, line, column);
    node->data.literal.int_val = 0;
    node->data.literal.float_val = 0;
    node->data.literal.string_val = val;
    node->data.literal.bool_val = false;
    return node;
}

static ASTNode* new_identifier(char* name, int line, int column) {
    ASTNode* node = new_node(NODE_IDENTIFIER, line, column);
    node->data.identifier.name = name;
    return node;
}

// ============================================================
// PARSING FUNCTIONS
// ============================================================

static ASTNode* parse_primary(Compiler* comp) {
    Token* token = current_token(comp);
    
    switch (token->type) {
        case TOK_INT: {
            advance_token(comp);
            char* num_str = my_strndup(token->start, token->length);
            int64_t val = strtoll(num_str, NULL, 10);
            free(num_str);
            return new_literal_int(val, token->line, token->column);
        }
        
        case TOK_FLOAT: {
            advance_token(comp);
            char* num_str = my_strndup(token->start, token->length);
            double val = strtod(num_str, NULL);
            free(num_str);
            ASTNode* node = new_node(NODE_LITERAL, token->line, token->column);
            node->data.literal.float_val = val;
            return node;
        }
        
        case TOK_STRING: {
            advance_token(comp);
            // Extract string content (remove quotes)
            char* str = my_strndup(token->start + 1, token->length - 2);
            return new_literal_string(str, token->line, token->column);
        }
        
        
        case TOK_IDENT:
        case TOK_INT8:
        case TOK_INT16:
        case TOK_INT32:
        case TOK_INT64:
        case TOK_STRING_TYPE:
        
        case TOK_LPAREN: {
            advance_token(comp);
            ASTNode* expr = parse_expression(comp);
            expect(comp, TOK_RPAREN, "Expected ')' after expression");
            return expr;
        }
        
        case TOK_LBRACKET: {
            // Array literal or bracket call
            advance_token(comp);
            
            // Check if it's an empty array
            if (check(comp, TOK_RBRACKET)) {
                advance_token(comp);
                ASTNode* array = new_node(NODE_ARRAY, token->line, token->column);
                array->data.array.elements = NULL;
                array->data.array.element_count = 0;
                return array;
            }
            
            // Parse array elements
            ASTNode** elements = allocate(sizeof(ASTNode*) * 16);
            int count = 0;
            int capacity = 16;
            
            do {
                if (count >= capacity) {
                    capacity *= 2;
                    elements = realloc(elements, sizeof(ASTNode*) * capacity);
                }
                elements[count++] = parse_expression(comp);
            } while (match_token(comp, TOK_COMMA));
            
            expect(comp, TOK_RBRACKET, "Expected ']' after array elements");
            
            ASTNode* array = new_node(NODE_ARRAY, token->line, token->column);
            array->data.array.elements = elements;
            array->data.array.element_count = count;
            return array;
        }
        
        case TOK_FN: {
            // Lambda expression
            advance_token(comp);
            
            ASTNode* lambda = new_node(NODE_LAMBDA, token->line, token->column);
            
            // Parse parameters
            ASTNode** params = allocate(sizeof(ASTNode*) * MAX_PARAMS);
            int param_count = 0;
            
            if (match_token(comp, TOK_LBRACKET)) {
                // Parameters in brackets
                if (!check(comp, TOK_RBRACKET)) {
                    do {
                        if (param_count >= MAX_PARAMS) break;
                        params[param_count++] = parse_expression(comp);
                    } while (match_token(comp, TOK_COMMA));
                }
                expect(comp, TOK_RBRACKET, "Expected ']' after lambda parameters");
            }
            
            expect(comp, TOK_ARROW, "Expected '=>' in lambda");
            
            lambda->data.lambda.params = params;
            lambda->data.lambda.param_count = param_count;
            lambda->data.lambda.body = parse_expression(comp);
            
            return lambda;
        }
        
        default:
            break;
    }
    
    report_error(token->line, "Expected expression");
    comp->parser.had_error = true;
    return new_literal_int(0, token->line, token->column);
}

static ASTNode* parse_call(Compiler* comp, ASTNode* callee) {
    Token* token = previous_token(comp);
    
    ASTNode* call = new_node(NODE_CALL, token->line, token->column);
    call->data.call.callee = callee;
    call->data.call.use_brackets = (token->type == TOK_LBRACKET);
    
    ASTNode** args = allocate(sizeof(ASTNode*) * MAX_PARAMS);
    int arg_count = 0;
    
    if (!check(comp, (token->type == TOK_LBRACKET) ? TOK_RBRACKET : TOK_RPAREN)) {
        do {
            if (arg_count >= MAX_PARAMS) break;
            args[arg_count++] = parse_expression(comp);
        } while (match_token(comp, TOK_COMMA));
    }
    
    call->data.call.args = args;
    call->data.call.arg_count = arg_count;
    
    if (token->type == TOK_LBRACKET) {
        expect(comp, TOK_RBRACKET, "Expected ']' after arguments");
    } else {
        expect(comp, TOK_RPAREN, "Expected ')' after arguments");
    }
    
    return call;
}

static ASTNode* parse_postfix(Compiler* comp) {
    ASTNode* expr = parse_primary(comp);
    
    for (;;) {
        if (match_token(comp, TOK_DOT)) {
            Token* token = previous_token(comp);
            Token* member = current_token(comp);
            
            if (member->type != TOK_IDENT) {
                report_error(member->line, "Expected member name after '.'");
                comp->parser.had_error = true;
                break;
            }
            
            advance_token(comp);
            
            ASTNode* member_access = new_node(NODE_MEMBER, token->line, token->column);
            member_access->data.member.object = expr;
            member_access->data.member.member = my_strndup(member->start, member->length);
            expr = member_access;
            
            // Check for method call
            if (match_token(comp, TOK_LBRACKET) || match_token(comp, TOK_LPAREN)) {
                expr = parse_call(comp, expr);
            }
        } else if (match_token(comp, TOK_LBRACKET)) {
            // Could be indexing or bracket-style call
            if (expr->type == NODE_CALL || expr->type == NODE_IDENTIFIER || 
                expr->type == NODE_MEMBER) {
                // This is a bracket-style function call
                expr = parse_call(comp, expr);
            } else {
                // This is array indexing
                Token* token = previous_token(comp);
                ASTNode* index = parse_expression(comp);
                expect(comp, TOK_RBRACKET, "Expected ']' after index");
                
                ASTNode* index_expr = new_node(NODE_INDEX, token->line, token->column);
                index_expr->data.index.object = expr;
                index_expr->data.index.index = index;
                expr = index_expr;
            }
        } else if (match_token(comp, TOK_LPAREN)) {
            // Regular function call
            expr = parse_call(comp, expr);
        } else {
            break;
        }
    }
    
    return expr;
}

static ASTNode* parse_unary(Compiler* comp) {
    if (match_token(comp, TOK_MINUS) || match_token(comp, TOK_NOT)) {
        Token* token = previous_token(comp);
        ASTNode* operand = parse_unary(comp);
        
        ASTNode* unary = new_node(NODE_UNARY, token->line, token->column);
        unary->data.unary.operand = operand;
        unary->data.unary.op = *token;
        return unary;
    }
    
    return parse_postfix(comp);
}

static ASTNode* parse_multiplicative(Compiler* comp) {
    ASTNode* left = parse_unary(comp);
    
    while (match_token(comp, TOK_STAR) || match_token(comp, TOK_SLASH) || 
           match_token(comp, TOK_PERCENT)) {
        Token* token = previous_token(comp);
        ASTNode* right = parse_unary(comp);
        
        ASTNode* binary = new_node(NODE_BINARY, token->line, token->column);
        binary->data.binary.left = left;
        binary->data.binary.right = right;
        binary->data.binary.op = *token;
        left = binary;
    }
    
    return left;
}

static ASTNode* parse_additive(Compiler* comp) {
    ASTNode* left = parse_multiplicative(comp);
    
    while (match_token(comp, TOK_PLUS) || match_token(comp, TOK_MINUS)) {
        Token* token = previous_token(comp);
        ASTNode* right = parse_multiplicative(comp);
        
        ASTNode* binary = new_node(NODE_BINARY, token->line, token->column);
        binary->data.binary.left = left;
        binary->data.binary.right = right;
        binary->data.binary.op = *token;
        left = binary;
    }
    
    return left;
}

static ASTNode* parse_comparison(Compiler* comp) {
    ASTNode* left = parse_additive(comp);
    
    while (match_token(comp, TOK_LT) || match_token(comp, TOK_GT) ||
           match_token(comp, TOK_LTE) || match_token(comp, TOK_GTE)) {
        Token* token = previous_token(comp);
        ASTNode* right = parse_additive(comp);
        
        ASTNode* binary = new_node(NODE_BINARY, token->line, token->column);
        binary->data.binary.left = left;
        binary->data.binary.right = right;
        binary->data.binary.op = *token;
        left = binary;
    }
    
    return left;
}

static ASTNode* parse_equality(Compiler* comp) {
    ASTNode* left = parse_comparison(comp);
    
    while (match_token(comp, TOK_EQ) || match_token(comp, TOK_NEQ)) {
        Token* token = previous_token(comp);
        ASTNode* right = parse_comparison(comp);
        
        ASTNode* binary = new_node(NODE_BINARY, token->line, token->column);
        binary->data.binary.left = left;
        binary->data.binary.right = right;
        binary->data.binary.op = *token;
        left = binary;
    }
    
    return left;
}

static ASTNode* parse_logical_and(Compiler* comp) {
    ASTNode* left = parse_equality(comp);
    
    while (match_token(comp, TOK_AND)) {
        Token* token = previous_token(comp);
        ASTNode* right = parse_equality(comp);
        
        ASTNode* binary = new_node(NODE_BINARY, token->line, token->column);
        binary->data.binary.left = left;
        binary->data.binary.right = right;
        binary->data.binary.op = *token;
        left = binary;
    }
    
    return left;
}

static ASTNode* parse_logical_or(Compiler* comp) {
    ASTNode* left = parse_logical_and(comp);
    
    while (match_token(comp, TOK_OR)) {
        Token* token = previous_token(comp);
        ASTNode* right = parse_logical_and(comp);
        
        ASTNode* binary = new_node(NODE_BINARY, token->line, token->column);
        binary->data.binary.left = left;
        binary->data.binary.right = right;
        binary->data.binary.op = *token;
        left = binary;
    }
    
    return left;
}

static ASTNode* parse_expression(Compiler* comp) {
    return parse_logical_or(comp);
}

// ============================================================
// STATEMENT PARSING
// ============================================================

static ASTNode* parse_var_decl(Compiler* comp) {
    Token* token = previous_token(comp);
    
    // Get variable name
    if (!check(comp, TOK_IDENT)) {
        report_error(current_token(comp)->line, "Expected variable name");
        comp->parser.had_error = true;
        return NULL;
    }
    
    Token* name_token = current_token(comp);
    char* name = my_strndup(name_token->start, name_token->length);
    advance_token(comp);
    
    // Optional type annotation
    char* type_name = NULL;
    if (match_token(comp, TOK_COLON)) {
        Token* type_token = current_token(comp);
        type_name = my_strndup(type_token->start, type_token->length);
        advance_token(comp);
    }
    
    // Optional initializer
    ASTNode* initializer = NULL;
    if (match_token(comp, TOK_ASSIGN)) {
        initializer = parse_expression(comp);
    }
    
    expect(comp, TOK_SEMICOLON, "Expected ';' after variable declaration");
    
    ASTNode* decl = new_node(NODE_VAR_DECL, token->line, token->column);
    decl->data.var_decl.name = name;
    decl->data.var_decl.type_name = type_name;
    decl->data.var_decl.initializer = initializer;
    decl->data.var_decl.is_const = false;
    
    return decl;
}

static ASTNode* parse_if_statement(Compiler* comp) {
    Token* token = previous_token(comp);
    
    expect(comp, TOK_LPAREN, "Expected '(' after 'if'");
    ASTNode* condition = parse_expression(comp);
    expect(comp, TOK_RPAREN, "Expected ')' after condition");
    
    ASTNode* then_branch = parse_statement(comp);
    
    ASTNode* else_branch = NULL;
    if (match_token(comp, TOK_ELSE)) {
        else_branch = parse_statement(comp);
    }
    
    ASTNode* if_stmt = new_node(NODE_IF, token->line, token->column);
    if_stmt->data.if_stmt.condition = condition;
    if_stmt->data.if_stmt.then_branch = then_branch;
    if_stmt->data.if_stmt.else_branch = else_branch;
    
    return if_stmt;
}

static ASTNode* parse_while_statement(Compiler* comp) {
    Token* token = previous_token(comp);
    
    expect(comp, TOK_LPAREN, "Expected '(' after 'while'");
    ASTNode* condition = parse_expression(comp);
    expect(comp, TOK_RPAREN, "Expected ')' after condition");
    
    ASTNode* body = parse_statement(comp);
    
    ASTNode* while_stmt = new_node(NODE_WHILE, token->line, token->column);
    while_stmt->data.while_stmt.condition = condition;
    while_stmt->data.while_stmt.body = body;
    
    return while_stmt;
}

static ASTNode* parse_for_statement(Compiler* comp) {
    Token* token = previous_token(comp);
    
    expect(comp, TOK_LPAREN, "Expected '(' after 'for'");
    
    // Parse loop variable
    if (!check(comp, TOK_IDENT)) {
        report_error(current_token(comp)->line, "Expected variable name in for loop");
        comp->parser.had_error = true;
        return NULL;
    }
    
    Token* var_token = current_token(comp);
    char* var_name = my_strndup(var_token->start, var_token->length);
    advance_token(comp);
    
    char* type_name = NULL;
    if (match_token(comp, TOK_COLON)) {
        Token* type_token = current_token(comp);
        type_name = my_strndup(type_token->start, type_token->length);
        advance_token(comp);
    }
    
    expect(comp, TOK_COLON, "Expected ':' in for-each loop");
    
    ASTNode* iterable = parse_expression(comp);
    expect(comp, TOK_RPAREN, "Expected ')' after iterable");
    
    ASTNode* body = parse_statement(comp);
    
    ASTNode* for_stmt = new_node(NODE_FOR, token->line, token->column);
    for_stmt->data.for_stmt.var_name = var_name;
    for_stmt->data.for_stmt.type_name = type_name;
    for_stmt->data.for_stmt.iterable = iterable;
    for_stmt->data.for_stmt.body = body;
    
    return for_stmt;
}

static ASTNode* parse_return_statement(Compiler* comp) {
    Token* token = previous_token(comp);
    
    ASTNode* value = NULL;
    if (!check(comp, TOK_SEMICOLON) && !check(comp, TOK_RBRACE)) {
        value = parse_expression(comp);
    }
    
    expect(comp, TOK_SEMICOLON, "Expected ';' after return");
    
    ASTNode* ret = new_node(NODE_RETURN, token->line, token->column);
    ret->data.return_stmt.value = value;
    
    return ret;
}

static ASTNode* parse_expression_statement(Compiler* comp) {
    Token* token = current_token(comp);
    ASTNode* expr = parse_expression(comp);
    expect(comp, TOK_SEMICOLON, "Expected ';' after expression");
    
    ASTNode* stmt = new_node(NODE_EXPRESSION, token->line, token->column);
    stmt->children = allocate(sizeof(ASTNode*));
    stmt->children[0] = expr;
    stmt->child_count = 1;
    
    return stmt;
}

static ASTNode* parse_assignment(Compiler* comp) {
    Token* token = current_token(comp);
    char* name = my_strndup(token->start, token->length);
    advance_token(comp);
    
    Token* op_token = previous_token(comp);
    bool is_compound = (op_token->type == TOK_PLUS_ASSIGN || 
                        op_token->type == TOK_MINUS_ASSIGN ||
                        op_token->type == TOK_STAR_ASSIGN ||
                        op_token->type == TOK_SLASH_ASSIGN);
    
    ASTNode* value = parse_expression(comp);
    expect(comp, TOK_SEMICOLON, "Expected ';' after assignment");
    
    ASTNode* assign = new_node(NODE_ASSIGN, token->line, token->column);
    assign->data.assign.name = name;
    assign->data.assign.value = value;
    assign->data.assign.is_compound = is_compound;
    if (is_compound) {
        assign->data.assign.op = *op_token;
    }
    
    return assign;
}

static ASTNode* parse_block(Compiler* comp) {
    Token* token = previous_token(comp);
    
    ASTNode** statements = allocate(sizeof(ASTNode*) * 64);
    int count = 0;
    
    comp->scope_depth++;
    
    while (!check(comp, TOK_RBRACE) && !check(comp, TOK_EOF)) {
        statements[count++] = parse_statement(comp);
    }
    
    comp->scope_depth--;
    
    expect(comp, TOK_RBRACE, "Expected '}' after block");
    
    ASTNode* block = new_node(NODE_BLOCK, token->line, token->column);
    block->data.block.statements = statements;
    block->data.block.statement_count = count;
    
    return block;
}

static ASTNode* parse_function(Compiler* comp, bool is_method, bool is_constructor) {
    Token* token = previous_token(comp);
    
    // Function name (optional for constructors)
    char* name = NULL;
    if (!is_constructor && check(comp, TOK_IDENT)) {
        Token* name_token = current_token(comp);
        name = my_strndup(name_token->start, name_token->length);
        advance_token(comp);
    } else if (is_constructor) {
        name = str_dup("constructor");
    }
    
    // Return type (before parameters for Grape syntax)
    char* return_type = NULL;
    
    // Check for parameters
    bool has_brackets = match_token(comp, TOK_LBRACKET);
    bool has_parens = match_token(comp, TOK_LPAREN);
    
    ASTNode** params = allocate(sizeof(ASTNode*) * MAX_PARAMS);
    int param_count = 0;
    
    if (has_brackets || has_parens) {
        if (!check(comp, (has_brackets ? TOK_RBRACKET : TOK_RPAREN))) {
            do {
                if (param_count >= MAX_PARAMS) break;
                params[param_count++] = parse_expression(comp);
            } while (match_token(comp, TOK_COMMA));
        }
        
        if (has_brackets) {
            expect(comp, TOK_RBRACKET, "Expected ']' after parameters");
        } else {
            expect(comp, TOK_RPAREN, "Expected ')' after parameters");
        }
    }
    
    // Parse function body
    expect(comp, TOK_LBRACE, "Expected '{' before function body");
    ASTNode* body = parse_block(comp);
    
    ASTNode* func = new_node(NODE_FUNCTION, token->line, token->column);
    func->data.function.name = name;
    func->data.function.return_type = return_type;
    func->data.function.params = params;
    func->data.function.param_count = param_count;
    func->data.function.body = body;
    func->data.function.is_method = is_method;
    func->data.function.is_constructor = is_constructor;
    func->data.function.is_nested = (comp->scope_depth > 0);
    func->data.function.nesting_depth = comp->scope_depth;
    
    return func;
}

static ASTNode* parse_class(Compiler* comp) {
    Token* token = previous_token(comp);
    
    // Class name
    if (!check(comp, TOK_IDENT)) {
        report_error(current_token(comp)->line, "Expected class name");
        comp->parser.had_error = true;
        return NULL;
    }
    
    Token* name_token = current_token(comp);
    char* name = my_strndup(name_token->start, name_token->length);
    advance_token(comp);
    
    // Optional template parameter
    char* template_param = NULL;
    if (match_token(comp, TOK_LBRACKET)) {
        if (check(comp, TOK_IDENT)) {
            Token* t_token = current_token(comp);
            template_param = my_strndup(t_token->start, t_token->length);
            advance_token(comp);
        }
        expect(comp, TOK_RBRACKET, "Expected ']' after template parameter");
    }
    
    expect(comp, TOK_LBRACE, "Expected '{' before class body");
    
    ASTNode** members = allocate(sizeof(ASTNode*) * MAX_FIELDS);
    int member_count = 0;
    
    comp->current_class = name;
    comp->class_depth++;
    
    while (!check(comp, TOK_RBRACE) && !check(comp, TOK_EOF)) {
        // Parse visibility modifier
        bool is_public = false;
        if (match_token(comp, TOK_PUBLIC)) {
            is_public = true;
        } else if (match_token(comp, TOK_PRIVATE)) {
            is_public = false;
        } else {
            is_public = true;  // Default to public
        }
        
        if (check(comp, TOK_IDENT) || check(comp, TOK_VOID) || 
            check(comp, TOK_INT8) || check(comp, TOK_INT16) ||
            check(comp, TOK_INT32) || check(comp, TOK_INT64) ||
            check(comp, TOK_STRING_TYPE) || check(comp, TOK_BOOL_TYPE)) {
            
            Token* type_token = current_token(comp);
            char* type_name = my_strndup(type_token->start, type_token->length);
            advance_token(comp);
            
            if (check(comp, TOK_IDENT)) {
                Token* name_tok = current_token(comp);
                
                // Check if it's a constructor
                if (strcmp(name_tok->start, "this") == 0) {
                    advance_token(comp);
                    members[member_count++] = parse_function(comp, true, true);
                } else {
                    char* field_name = my_strndup(name_tok->start, name_tok->length);
                    advance_token(comp);
                    
                    // Check if it's a method
                    if (match_token(comp, TOK_LBRACKET) || match_token(comp, TOK_LPAREN)) {
                        // It's a method - need to re-parse as function
                        // For simplicity, create a function node
                        members[member_count++] = parse_function(comp, true, false);
                    } else if (match_token(comp, TOK_LBRACE)) {
                        // Property with getter
                        ASTNode* prop = parse_block(comp);
                        members[member_count++] = prop;
                    } else {
                        // Field declaration
                        expect(comp, TOK_SEMICOLON, "Expected ';' after field");
                        
                        ASTNode* field = new_node(NODE_VAR_DECL, name_tok->line, name_tok->column);
                        field->data.var_decl.name = field_name;
                        field->data.var_decl.type_name = type_name;
                        field->data.var_decl.initializer = NULL;
                        field->data.var_decl.is_const = false;
                        members[member_count++] = field;
                    }
                }
            }
        }
    }
    
    comp->class_depth--;
    comp->current_class = NULL;
    
    expect(comp, TOK_RBRACE, "Expected '}' after class body");
    
    ASTNode* class_def = new_node(NODE_CLASS, token->line, token->column);
    class_def->data.class_def.name = name;
    class_def->data.class_def.template_param = template_param;
    class_def->data.class_def.members = members;
    class_def->data.class_def.member_count = member_count;
    
    return class_def;
}

static ASTNode* parse_statement(Compiler* comp) {
    if (match_token(comp, TOK_VAR)) {
        return parse_var_decl(comp);
    }
    
    if (match_token(comp, TOK_IF)) {
        return parse_if_statement(comp);
    }
    
    if (match_token(comp, TOK_WHILE)) {
        return parse_while_statement(comp);
    }
    
    if (match_token(comp, TOK_FOR)) {
        return parse_for_statement(comp);
    }
    
    if (match_token(comp, TOK_RETURN)) {
        return parse_return_statement(comp);
    }
    
    if (match_token(comp, TOK_LBRACE)) {
        return parse_block(comp);
    }
    
    // Check for assignment
    if (check(comp, TOK_IDENT)) {
        Token* next = &comp->parser.tokens[comp->parser.current + 1];
        if (next->type == TOK_ASSIGN || next->type == TOK_PLUS_ASSIGN ||
            next->type == TOK_MINUS_ASSIGN || next->type == TOK_STAR_ASSIGN ||
            next->type == TOK_SLASH_ASSIGN) {
            return parse_assignment(comp);
        }
    }
    
    // Expression statement or function call
    return parse_expression_statement(comp);
}

static ASTNode* parse_declaration(Compiler* comp) {
    if (match_token(comp, TOK_CLASS)) {
        return parse_class(comp);
    }
    
    if (match_token(comp, TOK_USE)) {
        // #use directive - skip for now
        expect(comp, TOK_STRING, "Expected string after #use");
        expect(comp, TOK_SEMICOLON, "Expected ';' after #use");
        return NULL;
    }
    
    // Check for function declaration
    if (check(comp, TOK_INT8) || check(comp, TOK_INT16) || 
        check(comp, TOK_INT32) || check(comp, TOK_INT64) ||
        check(comp, TOK_VOID) || check(comp, TOK_STRING_TYPE) ||
        check(comp, TOK_BOOL_TYPE) || check(comp, TOK_IDENT)) {
        
        Token* type_token = current_token(comp);
        char* type_name = my_strndup(type_token->start, type_token->length);
        advance_token(comp);
        
        if (check(comp, TOK_IDENT)) {
            Token* name_token = current_token(comp);
            char* name = my_strndup(name_token->start, name_token->length);
            advance_token(comp);
            
            // Check for bracket-style parameters
            bool has_brackets = match_token(comp, TOK_LBRACKET);
            bool has_parens = match_token(comp, TOK_LPAREN);
            
            ASTNode** params = allocate(sizeof(ASTNode*) * MAX_PARAMS);
            int param_count = 0;
            
            if (has_brackets || has_parens) {
                if (!check(comp, (has_brackets ? TOK_RBRACKET : TOK_RPAREN))) {
                    do {
                        if (param_count >= MAX_PARAMS) break;
                        params[param_count++] = parse_expression(comp);
                    } while (match_token(comp, TOK_COMMA));
                }
                
                if (has_brackets) {
                    expect(comp, TOK_RBRACKET, "Expected ']' after parameters");
                } else {
                    expect(comp, TOK_RPAREN, "Expected ')' after parameters");
                }
            }
            
            expect(comp, TOK_LBRACE, "Expected '{' before function body");
            ASTNode* body = parse_block(comp);
            
            ASTNode* func = new_node(NODE_FUNCTION, type_token->line, type_token->column);
            func->data.function.name = name;
            func->data.function.return_type = type_name;
            func->data.function.params = params;
            func->data.function.param_count = param_count;
            func->data.function.body = body;
            func->data.function.is_method = false;
            func->data.function.is_constructor = false;
            func->data.function.is_nested = false;
            func->data.function.nesting_depth = 0;
            
            return func;
        }
    }
    
    return parse_statement(comp);
}

static ASTNode* parse_program(Compiler* comp) {
    ASTNode** declarations = allocate(sizeof(ASTNode*) * MAX_TOKENS);
    int count = 0;
    
    while (!check(comp, TOK_EOF)) {
        declarations[count++] = parse_declaration(comp);
    }
    
    ASTNode* program = new_node(NODE_PROGRAM, 0, 0);
    program->children = declarations;
    program->child_count = count;
    
    return program;
}

// ============================================================
// C CODE GENERATOR
// ============================================================

static void init_codegen(CodeGenerator* gen) {
    gen->capacity = 65536;
    gen->buffer = allocate(gen->capacity);
    gen->buffer[0] = '\0';
    gen->length = 0;
    gen->indent = 0;
    gen->needs_semicolon = false;
}

static void emit_indent(CodeGenerator* gen) {
    for (int i = 0; i < gen->indent; i++) {
        if (gen->length < gen->capacity - 1) {
            gen->buffer[gen->length++] = ' ';
        }
    }
}

static void emit(CodeGenerator* gen, const char* format, ...) {
    va_list args;
    va_start(args, format);
    
    if (gen->needs_semicolon && gen->length > 0 && gen->buffer[gen->length - 1] != '\n') {
        if (gen->length < gen->capacity - 1) {
            gen->buffer[gen->length++] = ';';
        }
        if (gen->length < gen->capacity - 1) {
            gen->buffer[gen->length++] = '\n';
        }
        gen->needs_semicolon = false;
    }
    
    emit_indent(gen);
    
    int written = vsnprintf(&gen->buffer[gen->length], gen->capacity - gen->length, format, args);
    gen->length += written;
    
    va_end(args);
}

static void emit_raw(CodeGenerator* gen, const char* str) {
    int len = strlen(str);
    if (gen->length + len >= gen->capacity) {
        gen->capacity *= 2;
        gen->buffer = realloc(gen->buffer, gen->capacity);
    }
    strcpy(&gen->buffer[gen->length], str);
    gen->length += len;
}

static void emit_line(CodeGenerator* gen, const char* format, ...) {
    va_list args;
    va_start(args, format);
    
    if (gen->needs_semicolon && gen->length > 0 && gen->buffer[gen->length - 1] != '\n') {
        if (gen->length < gen->capacity - 1) {
            gen->buffer[gen->length++] = ';';
        }
        if (gen->length < gen->capacity - 1) {
            gen->buffer[gen->length++] = '\n';
        }
        gen->needs_semicolon = false;
    }
    
    emit_indent(gen);
    
    int written = vsnprintf(&gen->buffer[gen->length], gen->capacity - gen->length, format, args);
    gen->length += written;
    
    if (gen->length < gen->capacity - 1) {
        gen->buffer[gen->length++] = '\n';
    }
    
    va_end(args);
}

static void emit_open_brace(CodeGenerator* gen) {
    emit_line(gen, "{");
    gen->indent++;
}

static void emit_close_brace(CodeGenerator* gen) {
    gen->indent--;
    emit_line(gen, "}");
}

static const char* get_c_type(const char* grape_type) {
    if (!grape_type) return "int";
    if (strcmp(grape_type, "int8") == 0 || strcmp(grape_type, "int8") == 0) return "int8_t";
    if (strcmp(grape_type, "int16") == 0) return "int16_t";
    if (strcmp(grape_type, "int32") == 0) return "int32_t";
    if (strcmp(grape_type, "int64") == 0) return "int64_t";
    if (strcmp(grape_type, "uint8") == 0) return "uint8_t";
    if (strcmp(grape_type, "uint16") == 0) return "uint16_t";
    if (strcmp(grape_type, "uint32") == 0) return "uint32_t";
    if (strcmp(grape_type, "uint64") == 0) return "uint64_t";
    if (strcmp(grape_type, "float32") == 0 || strcmp(grape_type, "float") == 0) return "float";
    if (strcmp(grape_type, "float64") == 0 || strcmp(grape_type, "double") == 0) return "double";
    if (strcmp(grape_type, "bool") == 0) return "bool";
    if (strcmp(grape_type, "string") == 0 || strcmp(grape_type, "String") == 0) return "char*";
    if (strcmp(grape_type, "void") == 0) return "void";
    return "int";  // Default
}

static void generate_expression(CodeGenerator* gen, ASTNode* node);

static void generate_statement(CodeGenerator* gen, ASTNode* node) {
    if (!node) return;
    
    switch (node->type) {
        case NODE_VAR_DECL: {
            const char* c_type = get_c_type(node->data.var_decl.type_name);
            if (node->data.var_decl.initializer) {
                emit_line(gen, "%s %s = ", c_type, node->data.var_decl.name);
                generate_expression(gen, node->data.var_decl.initializer);
                gen->needs_semicolon = true;
            } else {
                emit_line(gen, "%s %s;", c_type, node->data.var_decl.name);
            }
            break;
        }
        
        case NODE_ASSIGN: {
            if (node->data.assign.is_compound) {
                const char* op = "";
                switch (node->data.assign.op.type) {
                    case TOK_PLUS_ASSIGN: op = "+="; break;
                    case TOK_MINUS_ASSIGN: op = "-="; break;
                    case TOK_STAR_ASSIGN: op = "*="; break;
                    case TOK_SLASH_ASSIGN: op = "/="; break;
                    default: op = "="; break;
                }
                emit(gen, "%s %s ", node->data.assign.name, op);
            } else {
                emit(gen, "%s = ", node->data.assign.name);
            }
            generate_expression(gen, node->data.assign.value);
            gen->needs_semicolon = true;
            break;
        }
        
        case NODE_IF: {
            emit(gen, "if (");
            generate_expression(gen, node->data.if_stmt.condition);
            emit_raw(gen, ") ");
            emit_open_brace(gen);
            generate_statement(gen, node->data.if_stmt.then_branch);
            emit_close_brace(gen);
            
            if (node->data.if_stmt.else_branch) {
                emit_line(gen, "else ");
                emit_open_brace(gen);
                generate_statement(gen, node->data.if_stmt.else_branch);
                emit_close_brace(gen);
            }
            break;
        }
        
        case NODE_WHILE: {
            emit(gen, "while (");
            generate_expression(gen, node->data.while_stmt.condition);
            emit_raw(gen, ") ");
            emit_open_brace(gen);
            generate_statement(gen, node->data.while_stmt.body);
            emit_close_brace(gen);
            break;
        }
        
        case NODE_FOR: {
            const char* c_type = get_c_type(node->data.for_stmt.type_name);
            emit_line(gen, "for (int _i = 0; _i < %s_length; _i++) {", node->data.for_stmt.iterable->data.identifier.name);
            gen->indent++;
            emit_line(gen, "%s %s = %s[_i];", c_type, node->data.for_stmt.var_name, 
                     node->data.for_stmt.iterable->data.identifier.name);
            generate_statement(gen, node->data.for_stmt.body);
            gen->indent--;
            emit_line(gen, "}");
            break;
        }
        
        case NODE_RETURN: {
            if (node->data.return_stmt.value) {
                emit(gen, "return ");
                generate_expression(gen, node->data.return_stmt.value);
                gen->needs_semicolon = true;
            } else {
                emit_line(gen, "return;");
            }
            break;
        }
        
        case NODE_BLOCK: {
            for (int i = 0; i < node->data.block.statement_count; i++) {
                generate_statement(gen, node->data.block.statements[i]);
            }
            break;
        }
        
        case NODE_EXPRESSION: {
            if (node->child_count > 0) {
                generate_expression(gen, node->children[0]);
                gen->needs_semicolon = true;
            }
            break;
        }
        
        case NODE_FUNCTION: {
            // Function declaration handled separately
            break;
        }
        
        default: {
            generate_expression(gen, node);
            gen->needs_semicolon = true;
            break;
        }
    }
}

static void generate_expression(CodeGenerator* gen, ASTNode* node) {
    if (!node) return;
    
    switch (node->type) {
        case NODE_LITERAL: {
            if (node->data.literal.string_val) {
                emit(gen, "\"%s\"", node->data.literal.string_val);
            } else if (node->data.literal.float_val != 0) {
                emit(gen, "%f", node->data.literal.float_val);
            } else if (node->data.literal.bool_val) {
                emit_raw(gen, "true");
            } else {
                emit(gen, "%ld", node->data.literal.int_val);
            }
            break;
        }
        
        case NODE_IDENTIFIER: {
            emit_raw(gen, node->data.identifier.name);
            break;
        }
        
        case NODE_BINARY: {
            emit_raw(gen, "(");
            generate_expression(gen, node->data.binary.left);
            
            const char* op = " ";
            switch (node->data.binary.op.type) {
                case TOK_PLUS: op = " + "; break;
                case TOK_MINUS: op = " - "; break;
                case TOK_STAR: op = " * "; break;
                case TOK_SLASH: op = " / "; break;
                case TOK_PERCENT: op = " % "; break;
                case TOK_LT: op = " < "; break;
                case TOK_GT: op = " > "; break;
                case TOK_LTE: op = " <= "; break;
                case TOK_GTE: op = " >= "; break;
                case TOK_EQ: op = " == "; break;
                case TOK_NEQ: op = " != "; break;
                case TOK_AND: op = " && "; break;
                case TOK_OR: op = " || "; break;
                default: break;
            }
            emit_raw(gen, op);
            generate_expression(gen, node->data.binary.right);
            emit_raw(gen, ")");
            break;
        }
        
        case NODE_UNARY: {
            const char* op = "";
            switch (node->data.unary.op.type) {
                case TOK_MINUS: op = "-"; break;
                case TOK_NOT: op = "!"; break;
                default: break;
            }
            emit(gen, "(%s", op);
            generate_expression(gen, node->data.unary.operand);
            emit_raw(gen, ")");
            break;
        }
        
        case NODE_CALL: {
            generate_expression(gen, node->data.call.callee);
            emit_raw(gen, "(");
            for (int i = 0; i < node->data.call.arg_count; i++) {
                if (i > 0) emit_raw(gen, ", ");
                generate_expression(gen, node->data.call.args[i]);
            }
            emit_raw(gen, ")");
            break;
        }
        
        case NODE_MEMBER: {
            generate_expression(gen, node->data.member.object);
            emit(gen, ".%s", node->data.member.member);
            break;
        }
        
        case NODE_INDEX: {
            generate_expression(gen, node->data.index.object);
            emit_raw(gen, "[");
            generate_expression(gen, node->data.index.index);
            emit_raw(gen, "]");
            break;
        }
        
        case NODE_LAMBDA: {
            // Lambdas compiled as static functions with context
            emit_raw(gen, "/* lambda */");
            break;
        }
        
        case NODE_ARRAY: {
            emit_raw(gen, "{");
            for (int i = 0; i < node->data.array.element_count; i++) {
                if (i > 0) emit_raw(gen, ", ");
                generate_expression(gen, node->data.array.elements[i]);
            }
            emit_raw(gen, "}");
            break;
        }
        
        default:
            emit_raw(gen, "/* unknown expression */");
            break;
    }
}

static void generate_function(CodeGenerator* gen, ASTNode* node) {
    if (!node || node->type != NODE_FUNCTION) return;
    
    const char* return_type = get_c_type(node->data.function.return_type);
    const char* name = node->data.function.name ? node->data.function.name : "anonymous";
    
    emit(gen, "%s %s(", return_type, name);
    
    for (int i = 0; i < node->data.function.param_count; i++) {
        if (i > 0) emit_raw(gen, ", ");
        // Simplified parameter handling
        emit_raw(gen, "int param");
        emit(gen, "%d", i);
    }
    
    if (node->data.function.param_count == 0) {
        emit_raw(gen, "void");
    }
    
    emit_raw(gen, ") ");
    emit_open_brace(gen);
    
    generate_statement(gen, node->data.function.body);
    
    emit_close_brace(gen);
    emit_line(gen, "");
}

static void generate_class(CodeGenerator* gen, ASTNode* node) {
    if (!node || node->type != NODE_CLASS) return;
    
    const char* name = node->data.class_def.name;
    
    emit_line(gen, "typedef struct {");
    gen->indent++;
    
    // Generate fields from members
    for (int i = 0; i < node->data.class_def.member_count; i++) {
        ASTNode* member = node->data.class_def.members[i];
        if (member && member->type == NODE_VAR_DECL) {
            const char* c_type = get_c_type(member->data.var_decl.type_name);
            emit_line(gen, "%s %s;", c_type, member->data.var_decl.name);
        }
    }
    
    gen->indent--;
    emit_line(gen, "} %s;", name);
    emit_line(gen, "");
}

static char* generate_c_code(ASTNode* ast) {
    CodeGenerator gen;
    init_codegen(&gen);
    
    // Header
    emit_line(&gen, "// Generated by Grape Compiler");
    emit_line(&gen, "#include <stdio.h>");
    emit_line(&gen, "#include <stdlib.h>");
    emit_line(&gen, "#include <stdint.h>");
    emit_line(&gen, "#include <stdbool.h>");
    emit_line(&gen, "#include <string.h>");
    emit_line(&gen, "");
    
    // Generate classes first
    for (int i = 0; i < ast->child_count; i++) {
        ASTNode* node = ast->children[i];
        if (node && node->type == NODE_CLASS) {
            generate_class(&gen, node);
        }
    }
    
    // Forward declarations for functions
    for (int i = 0; i < ast->child_count; i++) {
        ASTNode* node = ast->children[i];
        if (node && node->type == NODE_FUNCTION) {
            const char* return_type = get_c_type(node->data.function.return_type);
            const char* name = node->data.function.name ? node->data.function.name : "anonymous";
            emit_line(&gen, "%s %s(void);", return_type, name);
        }
    }
    emit_line(&gen, "");
    
    // Generate functions
    for (int i = 0; i < ast->child_count; i++) {
        ASTNode* node = ast->children[i];
        if (node && node->type == NODE_FUNCTION) {
            generate_function(&gen, node);
        }
    }
    
    // Main function
    emit_line(&gen, "int main(void) {");
    gen.indent++;
    emit_line(&gen, "printf(\"Grape program executed!\\n\");");
    emit_line(&gen, "return 0;");
    gen.indent--;
    emit_line(&gen, "}");
    
    return gen.buffer;
}

// ============================================================
// MAIN COMPILER FUNCTION
// ============================================================

static char* compile(const char* source) {
    Compiler* comp = allocate(sizeof(Compiler));
    memset(comp, 0, sizeof(Compiler));
    
    // Tokenize
    int token_count = 0;
    g_compiler = comp;
    Token* tokens = tokenize(source, &token_count);
    
    if (tokens[0].type == TOK_ERROR) {
        fprintf(stderr, "Lexer error: %s\n", tokens[0].start);
        return NULL;
    }
    
    // Parse
    init_parser(comp, tokens, token_count);
    ASTNode* ast = parse_program(comp);
    
    if (comp->parser.had_error) {
        fprintf(stderr, "Parsing failed\n");
        return NULL;
    }
    
    // Generate C code
    char* c_code = generate_c_code(ast);
    
    return c_code;
}

// ============================================================
// PROGRAM ENTRY POINT
// ============================================================

int main(int argc, char** argv) {
    printf("Grape Compiler v0.1.0\n");
    printf("=====================\n\n");
    
    if (argc < 2) {
        fprintf(stderr, "Usage: %s <input.grape> [output.c]\n", argv[0]);
        return 1;
    }
    
    // Read input file
    FILE* input = fopen(argv[1], "r");
    if (!input) {
        fprintf(stderr, "Error: Cannot open file '%s'\n", argv[1]);
        return 1;
    }
    
    fseek(input, 0, SEEK_END);
    long file_size = ftell(input);
    fseek(input, 0, SEEK_SET);
    
    char* source = allocate(file_size + 1);
    fread(source, 1, file_size, input);
    source[file_size] = '\0';
    fclose(input);
    
    printf("Compiling '%s' (%ld bytes)...\n", argv[1], file_size);
    
    // Compile
    char* c_code = compile(source);
    
    if (!c_code) {
        fprintf(stderr, "Compilation failed\n");
        return 1;
    }
    
    // Write output
    const char* output_file = (argc >= 3) ? argv[2] : "output.c";
    FILE* output = fopen(output_file, "w");
    if (!output) {
        fprintf(stderr, "Error: Cannot write to '%s'\n", output_file);
        return 1;
    }
    
    fwrite(c_code, 1, strlen(c_code), output);
    fclose(output);
    
    printf("Successfully generated '%s'\n", output_file);
    printf("\nTo compile to native code:\n");
    printf("  gcc -o output %s -std=c11\n", output_file);
    
    return 0;
}
