#include <stdio.h>
#include "util.h"
#include "errormsg.h"
#include "symbol.h"
#include "absyn.h"
#include "types.h"
#include "env.h"
#include "temp.h"
#include "tree.h"
#include "frame.h"
#include "semant.h"
#include "translate.h"
#include "string.h"

struct expty {Tr_exp exp; Ty_ty ty; };

static struct expty transExp(Tr_level level, Tr_exp breakk, S_table venv, S_table tenv, A_exp a);
static struct expty transVar(Tr_level level, Tr_exp breakk, S_table venv, S_table tenv, A_var v);
static Tr_exp       transDec(Tr_level level, Tr_exp breakk, S_table venv, S_table tenv, A_dec d);
static Ty_ty transTy(S_table tenv, A_ty t);

static struct expty expTy(Tr_exp exp, Ty_ty ty);

static Ty_tyList makeFormalTys(S_table tenv, A_fieldList params);
static Ty_fieldList makeFieldTys(S_table tenv, A_fieldList fields);
static U_boolList makeFormals(A_fieldList params);
static Ty_ty actual_ty(Ty_ty ty);
static int ty_match(Ty_ty tType, Ty_ty eType);

string readonly_var = "";

static struct expty expTy(Tr_exp exp, Ty_ty ty)
{
    struct expty e;
    e.exp = exp; e.ty = ty;
    return e;
}


F_fragList SEM_transProg(A_exp exp)
{
    //printf("SEM_transProg begin\n");
    S_table tenv = E_base_tenv();
    S_table venv = E_base_venv();
    struct expty et;
    et = transExp(Tr_outermost(), NULL, venv, tenv, exp);
    Tr_procEntryExit(Tr_outermost(), et.exp, NULL);
    //printf("SEM_transProg end\n");
    return Tr_getResult();
}

