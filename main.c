#include "al_alloc.h"
#include "al_eval.h"
#include "al_fileutils.h"
#include "al_lexer.h"
#include "al_reader.h"

int main() {
    Eval_Runtime eval = eval_init_from_file("test.al");
    next_token(eval.reader->lexer);

    AST_Node* node = read(eval.reader);
    Al_Object* obj = eval_ast(&eval, node);

    print_object(obj);
    destroy_object(obj);

    String* s = string_of_node_kind(node->kind);
    str_print(s);
    str_free(s);


    deinit_reader(eval.reader);
    ot_destroy(eval.toplevel->locals);
    dealloc(eval.toplevel);



    /*
    const AST_Node* param1 = node->as_funcall.func_params->as_param_list->params->data[0];
    if (param1 && param1->kind == nkStrLit) {
        str_free(param1->as_strlit);
    }*/

  //  dealloc(node->as_funcall.func_params->as_param_list->params->data);
  //  dealloc(node->as_funcall.func_params->as_param_list->params);


    return 0;
}
