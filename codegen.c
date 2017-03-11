#include <stdio.h>
#include <stdlib.h>
#include "util.h"
#include "symbol.h"
#include "absyn.h"
#include "temp.h"
#include "errormsg.h"
#include "tree.h"
#include "assem.h"
#include "frame.h"
#include "codegen.h"
#include "table.h"

//Lab 6: your code here
#define BUF_SIZE 20
static Temp_temp     munchExp (T_exp e);
static void          munchStm (T_stm s);
static Temp_tempList munchArgs(int i, T_expList args);
static void emit(AS_instr instr);

static AS_instrList iList = NULL, last = NULL;

static void emit(AS_instr inst)
{
    if (!iList) iList = last = AS_InstrList(inst, NULL);
    else last = last->tail = AS_InstrList(inst, NULL);
}

static void munchStm(T_stm stm)
{

    switch(stm->kind) {
    case T_MOVE:
    {
        T_exp dst = stm->u.MOVE.dst, src = stm->u.MOVE.src;
        if (dst->kind == T_MEM) {
            if (dst->u.MEM->kind == T_BINOP &&
                dst->u.MEM->u.BINOP.op == T_plus &&
                dst->u.MEM->u.BINOP.right->kind == T_CONST) {
                /* MOVE(MEM(BINOP(PLUS, e1, CONST(n))), e2) */
                T_exp e1 = dst->u.MEM->u.BINOP.left;
                int n = dst->u.MEM->u.BINOP.right->u.CONST;
                char buf[BUF_SIZE];
                sprintf(buf,"movl `s0,%d(`s1)\n", n);
                Temp_temp r1 = munchExp(src),
                          r2 = munchExp(e1);
                emit(AS_Move(String(buf), NULL, TL(r1, TL(r2, NULL))));

            } else if (dst->u.MEM->kind == T_BINOP &&
                       dst->u.MEM->u.BINOP.op == T_plus &&
                       dst->u.MEM->u.BINOP.left->kind == T_CONST) {
                /* MOVE(MEM(BINOP(PLUS, CONST(n), e1)), e2) */
                T_exp e1 = dst->u.MEM->u.BINOP.right;
                int n = dst->u.MEM->u.BINOP.left->u.CONST;
                char buf[BUF_SIZE];
                sprintf(buf,"movl `s0,%d(`s1)\n", n);
                Temp_temp r1 = munchExp(src),
                          r2 = munchExp(e1);
                emit(AS_Move(String(buf), NULL, TL(r1, TL(r2, NULL))));
            } else if (dst->u.MEM->kind == T_CONST) {
                /* MOVE(MEM(CONST(n)), e2) */
                int n = dst->u.MEM->u.CONST;
                char buf[BUF_SIZE];
                sprintf(buf,"movl `s0,($%d)\n", n);
                emit(AS_Move(String(buf), NULL, TL(munchExp(src), NULL)));
            } else if (src->kind == T_MEM) {
                /* MOVE(MEM(e1), MEM(e2)) */
                T_exp e1 = dst->u.MEM, e2 = src->u.MEM;
                Temp_temp r1 = munchExp(e2),
                          r2 = munchExp(e1),
                          t = Temp_newtemp();
                emit(AS_Move("movl (`s0),`d0\n",TL(t, NULL), TL(r1, NULL)));
                emit(AS_Move("movl `s0,(`s1)\n",NULL, TL(t, TL(r2, NULL))));
            } else {
                /* MOVE(MEM(e1), e2) */
                T_exp e1 = dst->u.MEM, e2 = src;
                Temp_temp r1 = munchExp(e2),
                          r2 = munchExp(e1);
                emit(AS_Move("movl `s0,(`s1)\n", NULL, TL(r1, TL(r2, NULL))));
            }
        }
        else if (dst->kind == T_TEMP) {
            /* MOVE(TEMP(e1), src) */
            Temp_temp t = munchExp(src);
            char buf[BUF_SIZE];
            if (src->kind == T_NAME)
                sprintf(buf,"movl $`s0,`d0\n");
            else if (src->kind == T_CALL){
               t = F_RV();
               sprintf(buf,"movl `s0,`d0\n");
            }
            else
                sprintf(buf,"movl `s0,`d0\n");
            emit(AS_Move(String(buf), TL(dst->u.TEMP, NULL), TL(t, NULL)));
        }
        else assert(0);     /* destination of move must be temp or memory location */
        break;
    }
    case T_SEQ:
    {
        /* SEQ(stm1, stm2) */
        munchStm(stm->u.SEQ.left);
        munchStm(stm->u.SEQ.right);
        break;
    }
    case T_LABEL:
    {
        char buf[BUF_SIZE];
        sprintf(buf,"%s:\n", Temp_labelstring(stm->u.LABEL));
        //printf("test buf %s\n",buf);
        emit(AS_Label(String(buf),stm->u.LABEL));
        break;
    }
    case T_JUMP:
    {
        Temp_temp r = munchExp(stm->u.JUMP.exp);
        emit(AS_Oper("jmp `d0\n", TL(r, NULL), NULL, AS_Targets(stm->u.JUMP.jumps)));
        break;
    }
    case T_CJUMP:
    {   //printf("CJUMP munch left and right\n");
        //fflush(stdout);
        Temp_temp left = munchExp(stm->u.CJUMP.left),
                  right = munchExp(stm->u.CJUMP.right);
        emit(AS_Oper("cmp `s1,`s0\n", NULL, TL(left, TL(right, NULL)), NULL));
        /* No need to deal with CJUMP false label
         * as canonical module has it follow CJUMP */
        char *instr = NULL;
        switch (stm->u.CJUMP.op) {
        case T_eq:
            instr = "je"; break;
        case T_ne:
            instr = "jne"; break;
        case T_lt:
            instr = "jl"; break;
        case T_gt:
            instr = "jg"; break;
        case T_le:
            instr = "jle"; break;
        case T_ge:
            instr = "jge"; break;
        default: assert(0);
        }
        char buf[BUF_SIZE];
        sprintf(buf,"%s `j0\n", instr);
        //printf("test buf %s\n",buf);
        emit(AS_Oper(String(buf), NULL, NULL, AS_Targets(Temp_LabelList(stm->u.CJUMP.true, NULL))));
        break;
    }
    case T_EXP:
    {
        munchExp(stm->u.EXP);
        break;
    }
    default: assert(0);
    }
}