static struct expty transExp(Tr_level level, Tr_exp breakk, S_table venv, S_table tenv, A_exp a)
{
    //printf("die in transExp\n");
    switch (a->kind) {
    case A_varExp:
    {
        return transVar(level, breakk, venv, tenv, a->u.var);
    }
    case A_nilExp:
    {
        return expTy(Tr_nilExp(), Ty_Nil());
    }
    case A_intExp:
    {
        return expTy(Tr_intExp(a->u.intt), Ty_Int());
    }
    case A_stringExp:
    {
        return expTy(Tr_stringExp(a->u.stringg), Ty_String());
    }
    case A_callExp:
    {
        A_expList args = NULL;
        Ty_tyList formals;
        E_enventry x = S_look(venv, a->u.call.func);
        Tr_exp translation = Tr_noExp();
        Tr_expList argList = Tr_ExpList();
        if (x && x->kind == E_funEntry) {
            // check type of formals
            formals = x->u.fun.formals;
            int count = 0;
            for (args = a->u.call.args; args && formals;
                 args = args->tail, formals = formals->tail)
            {
                count++;
                struct expty arg = transExp(level, breakk, venv, tenv, args->head);
                if (!ty_match(arg.ty, formals->head))
                    EM_error(args->head->pos, "para type mismatch");
                Tr_ExpList_append(argList, arg.exp);
            }
            //printf("\n\ncount %d\n\n",count);
            if (args == NULL && formals != NULL)
                EM_error(a->pos, "not enough arguments");
            else if (args != NULL && formals == NULL)
                EM_error(a->pos, "too many arguments");
            translation = Tr_callExp(level, x->u.fun.level, x->u.fun.label, argList);
            //printf("\n\n\n%s\n\n\n",x->u.fun.label)
            return expTy(translation, actual_ty(x->u.fun.result));
        }
        else {
            EM_error(a->pos, "undefined function %s", S_name(a->u.call.func));
            return expTy(translation, Ty_Int());
        }
    }
    case A_opExp:
    {
        A_oper oper = a->u.op.oper;
        struct expty left  = transExp(level, breakk, venv, tenv, a->u.op.left);
        struct expty right = transExp(level, breakk, venv, tenv, a->u.op.right);
        Tr_exp translation = Tr_noExp();
        switch (oper) {
        case A_plusOp:
        case A_minusOp:
        case A_timesOp:
        case A_divideOp:
            if (left.ty->kind != Ty_int)
                EM_error(a->u.op.left->pos, "integer required");
            if (right.ty->kind != Ty_int)
                EM_error(a->u.op.right->pos,"integer required");
            return expTy(Tr_arithExp(oper, left.exp, right.exp), Ty_Int());
        case A_eqOp:
        case A_neqOp:
            switch(left.ty->kind) {
            case Ty_int:
                if (ty_match(right.ty, left.ty))
                    translation = Tr_eqExp(oper, left.exp, right.exp);
                break;
            case Ty_string:
                if (ty_match(right.ty, left.ty))
                    translation = Tr_eqStringExp(oper, left.exp, right.exp);
                break;
            case Ty_array:
            {
                if (right.ty->kind != left.ty->kind) {
                    EM_error(a->u.op.right->pos,"same type required");
                }
                translation = Tr_eqRef(oper, left.exp, right.exp);
                break;
            }
            case Ty_record:
            {
                if (right.ty->kind != Ty_record && right.ty->kind != Ty_nil) {
                    EM_error(a->u.op.right->pos,"same type required");
                }
                translation = Tr_eqRef(oper, left.exp, right.exp);
                break;
            }
            default:
            {
            }
            }
            return expTy(translation, Ty_Int());
        case A_ltOp:
        case A_leOp:
        case A_gtOp:
        case A_geOp:
        {
            if (right.ty->kind != left.ty->kind) {
                EM_error(a->u.op.right->pos,"same type required");
            }
            switch(left.ty->kind) {
            case Ty_int:
                translation = Tr_relExp(oper, left.exp, right.exp); break;
            case Ty_string:
                translation = Tr_noExp(); break;
            default:
            {
                translation = Tr_noExp();
            }
            }
            return expTy(translation, Ty_Int());
        }
        }
        assert(0 && "Invalid operator in expression");
        return expTy(Tr_noExp(), Ty_Int());
    }
    case A_recordExp:
    {
        Ty_ty typ = actual_ty(S_look(tenv, a->u.record.typ));
        if (!typ) {
            EM_error(a->pos, "undefined type %s", S_name(a->u.record.typ));
            return expTy(Tr_noExp(), Ty_Record(NULL));
        }
        Ty_fieldList fieldTys = typ->u.record;
        A_efieldList recList;
        Tr_expList list = Tr_ExpList();
        int n = 0;
        for (recList = a->u.record.fields; recList;
             recList = recList->tail, fieldTys = fieldTys->tail, n++) {
            struct expty e = transExp(level, breakk, venv, tenv, recList->head->exp);
            if (recList->head->name != fieldTys->head->name)
                EM_error(a->pos, "%s not a valid field name", recList->head->name);
            Tr_ExpList_prepend(list, e.exp);
        }
        Tr_access access = Tr_allocLocal(level,TRUE);
        return expTy(Tr_recordExp(n, list, access), typ);
    }
    case A_seqExp:
    {
        struct expty expr = expTy(Tr_noExp(), Ty_Void());     /* empty seq case */
        A_expList seq;
        Tr_expList list = Tr_ExpList();
        for (seq = a->u.seq; seq; seq = seq->tail) {
            expr = transExp(level, breakk, venv, tenv, seq->head);
            Tr_ExpList_prepend(list, expr.exp);
        }
        if (Tr_ExpList_empty(list))
            Tr_ExpList_prepend(list, expr.exp);
        return expTy(Tr_seqExp(list), expr.ty);
    }
    case A_assignExp:
    {
        struct expty var = transVar(level, breakk, venv, tenv, a->u.assign.var);
        struct expty exp = transExp(level, breakk, venv, tenv, a->u.assign.exp);
        if (!ty_match(var.ty, exp.ty))
            EM_error(a->u.assign.exp->pos, "unmatched assign exp");
        if(a->u.assign.var->kind == A_simpleVar) {
            if(strlen(readonly_var) && !strcmp(S_name(a->u.assign.var->u.simple), readonly_var)) {
                EM_error(a->pos, "loop variable can't be assigned");
                readonly_var = ""; // should this operation be done in case_for or this case ?
            }
        }
        return expTy(Tr_assignExp(var.exp, exp.exp), Ty_Void());
    }
    case A_ifExp:
    {
        struct expty test, then, elsee;
        test = transExp(level, breakk, venv, tenv, a->u.iff.test);
        if (test.ty->kind != Ty_int)
            EM_error(a->u.iff.test->pos, "test should be integer");
        then = transExp(level, breakk, venv, tenv, a->u.iff.then);
        if (a->u.iff.elsee) {
            elsee = transExp(level, breakk, venv, tenv, a->u.iff.elsee);
            if (a->u.iff.elsee->kind != A_nilExp&&!ty_match(then.ty, elsee.ty)) {
                EM_error(a->u.iff.elsee->pos, "then exp and else exp type mismatch");
            }
            return expTy(Tr_ifExp(test.exp, then.exp, elsee.exp), then.ty);
        } else {
            if (then.ty->kind != Ty_void)
                EM_error(a->u.iff.then->pos, "if-then exp's body must produce no value");
            return expTy(Tr_ifExp(test.exp, then.exp, NULL), Ty_Void());
        }
    }
    case A_whileExp:
    {
        struct expty test = transExp(level, breakk, venv, tenv, a->u.whilee.test);
        if (test.ty->kind != Ty_int)
            EM_error(a->u.whilee.test->pos, "test should be integer");
        Tr_exp newBreakk = Tr_doneExp();
        struct expty body = transExp(level, newBreakk, venv, tenv, a->u.whilee.body);
        if (body.ty->kind != Ty_void)
            EM_error(a->u.whilee.body->pos, "while body must produce no value");
        return expTy(Tr_whileExp(test.exp, newBreakk, body.exp), Ty_Void());
    }
    case A_forExp:
    {
        /* Convert a for loop into a let expression with a while loop */
        A_dec i = A_VarDec(a->pos, a->u.forr.var, NULL, a->u.forr.lo);
        A_dec limit = A_VarDec(a->pos, S_Symbol("limit"), NULL, a->u.forr.hi);
        A_dec test = A_VarDec(a->pos, S_Symbol("test"), NULL, A_IntExp(a->pos, 1));
        A_exp testExp = A_VarExp(a->pos, A_SimpleVar(a->pos, S_Symbol("test")));
        A_exp iExp = A_VarExp(a->pos, A_SimpleVar(a->pos, a->u.forr.var));
        A_exp limitExp = A_VarExp(a->pos, A_SimpleVar(a->pos, S_Symbol("limit")));
        A_exp increment = A_AssignExp(a->pos,
                                      A_SimpleVar(a->pos, a->u.forr.var),
                                      A_OpExp(a->pos, A_plusOp, iExp, A_IntExp(a->pos, 1)));
        A_exp setFalse = A_AssignExp(a->pos,
                                     A_SimpleVar(a->pos, S_Symbol("test")), A_IntExp(a->pos, 0));
        /* The let expression we pass to this function */
        A_exp letExp = A_LetExp(a->pos,
                                A_DecList(i, A_DecList(limit, A_DecList(test, NULL))),
                                A_SeqExp(a->pos,
                                         A_ExpList(
                                             A_IfExp(a->pos,
                                                     A_OpExp(a->pos, A_leOp, a->u.forr.lo, a->u.forr.hi),
                                                     A_WhileExp(a->pos, testExp,
                                                                A_SeqExp(a->pos,
                                                                         A_ExpList(a->u.forr.body,
                                                                                   A_ExpList(
                                                                                       A_IfExp(a->pos,
                                                                                               A_OpExp(a->pos, A_ltOp, iExp,
                                                                                                       limitExp),
                                                                                               increment, setFalse),
                                                                                       NULL)))),
                                                     NULL),
                                             NULL)));
        struct expty e = transExp(level, breakk, venv, tenv, letExp);
        return e;
    }
    case A_breakExp:
    {
        Tr_exp translation = Tr_noExp();
        if (!breakk) {
            EM_error(a->pos, "illegal break expression; must be inside loop construct");
        } else {
            translation = Tr_breakExp(breakk);
        }
        return expTy(translation, Ty_Void());
    }
    case A_letExp:
    {
        struct expty expr;
        A_decList d;
        Tr_expList list = Tr_ExpList();
        S_beginScope(venv);
        S_beginScope(tenv);
        for (d = a->u.let.decs; d; d = d->tail)
            Tr_ExpList_prepend(list, transDec(level, breakk, venv, tenv, d->head));
        expr = transExp(level, breakk, venv, tenv, a->u.let.body);
        Tr_ExpList_prepend(list, expr.exp);     // need result of let at the beginning
        S_endScope(venv);
        S_endScope(tenv);
        return expTy(Tr_seqExp(list), expr.ty);
    }
    case A_arrayExp:
    {
        Ty_ty typ = actual_ty(S_look(tenv, a->u.array.typ));
        if (!typ) {
            EM_error(a->pos, "undefined type");
            return expTy(Tr_noExp(), Ty_Int());
        } else {
            struct expty size = transExp(level, breakk, venv, tenv, a->u.array.size);
            struct expty init = transExp(level, breakk, venv, tenv, a->u.array.init);
            if (size.ty->kind != Ty_int)
                EM_error(a->u.array.size->pos,"array size should be integer");
            if (!ty_match(typ->u.array, init.ty))
                EM_error(a->u.array.init->pos,"type mismatch");
            return expTy(Tr_arrayExp(size.exp, init.exp), typ);
        }
    }
    }
    assert(0);
}

