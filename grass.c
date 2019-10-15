#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define BUF_MAX 1024

struct node {
    size_t size;
    void *value;
};

struct list {
    struct list *next;
    struct node *node;
};

struct appl {
    int type; // always 0
    int arg;
    int fun;
};

struct abst {
    int type; // always 1
    int argc;
    struct list *app_list;
};

struct insn {
    int type;
    int fun_argc;
    void *arg_appl;
};

struct value {
    int type; // always 0 for non-ce
    struct list* (*apply)(struct value *, struct value *, struct list *);
    char (*getch)(struct value *);
    struct list *ptr1; // only for ce
    struct list *ptr2; // only for ce
};

struct ce {
    int type; // always 1
    void (*apply)(struct value *);
    char (*getch)();
    struct list *code;
    struct list *env;
};

/*
 * Start of list structure functions
 */
int append_to_list(size_t node_size, void *node, struct list *head) {
    if (!head->next) {
        struct node *next_node = malloc(sizeof(struct node));
        memset(next_node, 0, sizeof(struct node));
        next_node->value = node;
        next_node->size = node_size;

        if (!head->node) {
            head->node = next_node;
        } else {
            head->next = malloc(sizeof(struct list));
            memset(head->next, 0, sizeof(struct list));
            head->next->node = next_node;
        }
        return 1;
    } else {
        return append_to_list(node_size, node, head->next);
    }
}

struct list* push_to_stack(size_t node_size, void *node, struct list *stack) {
    struct node *next_node = malloc(sizeof(struct node));
    next_node->value = node;
    next_node->size = node_size;

    if (!stack) {
        stack = malloc(sizeof(struct list));
    }
    if (!stack->node) {
        stack->node = next_node;

        return stack;
    } else {
        struct list *new_node = malloc(sizeof(struct list));
        new_node->next = stack;
        new_node->node = next_node;

        return new_node;
    }
}

struct list* remove_top_of_stack(struct list *stack) {
    if (!stack->next) {
        return (void *) 0;
    } else {
        return stack->next;
    }
}

void* stack_get(int index, int index_current, struct list *head) {
    if (index_current == index) {
        return head->node->value;
    } else {
        if (!head->next) {
            return (void *) 0;
        } else {
            return stack_get(index, index_current + 1, head->next);
        }
    }
}

void* list_get(int index, struct list *head) {
    if (index == 0) {
        return head->node->value;
    } else {
        if (!head->next) {
            return (void *) 0;
        } else {
            return list_get(index - 1, head->next);
        }
    }
}

struct list* copy_list(struct list *head) {
    if (!head->node) {
        return malloc(sizeof(struct list));
    }

    struct list *ret = malloc(sizeof(struct list));
    void *cpy_value = malloc(head->node->size);
    struct node *cpy_node = malloc(sizeof(struct node));
    memcpy(cpy_value, head->node->value, head->node->size);
    cpy_node->size = head->node->size;
    cpy_node->value = cpy_value;
    ret->node = cpy_node;

    if (head->next) {
        ret->next = copy_list(head->next);
    } 
    return ret;
}
/*
 * End of list structure functions
 */

char getch_error(struct value *error) {
    fprintf(stderr, "Runtime error at getch.\n");
    exit(-1);
}

char getch_succ(struct value *val) {
    return (char) val->ptr1;
}

/*
 * CE encoding boolean false
 */
struct ce* ce_false() {
    struct list *fcode = malloc(sizeof(struct list));
    struct abst *fabst = malloc(sizeof(struct abst));
    fabst->type = 1;
    fabst->argc = 1;
    fabst->app_list = malloc(sizeof(struct list));
    append_to_list(sizeof(struct list), fabst, fcode);

    struct ce *fce = malloc(sizeof(struct ce));
    fce->type = 1;
    fce->code = fcode;
    fce->env = malloc(sizeof(struct list));
    fce->getch = &getch_error;

    return fce;
}

/*
 * CE encoding boolean true
 */
struct ce* ce_true() {
    struct list *tcode = malloc(sizeof(struct list));
    struct abst *tabst = malloc(sizeof(struct abst));
    tabst->type = 1;
    tabst->argc = 1;
    tabst->app_list = malloc(sizeof(struct list));
    struct appl *tapp = malloc(sizeof(struct appl));
    tapp->type = 0;
    tapp->fun = 2;
    tapp->arg = 3;
    append_to_list(sizeof(struct appl), tapp, tabst->app_list);
    append_to_list(sizeof(struct list), tabst, tcode);

    struct ce *envce = malloc(sizeof(struct ce));
    envce->type = 1;
    envce->code = malloc(sizeof(struct list));
    envce->env = malloc(sizeof(struct list));

    struct ce *tce = malloc(sizeof(struct ce));
    tce->type = 1;
    tce->code = tcode;
    tce->env = malloc(sizeof(struct list));
    tce->env = push_to_stack(sizeof(struct ce), envce, tce->env);
    tce->getch = &getch_error;

