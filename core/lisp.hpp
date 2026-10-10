// lisp.hpp
#pragma once
#include "python.hpp"
#include "math.hpp"

namespace lisp {

    // ----------------------------------------------------
    // 1. TOKENIZER (LEXER)
    // ----------------------------------------------------
    enum class TokenType { LPAREN, RPAREN, SYMBOL, NUMBER };

    struct Token {
        TokenType type;
        python::Str value;
    };

    inline bool is_digit(char c) { return c >= '0' && c <= '9'; }
    inline bool is_space(char c) { return c == ' ' || c == '\t' || c == '\n' || c == '\r'; }
    inline bool is_symbol_char(char c) { return !is_space(c) && c != '(' && c != ')' && c != '\0'; }

    inline python::List<Token> tokenize(python::Str code, python::Arena* arena) {
        python::List<Token> tokens = python::List<Token>::create(arena);
        unsigned long i = 0;

        while (i < code.len) {
            char c = code.ptr[i];
            if (is_space(c)) { i++; continue; }

            if (c == '(') {
                tokens.append(Token{ TokenType::LPAREN, python::Str{ code.ptr + i, 1 } });
                i++; continue;
            }
            if (c == ')') {
                tokens.append(Token{ TokenType::RPAREN, python::Str{ code.ptr + i, 1 } });
                i++; continue;
            }
            if (is_digit(c) || (c == '-' && i + 1 < code.len && is_digit(code.ptr[i + 1]))) {
                unsigned long start = i; i++;
                while (i < code.len && is_digit(code.ptr[i])) i++;
                tokens.append(Token{ TokenType::NUMBER, python::Str{ code.ptr + start, i - start } });
                continue;
            }
            if (is_symbol_char(c)) {
                unsigned long start = i;
                while (i < code.len && is_symbol_char(code.ptr[i])) i++;
                tokens.append(Token{ TokenType::SYMBOL, python::Str{ code.ptr + start, i - start } });
                continue;
            }
            i++;
        }
        return tokens;
    }

    // ----------------------------------------------------
    // 2. AST DÜĞÜM YAPISI (AST NODE)
    // ----------------------------------------------------
    enum class NodeKind { NUMBER, SYMBOL, LIST, NATIVE_FN };

    struct Environment; // Forward declaration
    struct Node;

    typedef Node* (*NativeFn)(python::List<Node*> args, Environment* env, python::Arena* arena);

    struct Node {
        NodeKind kind;
        union {
            long num_val;
            python::Str sym_val;
            python::List<Node*> list_val;
            NativeFn fn_val;
        };

        static Node* make_number(long num, python::Arena* arena) {
            Node* n = (Node*)arena->alloc(sizeof(Node));
            n->kind = NodeKind::NUMBER;
            n->num_val = num;
            return n;
        }

        static Node* make_symbol(python::Str sym, python::Arena* arena) {
            Node* n = (Node*)arena->alloc(sizeof(Node));
            n->kind = NodeKind::SYMBOL;
            n->sym_val = sym;
            return n;
        }

        static Node* make_list(python::Arena* arena) {
            Node* n = (Node*)arena->alloc(sizeof(Node));
            n->kind = NodeKind::LIST;
            n->list_val = python::List<Node*>::create(arena);
            return n;
        }

        static Node* make_native_fn(NativeFn fn, python::Arena* arena) {
            Node* n = (Node*)arena->alloc(sizeof(Node));
            n->kind = NodeKind::NATIVE_FN;
            n->fn_val = fn;
            return n;
        }
    };

    // ----------------------------------------------------
    // 3. PARSER (RECURSIVE DESCENT)
    // ----------------------------------------------------
    inline Node* parse_expression(python::List<Token> tokens, unsigned long* pos, python::Arena* arena) {
        if (*pos >= tokens.count) return nullptr;

        Token tok = tokens.items[*pos];

        if (tok.type == TokenType::NUMBER) {
            (*pos)++;
            long val = 0;
            bool neg = false;
            unsigned long idx = 0;
            if (tok.value.ptr[0] == '-') { neg = true; idx = 1; }
            for (; idx < tok.value.len; idx++) {
                val = val * 10 + (tok.value.ptr[idx] - '0');
            }
            if (neg) val = -val;
            return Node::make_number(val, arena);
        }

        if (tok.type == TokenType::SYMBOL) {
            (*pos)++;
            return Node::make_symbol(tok.value, arena);
        }

        if (tok.type == TokenType::LPAREN) {
            (*pos)++; // '(' karakterini atla
            Node* list_node = Node::make_list(arena);

            while (*pos < tokens.count && tokens.items[*pos].type != TokenType::RPAREN) {
                Node* child = parse_expression(tokens, pos, arena);
                if (child) list_node->list_val.append(child);
            }

            if (*pos < tokens.count && tokens.items[*pos].type == TokenType::RPAREN) {
                (*pos)++; // ')' karakterini atla
            }
            return list_node;
        }

        return nullptr;
    }

    // ----------------------------------------------------
    // 4. ENVIRONMENT (SEMBOL TABLOSU)
    // ----------------------------------------------------
    struct Binding {
        python::Str key;
        Node* value;
    };

    struct Environment {
        python::List<Binding> bindings;
        Environment* parent;

        static Environment* create(python::Arena* arena, Environment* parent_env = nullptr) {
            Environment* env = (Environment*)arena->alloc(sizeof(Environment));
            env->bindings = python::List<Binding>::create(arena);
            env->parent = parent_env;
            return env;
        }