static struct expty transVar(Tr_level level, Tr_exp breakk, S_table venv, S_table tenv, A_var v)
{
    //printf("die in transVar\n");
    switch(v->kind) {
    case A_simpleVar:    /* var id (a)*/
    {
        E_enventry x = S_look(venv, v->u.simple);
        Tr_exp translation = Tr_noExp();
        if (x && x->kind == E_varEntry) {
            translation = Tr_simpleVar(x->u.var.access, level);
            return expTy(translation, actual_ty(x->u.var.ty));
        } else {
            EM_error(v->pos, "undefined variable %s", S_name(v->u.simple));
            return expTy(translation, Ty_Int());
        }
    }
    case A_fieldVar:    /* var record (a.b)*/
    {
        struct expty e = transVar(level, breakk, venv, tenv, v->u.field.var);
        if (e.ty->kind != Ty_record) {
            EM_error(v->u.field.var->pos, "not a record type");
        } else {
            /* Cycle through record field type list looking for field we want */
            Ty_fieldList f;
            int fieldOffset = 0;
            for (f = e.ty->u.record; f; f = f->tail, fieldOffset++) {
                if (f->head->name == v->u.field.sym) {
                    return expTy(Tr_fieldVar(e.exp, fieldOffset), actual_ty(f->head->ty));
                }
            }
            EM_error(v->pos, "field %s doesn't exist", S_name(v->u.field.sym));
        }
        return expTy(Tr_noExp(), Ty_Int());
    }
    case A_subscriptVar:    /*var array (a[b])*/
    {
        struct expty e = transVar(level, breakk, venv, tenv, v->u.subscript.var);
        Tr_exp translation = Tr_noExp();
        if (e.ty->kind != Ty_array) {
            EM_error(v->u.subscript.var->pos, "array type required");
            return expTy(translation, Ty_Int());
        } else {
            struct expty index = transExp(level, breakk, venv, tenv, v->u.subscript.exp);
            if (index.ty->kind != Ty_int) {
                EM_error(v->u.subscript.exp->pos, "integer required");
            } else {
                translation =  Tr_subscriptVar(e.exp, index.exp);
            }
            return expTy(translation, actual_ty(e.ty->u.array));
        }
    }
    }
    assert(0);
}