static Temp_temp munchExp(T_exp expr)
{
    switch(expr->kind) {
    case T_BINOP:
    {
        char *op = NULL;
        switch (expr->u.BINOP.op) {
        case T_plus:
            op = "addl";  break;
        case T_minus:
            op = "subl";  break;
        case T_mul:
            op = "imull"; break;
        case T_div: {
            op = "idivl";
            if (expr->u.BINOP.left->kind == T_CONST) {
                /* BINOP(op, CONST(i), e2) */
                T_exp e2 = expr->u.BINOP.right;
                int n = expr->u.BINOP.left->u.CONST;
                char buf[BUF_SIZE];
                Temp_temp s = munchExp(e2);
                sprintf(buf,"movl $%d,`d0\n",n);
                emit(AS_Move(String(buf),TL(F_EAX(),NULL),NULL));
                sprintf(buf,"movl `s0,`d0\n");
                emit(AS_Move(String(buf),TL(F_EDX(),NULL),TL(F_EAX(),NULL)));
                sprintf(buf,"sar $31,`d0\n");
                emit(AS_Oper(String(buf), TL(F_EDX(), NULL), TL(F_EDX(), NULL), NULL));
                sprintf(buf,"%s `s0\n", op);
                emit(AS_Oper(String(buf), TL(F_EAX(), NULL), TL(s, TL(F_EAX(),NULL)), NULL));
                return F_EAX();
            } else if (expr->u.BINOP.right->kind == T_CONST) {
                /* BINOP(op, e2, CONST(i)) */
                Temp_temp r = Temp_newtemp();
                T_exp e2 = expr->u.BINOP.left;
                int n = expr->u.BINOP.right->u.CONST;
                char buf[BUF_SIZE];
                Temp_temp s = munchExp(e2);

                sprintf(buf,"movl `s0,`d0\n");
                emit(AS_Move(String(buf),TL(F_EAX(),NULL),TL(s,NULL)));

                sprintf(buf,"movl `s0,`d0\n");
                emit(AS_Move(String(buf),TL(F_EDX(),NULL),TL(F_EAX(),NULL)));

                sprintf(buf,"sar $31,`d0\n");
                emit(AS_Oper(String(buf), TL(F_EDX(), NULL), TL(F_EDX(), NULL), NULL));

                sprintf(buf,"movl $%d,`d0\n",n);
                emit(AS_Move(String(buf),TL(r,NULL),NULL));

                sprintf(buf,"%s `s0\n", op);
                emit(AS_Oper(String(buf), TL(F_EAX(), NULL), TL(r, TL(F_EAX(),NULL)), NULL));
                return F_EAX();
            } else {
                /* BINOP(op, e1, e2) */
                T_exp e1 = expr->u.BINOP.left,
                      e2 = expr->u.BINOP.right;
                Temp_temp r1 = munchExp(e1);
                Temp_temp r2 = munchExp(e2);
                char buf[BUF_SIZE];
                sprintf(buf,"movl `s0,`d0\n");
                emit(AS_Move(String(buf),TL(F_EAX(),NULL),TL(r1,NULL)));
                sprintf(buf,"movl `s0,`d0\n");
                emit(AS_Move(String(buf),TL(F_EDX(),NULL),TL(F_EAX(),NULL)));
                sprintf(buf,"sar $31,`d0\n");
                emit(AS_Oper(String(buf), TL(F_EDX(), NULL), TL(F_EDX(), NULL), NULL));
                sprintf(buf,"%s `s0\n", op);
                emit(AS_Oper(String(buf), TL(F_EAX(), NULL), TL(r2, TL(F_EAX(),NULL)), NULL));
                return F_EAX();
            }
        }
        default:
            assert(0 && "Invalid operator");
        }

        if (expr->u.BINOP.left->kind == T_CONST) {
            /* BINOP(op, CONST(i), e2) */
            Temp_temp r = Temp_newtemp();
            T_exp e2 = expr->u.BINOP.right;
            Temp_temp r2 = munchExp(e2);
            int n = expr->u.BINOP.left->u.CONST;
            char buf[BUF_SIZE];
            sprintf(buf,"movl $%d,`d0\n",n);
            emit(AS_Move(String(buf),TL(r,NULL),NULL));
            sprintf(buf,"%s `s0,`d0\n", op);
            emit(AS_Oper(String(buf), TL(r, NULL), TL(r2, TL(r,NULL)), NULL));
            return r;
        } else if (expr->u.BINOP.right->kind == T_CONST) {
            /* BINOP(op, e2, CONST(i)) */
            Temp_temp r = Temp_newtemp();
            T_exp e2 = expr->u.BINOP.left;
            int n = expr->u.BINOP.right->u.CONST;
            char buf[BUF_SIZE];
            sprintf(buf,"movl $%d,`d0\n",n);
            emit(AS_Move(String(buf),TL(r,NULL),NULL));
            Temp_temp t = munchExp(e2);
            Temp_temp d = Temp_newtemp();
            emit(AS_Move("movl `s0,`d0\n",TL(d,NULL),TL(t,NULL)));
            sprintf(buf,"%s `s0,`d0\n", op);
            emit(AS_Oper(String(buf), TL(d, NULL), TL(r, TL(t,NULL)), NULL));
            return d;
        } else {
            /* BINOP(op, e1, e2) */
            T_exp e1 = expr->u.BINOP.left,
                  e2 = expr->u.BINOP.right;

            Temp_temp r2 = munchExp(e2);
            Temp_temp r1 = munchExp(e1);
            char buf[BUF_SIZE];
            sprintf(buf,"%s `s0,`d0\n",op);
            emit(AS_Oper(String(buf), TL(r1, NULL), TL(r2, TL(r1,NULL)), NULL));
            return r1;
        }

        printf("error: munchExp T_BINOP\n");
        return NULL;
    }
    case T_MEM:
    {
        T_exp loc = expr->u.MEM;
        if (loc->kind == T_BINOP && loc->u.BINOP.op == T_plus) {
            if (loc->u.BINOP.left->kind == T_CONST) {
                /* MEM(BINOP(PLUS, CONST(i), e2)) */
                Temp_temp r = Temp_newtemp();
                T_exp right = loc->u.BINOP.right;
                int left = loc->u.BINOP.left->u.CONST;
                char buf[BUF_SIZE];
                sprintf(buf,"movl %d(`s0),`d0\n", left);
                //printf("test buf %s\n",buf);
                emit(AS_Move(String(buf), TL(r, NULL), TL(munchExp(right), NULL)));
                return r;
            } else if (loc->u.BINOP.right->kind == T_CONST) {
                /* MEM(BINOP(PLUS, e2, CONST(i))) */
                Temp_temp r = Temp_newtemp();
                T_exp e2 = loc->u.BINOP.left;
                int n = loc->u.BINOP.right->u.CONST;
                char buf[BUF_SIZE];
                sprintf(buf,"movl %d(`s0),`d0\n", n);
                emit(AS_Move(String(buf), TL(r, NULL), TL(munchExp(e2), NULL)));
                return r;
            }
            else {
                //printf("error: munchExp T_MEM 1\n");
                Temp_temp r = Temp_newtemp();

                char buf[BUF_SIZE];
                sprintf(buf,"mov (`s0),`d0\n");
                emit(AS_Move(String(buf),TL(r,NULL), TL(munchExp(loc), NULL) ));
                return r;
            } //assert(0);
        }
        else if (loc->kind == T_CONST) {
            /* MEM(CONST(i)) */
            Temp_temp r = Temp_newtemp();
            int n = loc->u.CONST;
            char buf[BUF_SIZE];
            sprintf(buf,"movl (%d),`d0\n", n);
            //printf("test buf %s\n",buf);
            emit(AS_Move(String(buf), TL(r, NULL), NULL));
            return r;
        }
        else if (loc->kind == T_MEM) {
            /* MEM(e1) */
            Temp_temp r = Temp_newtemp();
            T_exp e1 = loc->u.MEM;
            emit(AS_Move("movl (`s0),`d0\n", TL(r, NULL), TL(munchExp(e1), NULL)));
            return r;
        }
        else{
            printf("error: munchExp T_MEM 2\n");
            return NULL;
        }
    }
    case T_TEMP:
    {
        /* TEMP(t) */
        return expr->u.TEMP;
    }
    case T_ESEQ:
    {
        /* ESEQ(e1, e2) */
        munchStm(expr->u.ESEQ.stm);
        return munchExp(expr->u.ESEQ.exp);
    }
    case T_NAME:
    {
        /* NAME(n) */
        Temp_temp r = Temp_newtemp();
        Temp_enter(F_tempMap, r, Temp_labelstring(expr->u.NAME)); //diff
        return r;
    }
    case T_CONST:
    {
        /* CONST(i) */
        Temp_temp r = Temp_newtemp();
        char buf[BUF_SIZE];
        sprintf(buf,"movl $%d,`d0\n", expr->u.CONST);
        emit(AS_Move(String(buf),TL(r, NULL), NULL));
        return r;
    }
    case T_CALL:
    {
        /* CALL(fun, args) */
        Temp_temp r = munchExp(expr->u.CALL.fun);
        Temp_tempList list = munchArgs(0, expr->u.CALL.args);

        emit(AS_Oper("call `s0\n", F_caller_saves(), TL(r,list), NULL)); //TL(r,list)

        return r;
    }
    default: assert(0);
    }
}
static Temp_tempList munchArgs(int i, T_expList args /*, F_accessList formals*/)
{
    /* get args register-list */
    if (!args) return NULL;
    Temp_temp rarg = munchExp(args->head);
    //printf("\t\t%d\t\t",i);
    Temp_tempList tlist = munchArgs(i + 1, args->tail);
    char buf[BUF_SIZE];
    if (args->head->kind == T_NAME) {
        //sprintf(buf,"pushl $`s0\n");
        sprintf(buf,"movl $`s0,%d(`s1)\n",i * F_WORD_SIZE);
    }
    else {
        //sprintf(buf,"pushl `s0\n");
        sprintf(buf,"movl `s0,%d(`s1)\n",i * F_WORD_SIZE);
    }
    emit(AS_Move(String(buf), NULL, TL(rarg, TL(F_ESP(),NULL))));
    //emit(AS_Oper(String(buf), NULL, TL(rarg, NULL), NULL));
    return TL(rarg, tlist);
}
static void prologue(F_frame f){
    //assert(as_instrList&&as_instrList->head->kind==I_LABEL);

    char instr_1[BUF_SIZE];
    sprintf(instr_1,"pushl `s0\n");
    char instr_2[BUF_SIZE];
    sprintf(instr_2,"movl `s0,`d0\n");
    char instr_3[BUF_SIZE];
    sprintf(instr_3,"subl $%d,`s0\n",-f->local_count * F_WORD_SIZE);
    emit(AS_Oper(String(instr_1),NULL,Temp_TempList(F_EBP(),NULL),NULL));
    emit(AS_Move(String(instr_2),Temp_TempList(F_EBP(),NULL),Temp_TempList(F_ESP(),NULL)));
    emit(AS_Oper(String(instr_3),NULL,Temp_TempList(F_ESP(),NULL),NULL));
}
static void epilogue(F_frame f){
    char instr_4[BUF_SIZE];
    sprintf(instr_4,"movl `s0,`d0\n");
    char instr_5[BUF_SIZE];
    sprintf(instr_5,"popl `d0\n");
    char instr_6[BUF_SIZE];
    sprintf(instr_6,"ret\n");
    emit(AS_Oper("",NULL,F_callee_saves(),NULL));
    emit(AS_Move(String(instr_4),Temp_TempList(F_ESP(),NULL),Temp_TempList(F_EBP(),NULL)));
    emit(AS_Oper(String(instr_5),Temp_TempList(F_EBP(),NULL),NULL,NULL));
    emit(AS_Oper(String(instr_6),F_registers(),NULL,NULL));

}
AS_instrList F_codegen(F_frame f, T_stmList stmList) {
    AS_instrList asList = NULL;
    T_stmList sList = stmList;
    //CODEGEN_frame = frame; // set for munchArgs
    //prologue(f);
    for (; sList; sList = sList->tail)
        munchStm(sList->head);
    //epilogue(f);
    asList = iList;
    AS_proc a = F_procEntryExit3(f,asList);
    iList = last = NULL;
    return a->body;

}