        void set(python::Str key, Node* val) {
            for (unsigned long i = 0; i < bindings.count; i++) {
                if (bindings.items[i].key.len == key.len) {
                    bool match = true;
                    for (unsigned long j = 0; j < key.len; j++) {
                        if (bindings.items[i].key.ptr[j] != key.ptr[j]) { match = false; break; }
                    }
                    if (match) {
                        bindings.items[i].value = val;
                        return;
                    }
                }
            }
            bindings.append(Binding{ key, val });
        }

        Node* get(python::Str key) {
            for (unsigned long i = 0; i < bindings.count; i++) {
                if (bindings.items[i].key.len == key.len) {
                    bool match = true;
                    for (unsigned long j = 0; j < key.len; j++) {
                        if (bindings.items[i].key.ptr[j] != key.ptr[j]) { match = false; break; }
                    }
                    if (match) return bindings.items[i].value;
                }
            }
            if (parent) return parent->get(key);
            return nullptr;
        }
    };

    // Forward Declaration
    Node* eval(Node* node, Environment* env, python::Arena* arena);

    // ----------------------------------------------------
    // 5. YERLEŞİK (BUILT-IN) NATIVE FONKSİYONLAR
    // ----------------------------------------------------
    inline Node* fn_add(python::List<Node*> args, Environment* env, python::Arena* arena) {
        long acc = 0;
        for (unsigned long i = 0; i < args.count; i++) {
            Node* val = eval(args.items[i], env, arena);
            if (val && val->kind == NodeKind::NUMBER) acc += val->num_val;
        }
        return Node::make_number(acc, arena);
    }

    inline Node* fn_sub(python::List<Node*> args, Environment* env, python::Arena* arena) {
        if (args.count == 0) return Node::make_number(0, arena);
        Node* first = eval(args.items[0], env, arena);
        long acc = (first && first->kind == NodeKind::NUMBER) ? first->num_val : 0;
        if (args.count == 1) return Node::make_number(-acc, arena);

        for (unsigned long i = 1; i < args.count; i++) {
            Node* val = eval(args.items[i], env, arena);
            if (val && val->kind == NodeKind::NUMBER) acc -= val->num_val;
        }
        return Node::make_number(acc, arena);
    }

    inline Node* fn_mul(python::List<Node*> args, Environment* env, python::Arena* arena) {
        long acc = 1;
        for (unsigned long i = 0; i < args.count; i++) {
            Node* val = eval(args.items[i], env, arena);
            if (val && val->kind == NodeKind::NUMBER) acc *= val->num_val;
        }
        return Node::make_number(acc, arena);
    }

    inline Node* fn_print(python::List<Node*> args, Environment* env, python::Arena* arena) {
        for (unsigned long i = 0; i < args.count; i++) {
            Node* val = eval(args.items[i], env, arena);
            if (!val) continue;

            if (val->kind == NodeKind::NUMBER) {
                python::Str s = python::format_int(val->num_val, arena);
                python::sys_write(1, s.ptr, s.len);
            } else if (val->kind == NodeKind::SYMBOL) {
                python::sys_write(1, val->sym_val.ptr, val->sym_val.len);
            }
            if (i < args.count - 1) python::sys_write(1, " ", 1);
        }
        python::sys_write(1, "\n", 1);
        return Node::make_number(0, arena);
    }

    // Varsayılan Global Çevreyi Hazırla
    inline Environment* create_global_env(python::Arena* arena) {
        Environment* env = Environment::create(arena);
        env->set(python::Str{ "+", 1 }, Node::make_native_fn(fn_add, arena));
        env->set(python::Str{ "-", 1 }, Node::make_native_fn(fn_sub, arena));
        env->set(python::Str{ "*", 1 }, Node::make_native_fn(fn_mul, arena));
        env->set(python::Str{ "print", 5 }, Node::make_native_fn(fn_print, arena));
        return env;
    }

    // ----------------------------------------------------
    // 6. EVALUATOR (ÇALIŞTIRICI MOTOR)
    // ----------------------------------------------------
    inline Node* eval(Node* node, Environment* env, python::Arena* arena) {
        if (!node) return nullptr;

        // Literaller
        if (node->kind == NodeKind::NUMBER || node->kind == NodeKind::NATIVE_FN) {
            return node;
        }

        // Sembol Çözümleme
        if (node->kind == NodeKind::SYMBOL) {
            Node* val = env->get(node->sym_val);
            return val ? val : node;
        }

        // Liste / S-Expression Çalıştırma
        if (node->kind == NodeKind::LIST) {
            if (node->list_val.count == 0) return node;

            Node* head = node->list_val.items[0];

            // Özel Form: (def var_name value)
            if (head->kind == NodeKind::SYMBOL && head->sym_val.len == 3 &&
                head->sym_val.ptr[0] == 'd' && head->sym_val.ptr[1] == 'e' && head->sym_val.ptr[2] == 'f') {
                if (node->list_val.count >= 3) {
                    Node* var_sym = node->list_val.items[1];
                    Node* val_node = eval(node->list_val.items[2], env, arena);
                    if (var_sym->kind == NodeKind::SYMBOL) {
                        env->set(var_sym->sym_val, val_node);
                        return val_node;
                    }
                }
                return nullptr;
            }

            // Fonksiyon Çağrısı
            Node* fn_node = eval(head, env, arena);
            if (fn_node && fn_node->kind == NodeKind::NATIVE_FN) {
                // Argümanları topla (Head dışındakiler)
                python::List<Node*> args = python::List<Node*>::create(arena);
                for (unsigned long i = 1; i < node->list_val.count; i++) {
                    args.append(node->list_val.items[i]);
                }
                return fn_node->fn_val(args, env, arena);
            }
        }

        return nullptr;
    }

} // namespace lisp