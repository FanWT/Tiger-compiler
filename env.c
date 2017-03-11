#include <stdio.h>
#include "util.h"
#include "symbol.h"
#include "types.h"
#include "env.h"
#include "translate.h"

#include "frame.h"

/*Lab4: Your implementation of lab4*/

S_table E_base_tenv(void) {
    S_table init_t = S_empty();
    S_enter(init_t, S_Symbol("int"), Ty_Int());
    S_enter(init_t, S_Symbol("string"), Ty_String());
    return init_t;
}

S_table E_base_venv(void) {
    S_table t = S_empty();

    S_enter(
        t,
        S_Symbol("print"),
        E_FunEntry(Tr_outermost(), Temp_namedlabel("print"), Ty_TyList(Ty_String(), NULL), Ty_Void())
        );
    S_enter(
        t,
        S_Symbol("printi"),
        E_FunEntry(Tr_outermost(), Temp_namedlabel("printi"), Ty_TyList(Ty_Int(), NULL), Ty_Void())
        );
    S_enter(
        t,
        S_Symbol("flush"),
        E_FunEntry(Tr_outermost(), Temp_namedlabel("flush"), NULL, Ty_Void())
        );
    S_enter(
        t,
        S_Symbol("getchar"),
        E_FunEntry(Tr_outermost(), Temp_namedlabel("getchar"), NULL, Ty_String())
        );
    S_enter(
        t,
        S_Symbol("ord"),
        E_FunEntry(Tr_outermost(), Temp_namedlabel("ord"), Ty_TyList(Ty_String(), NULL), Ty_Int())
        );
    S_enter(
        t,
        S_Symbol("chr"),
        E_FunEntry(Tr_outermost(), Temp_namedlabel("chr"), Ty_TyList(Ty_Int(), NULL), Ty_String())
        );
    // S_enter(
    //     t,
    //     S_Symbol("allocRecord"),
    //     E_FunEntry(Tr_outermost(), Temp_namedlabel("allocRecord"), Ty_TyList(Ty_Int(), NULL), Ty_Int())
    //     );
    S_enter(
        t,
        S_Symbol("size"),
        E_FunEntry(Tr_outermost(), Temp_namedlabel("size"), Ty_TyList(Ty_String(), NULL), Ty_Int())
        );
    S_enter(
        t,
        S_Symbol("substring"),
        E_FunEntry(
            Tr_outermost(),
            Temp_namedlabel("substring"),
            Ty_TyList(Ty_String(), Ty_TyList(Ty_Int(), Ty_TyList(Ty_Int(), NULL))),
            Ty_String())
        );
    S_enter(
        t,
        S_Symbol("concat"),
        E_FunEntry(
            Tr_outermost(),
            Temp_namedlabel("concat"),
            Ty_TyList(Ty_String(), Ty_TyList(Ty_String(), NULL)),
            Ty_String())
        );
    S_enter(
        t,
        S_Symbol("not"),
        E_FunEntry(Tr_outermost(), Temp_namedlabel("not"), Ty_TyList(Ty_Int(), NULL), Ty_Int())
        );
    S_enter(
        t,
        S_Symbol("exit"),
        E_FunEntry(Tr_outermost(), Temp_namedlabel("exit"), Ty_TyList(Ty_Int(), NULL), Ty_Void())
        );
    return t;
}

E_enventry E_VarEntry(Tr_access a, Ty_ty ty) {
    E_enventry final;
    final = checked_malloc(sizeof(*final));
    final->kind = E_varEntry;
    final->u.var.access = a;
    final->u.var.ty = ty;
    return final;
}

E_enventry E_FunEntry(Tr_level level, Temp_label label, Ty_tyList fms, Ty_ty resl) {
    E_enventry final = checked_malloc(sizeof(*final));
    final->kind = E_funEntry;
    final->u.fun.formals = fms;
    final->u.fun.level = level;
    final->u.fun.label = label;
    final->u.fun.result  = resl;
    return final;
}