    return tce;
}

/*
 * character primitive
 */
struct list* ch(struct value *fun, struct value *val, struct list *env) {
    char x = val->getch(val);
    char y = fun->getch(fun);
    if (x == y) {
        env = push_to_stack(sizeof(struct ce), ce_true(), env);
    } else {
        env = push_to_stack(sizeof(struct ce), ce_false(), env);
    }
    return env;
}

/*
 * OUT primitive
 */
struct list* out(struct value *fun, struct value *val, struct list *env) {
    char x = val->getch(val);
    printf("%c", x);
    env = push_to_stack(sizeof(struct value), val, env);
    return env;
}

/*
 * IN primitive
 */
struct list* in(struct value *fun, struct value *val, struct list *env) {
    char x = getchar();
    int c; while ((c = getchar()) != '\n' && c != EOF) { } //flush stdin

    if (x == -1) {
        env = push_to_stack(sizeof(struct value), val, env);
    } else {
        struct value *newch = malloc(sizeof(struct value));
        newch->type = 0;
        newch->apply = &ch;
        newch->ptr1 = x;
        newch->getch = &getch_succ;
        env = push_to_stack(sizeof(struct value), newch, env);
    }
    return env;
}

/*
 * Successor primitive
 */
struct list* succ(struct value *fun, struct value *val, struct list *env) {
    char x = val->getch(val);
    if (x == 255) {
        x = 0;
    } else {
        x = x + 1;
    }
    struct value *newch = malloc(sizeof(struct value));
    newch->type = 0;
    newch->apply = &ch;
    newch->ptr1 = x;
    newch->getch = &getch_succ;
    env = push_to_stack(sizeof(struct value), newch, env);
    return env;
}

/*
 * Parses a single application
 */
struct appl* parse_app(int *i_ptr, char *buf) {
    int fun = 0;
    int arg = 0;

    for (;;) {
        if (buf[*i_ptr] == 'W') {
            if (arg == 0) {
                fun = fun + 1;
            } else {
                break;
            }
        } else if (buf[*i_ptr] == 'w') {
            arg = arg + 1;
        } else if (buf[*i_ptr] == 'v') {
            break;
        } else if (buf[*i_ptr] == 0) {
            break;
        }
        *i_ptr = *i_ptr + 1;
    }

    struct appl *ret = malloc(sizeof(struct appl));
    ret->type = 0;
    ret->arg = arg;
    ret->fun = fun;
    return ret;
}

/*
 * Parses an application list
 */
struct list* parse_app_list(int *i_ptr, char *buf) {
    struct appl *app_curr;
    struct list *app_list = malloc(sizeof(struct list));
    memset(app_list, 0, sizeof(struct list));

    for (;;) {
        if (buf[*i_ptr] == 'W') {
            app_curr = parse_app(i_ptr, buf);
            append_to_list(sizeof(struct appl), app_curr, app_list);
        } else {
            break;
        }
    }

    return app_list;
}

/*
 * Parses an abstraction
 */
struct abst* parse_abs(int *i_ptr, char *buf) {
    int args = 0;
    struct list *app_list;

    for (;;) {
        if (buf[*i_ptr] == 'w') {
            args = args + 1;
        } else if (buf[*i_ptr] == 'W') {
            app_list = parse_app_list(i_ptr, buf);
            break;
        } else if ((buf[*i_ptr] == 'v') || (buf[*i_ptr] == 0)) {
            if (!app_list) {
                fprintf(stderr, "Parsing error!\n");
                exit(-1);
            }
            break;
        }
        *i_ptr = *i_ptr + 1;
    }

    struct abst *ret = malloc(sizeof(struct abst));
    ret->type = 1;
    ret->argc = args;
    ret->app_list = app_list;
    return ret;
}

/*
 * Processes grass string
 */