static Tr_exp transDec(Tr_level level, Tr_exp breakk, S_table venv, S_table tenv, A_dec d)
{
    switch (d->kind) {
    case A_functionDec:
    {
      //printf("reach functionDec\n\n\n");
        A_fundecList funList;
        Ty_tyList formalTys;
        U_boolList formals;
        Ty_ty resultTy;
        /* "headers" first -- so we can deal with mutally recursive functions */
        for (funList = d->u.function; funList; funList = funList->tail) {

            resultTy = NULL;
            if (funList->head->result) {
                resultTy = S_look(tenv, funList->head->result);
                if (!resultTy) {
                    EM_error(funList->head->pos, "undefined type");
                }
            }
            if (!resultTy) resultTy = Ty_Void();

            formalTys = makeFormalTys(tenv, funList->head->params);
            formals = makeFormals(funList->head->params);
            //Temp_label funLabel = Temp_newlabel();
            Temp_label funLabel = Temp_namedlabel(S_name(funList->head->name));
            Tr_level funLevel = Tr_newLevel(level, funLabel, formals);
            S_enter(venv, funList->head->name,
                    E_FunEntry(funLevel, funLabel, formalTys, resultTy));
        }
        E_enventry funEntry = NULL;
        for (funList = d->u.function; funList; funList = funList->tail) {

            funEntry = S_look(venv, funList->head->name);
            S_beginScope(venv);
            Ty_tyList paramTys = funEntry->u.fun.formals;
            A_fieldList paramFields;
            Tr_accessList accessList = Tr_formals(funEntry->u.fun.level);
            for (paramFields = funList->head->params; paramFields;
                 paramFields = paramFields->tail, paramTys = paramTys->tail,
                 accessList = accessList->tail) {
                S_enter(venv, paramFields->head->name,
                        E_VarEntry(accessList->head, paramTys->head));
            }
            struct expty e = transExp(funEntry->u.fun.level, breakk, venv, tenv, funList->head->body);
            if (!ty_match(funEntry->u.fun.result, e.ty))
                EM_error(funList->head->body->pos, "incorrect return type");
            Tr_procEntryExit(funEntry->u.fun.level, e.exp, accessList);
            S_endScope(venv);
        }
        return Tr_noExp();
    }
    case A_varDec:
    {
        struct expty e = transExp(level, breakk, venv, tenv, d->u.var.init);
        // Tr_access access = Tr_allocLocal(level, TRUE);
        Tr_access access = Tr_allocLocal(level, d->u.var.escape);
        if (d->u.var.typ) {
            Ty_ty type = actual_ty(S_look(tenv, d->u.var.typ));
            if (!type) {
                EM_error(d->pos, "undefined type %s", S_name(d->u.var.typ));
                type = Ty_Void();
            }
            if (!ty_match(type, e.ty))
                EM_error(d->pos, "type mismatch");
            S_enter(venv, d->u.var.var, E_VarEntry(access, type));
        } else {
            if (e.ty->kind == Ty_nil)
                EM_error(d->u.var.init->pos, "init should not be nil without type specified");
            S_enter(venv, d->u.var.var, E_VarEntry(access, e.ty));
        }
        return Tr_assignExp(Tr_simpleVar(access, level), e.exp);
    }
    case A_typeDec:
    {

        A_nametyList nameList;
        Ty_ty namety;
        bool isCyclic = TRUE;     /* Illegal cycle in type list */
        /* "headers" first */
        for (nameList = d->u.type; nameList; nameList = nameList->tail) {

            S_enter(tenv, nameList->head->name, Ty_Name(nameList->head->name, NULL));
        }
        /* now we can process the (possibly mutually) recursive bodies */
        for (nameList = d->u.type; nameList; nameList = nameList->tail) {
            Ty_ty t = transTy(tenv, nameList->head->ty);
            if (isCyclic) {
                if (t->kind != Ty_name) isCyclic = FALSE;
            }
            /* Update type binding */
            Ty_ty nameTy = S_look(tenv, nameList->head->name);
            nameTy->u.name.ty = t;
        }
        if (isCyclic)
            EM_error(d->pos, "illegal type cycle");
        return Tr_noExp();
    }
    }
    assert(0);
}