int run(char *input) {
    struct list *code = malloc(sizeof(struct list));
    memset(code, 0, sizeof(struct list));

    int i = 0;
    while (input[i] > 0) {
        if (input[i] == 'w') {
            append_to_list(sizeof(struct abst), parse_abs(&i, input), code);
        } else if (input[i] == 'W') {
            append_to_list(sizeof(struct appl), parse_app(&i, input), code);
        } else {
            i = i + 1;
        }
    }

    if (!code->node) {
        return 0; // no grass code was specified
    }

    // Initialize stack with primitives
    struct list *env = malloc(sizeof(struct list));
    struct value *val_in = malloc(sizeof(struct value));
    val_in->type = 0;
    val_in->apply = &in;
    val_in->getch = &getch_error;
    env = push_to_stack(sizeof(struct value), val_in, env);
    struct value *val_ch = malloc(sizeof(struct value));
    val_ch->type = 0;
    val_ch->apply = &ch;
    val_ch->ptr1 = 'w';
    val_ch->getch = &getch_succ;
    env = push_to_stack(sizeof(struct value), val_ch, env);
    struct value *val_succ = malloc(sizeof(struct value));
    val_succ->type = 0;
    val_succ->apply = &succ;
    val_succ->getch = &getch_error;
    env = push_to_stack(sizeof(struct value), val_succ, env);
    struct value *val_out = malloc(sizeof(struct value));
    val_out->type = 0;
    val_out->apply = &out;
    val_out->getch = &getch_error;
    env = push_to_stack(sizeof(struct value), val_out, env);

    // Initialize dump
    struct appl *dump_code_init = malloc(sizeof(struct appl));
    dump_code_init->arg = 1;
    dump_code_init->fun = 1;
    dump_code_init->type = 0;

    struct list *dump_code_init_list = malloc(sizeof(struct list));
    append_to_list(sizeof(struct appl), dump_code_init, dump_code_init_list);

    struct ce *dump_init = malloc(sizeof(struct ce));
    dump_init->code = dump_code_init_list;
    dump_init->env = malloc(sizeof(struct list));
    dump_init->type = 1;
    dump_init->getch = &getch_error;

    struct list *dump = malloc(sizeof(struct list));
    dump = push_to_stack(sizeof(struct ce), dump_init, dump);

    // Evaluation Loop
    for (;;) {
        if (!code) { // Restore dump
            if (!dump) {
                break; // You are done
            } else {
                struct ce *dumpce = stack_get(0, 0, dump);
                dump = remove_top_of_stack(dump);
                struct value *env_first = stack_get(0, 0, env);

                code = dumpce->code;
                env = dumpce->env;

                env = push_to_stack(sizeof(struct value), env_first, env);
            }
        } else {
            struct insn *ins = code->node->value;
            code = code->next;
            if (ins->type == 0) { // Evaluating Application
                struct appl *ins_appl = (struct appl *) ins;

                struct value *fun_val = (struct value *) stack_get(ins_appl->fun - 1, 0, env);
                struct value *arg_val = (struct value *) stack_get(ins_appl->arg - 1, 0, env);

                if (fun_val->type == 1) { // Applying CE
                    struct ce *fun = (struct ce *) fun_val;
                    struct ce *dumpce = malloc(sizeof(struct ce));
                    dumpce->code = code;
                    dumpce->env = env;
                    dumpce->type = 1;
                    dumpce->getch = &getch_error;

                    dump = push_to_stack(sizeof(struct ce), dumpce, dump);
                    code = copy_list(fun->code);
                    env = copy_list(fun->env);

                    env = push_to_stack(sizeof(struct value), arg_val, env);
                } else if (fun_val->type == 0) { // Applying Non-CE
                    env = fun_val->apply(fun_val, arg_val, env);
                }
            } else if (ins->type == 1) { // Evaluating Abstraction
                struct abst *ins_abst = (struct abst *) ins;

                if (ins_abst->argc == 1) { // Abstraction has only one argument
                    struct ce *new_ce = malloc(sizeof(struct ce));
                    new_ce->type = 1;
                    new_ce->code = ins_abst->app_list;
                    new_ce->env = copy_list(env);
                    new_ce->getch = &getch_error;

                    env = push_to_stack(sizeof(struct ce), new_ce, env);
                } else { // Abstraction has more than one argument
                    struct abst *new_abst = malloc(sizeof(struct abst));
                    new_abst->argc = ins_abst->argc - 1;
                    new_abst->app_list = copy_list(ins_abst->app_list);
                    new_abst->type = 1;

                    struct list *new_list = malloc(sizeof(struct list));
                    append_to_list(sizeof(struct abst), new_abst, new_list);

                    struct ce *new_ce = malloc(sizeof(struct ce));
                    new_ce->code = new_list;
                    new_ce->env = copy_list(env);
                    new_ce->type = 1;
                    new_ce->getch = &getch_error;

                    env = push_to_stack(sizeof(struct ce), new_ce, env);
                }
            }
        }
    }
}

int main(int argc, char** argv) {
    char* usage = "Usage: ./grass file | -i\n";
    if (argc == 2) {
        if (!(strncmp(argv[1], "-i", 2))) {
            char buffer_in[BUF_MAX];
            printf(">> ");
            while (fgets(buffer_in, BUF_MAX, stdin)) {
                run(buffer_in);
                printf("\n>> ");
            }
            puts("");
        } else {
            FILE *fd = fopen(argv[1], "rb");
            if (!fd) {
                fprintf(stderr, "File not found!\n");
                exit(-1);
            }
            fseek(fd, 0, SEEK_END);
            int len = ftell(fd) + 1;
            fseek(fd, 0, SEEK_SET);
            char buffer_in[len];
            fgets(buffer_in, len, fd);
            buffer_in[len] = 0;
            fclose(fd);
            run(buffer_in);
            puts("");
        }
    } else {
        fprintf(stderr, usage);
    }
}