static Ty_ty transTy(S_table tenv, A_ty t)
{
    //printf("die in transTy\n");
    Ty_ty ty;
    switch (t->kind) {
    case A_nameTy:
    {
        ty = S_look(tenv, t->u.name);
        if (!ty) {
            EM_error(t->pos, "undefined type %s", S_name(t->u.name));
        }
        return ty;
    }
    case A_recordTy:
    {
        Ty_fieldList fieldTys = makeFieldTys(tenv, t->u.record);
        return Ty_Record(fieldTys);
    }
    case A_arrayTy:
    {
        ty = S_look(tenv, t->u.name);
        if (!ty) {
            EM_error(t->pos, "undefined type %s",
                     S_name(t->u.name));
        }
        return Ty_Array(ty);
    }
    }
    assert(0);
}

static int ty_match(Ty_ty tType, Ty_ty eType)
{
    Ty_ty actualtType = actual_ty(tType);
    int tyKind = actualtType->kind;
    int eKind = eType->kind;
    return ( ((tyKind == Ty_record || tyKind == Ty_array) && actualtType == eType) ||
             (tyKind == Ty_record && eKind == Ty_nil) ||
             (tyKind != Ty_record && tyKind != Ty_array && tyKind == eKind) );
}

static Ty_ty actual_ty(Ty_ty ty)
{
    if (ty->kind == Ty_name)
        return actual_ty(ty->u.name.ty);
    else
        return ty;
}


static Ty_fieldList makeFieldTys(S_table tenv, A_fieldList fields)
{
    Ty_fieldList fieldTys = NULL;
    Ty_fieldList tailList = fieldTys;
    A_fieldList fieldList;
    for (fieldList = fields; fieldList; fieldList = fieldList->tail) {
        Ty_ty t = S_look(tenv, fieldList->head->typ);
        if (!t) {
            EM_error(fieldList->head->pos, "undefined type %s",
                     S_name(fieldList->head->typ));
        } else {
            Ty_field f = Ty_Field(fieldList->head->name, t);
            if (fieldTys) {
                tailList->tail = Ty_FieldList(f, NULL);
                tailList = tailList->tail;
            } else {
                fieldTys = Ty_FieldList(f, NULL);
                tailList = fieldTys;
            }
        }
    }
    return fieldTys;
}

static Ty_tyList makeFormalTys(S_table tenv, A_fieldList params)
{
    Ty_tyList paramTys = NULL;
    Ty_tyList tailList = paramTys;
    A_fieldList paramList;
    for (paramList = params; paramList; paramList = paramList->tail) {
        Ty_ty t = actual_ty(S_look(tenv, paramList->head->typ));
        if (!t) {
            EM_error(paramList->head->pos, "undefined type %s",
                     S_name(paramList->head->typ));
        } else {
            if (paramTys) {
                tailList->tail = Ty_TyList(t, NULL);
                tailList = tailList->tail;
            } else {
                paramTys = Ty_TyList(t, NULL);
                tailList = paramTys;
            }
        }
    }
    return paramTys;
}

static U_boolList makeFormals(A_fieldList params)
{
    U_boolList formalsList = NULL, tailList = NULL;
    A_fieldList paramList;
    for (paramList = params; paramList; paramList = paramList->tail) {
        if (formalsList) {
            tailList->tail = U_BoolList(TRUE, NULL);
            tailList = tailList->tail;
        } else {
            formalsList = U_BoolList(TRUE, NULL);
            tailList = formalsList;
        }
    }
    return formalsList;
}
