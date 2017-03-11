#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "util.h"
#include "symbol.h"
#include "temp.h"
#include "table.h"
#include "tree.h"
#include "frame.h"

Temp_temp F_EAX(){
    static Temp_temp eax;
    if(eax==NULL) {
        eax = Temp_newtemp();
    }
    return eax;
}
Temp_temp F_EBX(){
    static Temp_temp ebx;
    if(ebx==NULL) {
        ebx = Temp_newtemp();
    }
    return ebx;
}
Temp_temp F_ECX(){
    static Temp_temp ecx;
    if(ecx==NULL) {
        ecx = Temp_newtemp();
    }
    return ecx;
}
Temp_temp F_EDX(){
    static Temp_temp edx;
    if(edx==NULL) {
        edx = Temp_newtemp();
    }
    return edx;
}
Temp_temp F_EDI(){
    static Temp_temp edi;
    if(edi==NULL) {
        edi = Temp_newtemp();
    }
    return edi;
}
Temp_temp F_ESI(){
    static Temp_temp esi;
    if(esi==NULL) {
        esi = Temp_newtemp();
    }
    return esi;
}
Temp_temp F_EBP(){
    static Temp_temp ebp;
    if(ebp==NULL) {
        ebp = Temp_newtemp();
    }
    return ebp;
}
Temp_temp F_ESP(){
    static Temp_temp esp;
    if(esp==NULL) {
        esp = Temp_newtemp();
    }
    return esp;
}

Temp_temp F_FP(void)
{
    return F_EBP();
}

Temp_temp F_RV(void)
{
    return F_EAX();
}

const int F_WORD_SIZE = 4; // Stack grows to lower address
static const int F_K = 6; // Number of parameters kept in registers


static F_access InFrame(int offset);
static F_access InReg(Temp_temp reg);
static F_accessList F_AccessList(F_access head, F_accessList tail);
static F_accessList makeFormalAccessList(F_frame f, U_boolList formals);

static F_accessList F_AccessList(F_access head, F_accessList tail)
{
    F_accessList list = checked_malloc(sizeof(*list));
    list->head = head;
    list->tail = tail;
    return list;
}

static F_accessList makeFormalAccessList(F_frame f, U_boolList formals)
{
    U_boolList fmls;
    F_accessList headList = NULL, tailList = NULL;
    int i = 0;
    for (fmls = formals; fmls; fmls = fmls->tail, i++) {
        F_access access = NULL;
        if (i < F_K && !fmls->head) {
            access = InReg(Temp_newtemp());
        } else {
            /* Keep a space for return address space. */
            access = InFrame((2 + i) * F_WORD_SIZE);
            //access = InFrame((1 + i) * F_WORD_SIZE);
        }
        if (headList) {
            tailList->tail = F_AccessList(access, NULL);
            tailList = tailList->tail;
        } else {
            headList = F_AccessList(access, NULL);
            tailList = headList;
        }
    }
    return headList;
}

static F_access InFrame(int offset)
{
    F_access fa = checked_malloc(sizeof(*fa));
    fa->kind = inFrame;
    fa->u.offset = offset;
    return fa;
}

static F_access InReg(Temp_temp reg)
{
    F_access fa = checked_malloc(sizeof(*fa));
    fa->kind = inReg;
    fa->u.reg = reg;
    return fa;
}

/*Temp_map F_tempMap = NULL;*/
// static void F_add_to_map(string str, Temp_temp temp)
// {
//     if (!F_tempMap) {
//         F_tempMap = Temp_name();
//     }
//     Temp_enter(F_tempMap, temp, str);
// }

F_frame F_newFrame(Temp_label name, U_boolList formals)
{
    F_frame f = checked_malloc(sizeof(*f));
    f->name = name;
    f->formals = makeFormalAccessList(f, formals);
    f->local_count = 0;
    return f;
}

Temp_label F_name(F_frame f)
{
    return f->name;
}

F_accessList F_formals(F_frame f)
{
    return f->formals;
}

F_access F_allocLocal(F_frame f, bool escape)
{
    f->local_count++;
    if (escape) return InFrame(F_WORD_SIZE * (-f->local_count));
    return InReg(Temp_newtemp());
}

// bool F_doesEscape(F_access access)
// {
//  return (access != NULL && access->kind == inFrame);
// }

F_frag F_StringFrag(Temp_label label, string str)
{
    F_frag strfrag = checked_malloc(sizeof(*strfrag));
    strfrag->kind = F_stringFrag;
    strfrag->u.stringg.label = label;
    strfrag->u.stringg.str = str;
    return strfrag;
}

F_frag F_ProcFrag(T_stm body, F_frame frame)
{
    F_frag pfrag = checked_malloc(sizeof(*pfrag));
    pfrag->kind = F_procFrag;
    pfrag->u.proc.body = body;
    pfrag->u.proc.frame = frame;
    return pfrag;
}

F_fragList F_FragList(F_frag head, F_fragList tail)
{
    F_fragList fl = checked_malloc(sizeof(*fl));
    fl->head = head;
    fl->tail = tail;
    return fl;
}

Temp_tempList F_caller_saves(void) //return value register %eax
{
    return Temp_TempList(F_RV(),NULL);
}

Temp_tempList F_callee_saves(){ //%esp and %ebp
    static Temp_tempList temp_tempList = NULL;
    if(temp_tempList==NULL) {
        temp_tempList = Temp_TempList(F_ESP(),Temp_TempList(F_EBP(),NULL));
    }
    return temp_tempList;
}

T_exp F_Exp(F_access access, T_exp framePtr)
{
    if (access->kind == inFrame) {
        return T_Mem(T_Binop(T_plus, framePtr, T_Const(access->u.offset)));
    } else {
        return T_Temp(access->u.reg);
    }
}

T_exp F_externalCall(string str, T_expList args)
{
    return T_Call(T_Name(Temp_namedlabel(str)), args);
}



Temp_tempList F_registers(){
    static Temp_tempList registers = NULL;
    if (registers == NULL) {
        registers = Temp_TempList(F_EAX(),
                                  Temp_TempList(F_EBX(),
                                                Temp_TempList(F_ECX(),
                                                              Temp_TempList(F_EDX(),
                                                                            Temp_TempList(F_ESI(),
                                                                                          Temp_TempList(F_EDI(),
                                                                                                        Temp_TempList(F_ESP(),
                                                                                                                      Temp_TempList(F_EBP(), NULL))))))));
    }
    return registers;
}



Temp_map F_temp2Name(){
    static Temp_map temp2map = NULL;
    if(temp2map==NULL) {
        temp2map = Temp_layerMap(Temp_empty(),Temp_name());
        Temp_enter(Temp_name(), F_EAX(), "%eax");
        Temp_enter(Temp_name(), F_EBX(), "%ebx");
        Temp_enter(Temp_name(), F_ECX(), "%ecx");
        Temp_enter(Temp_name(), F_EDX(), "%edx");
        Temp_enter(Temp_name(), F_ESI(), "%esi");
        Temp_enter(Temp_name(), F_EDI(), "%edi");
        Temp_enter(Temp_name(), F_ESP(), "%esp");
        Temp_enter(Temp_name(), F_EBP(), "%ebp");
    }
    return temp2map;
}
Temp_map F_precolored(){
    static Temp_map initial = NULL;
    if(initial==NULL) {
        initial = Temp_empty();
        Temp_enter(initial, F_EAX(), "%eax");
        Temp_enter(initial, F_EBX(), "%ebx");
        Temp_enter(initial, F_ECX(), "%ecx");
        Temp_enter(initial, F_EDX(), "%edx");
        Temp_enter(initial, F_ESI(), "%esi");
        Temp_enter(initial, F_EDI(), "%edi");
        Temp_enter(initial, F_EBP(), "%ebp");
        Temp_enter(initial, F_ESP(), "%esp");
    }
    return initial;
}
static void AS_instrListAppend(AS_instrList as_instrList1,AS_instrList as_instrList2){
  if(as_instrList1==NULL) return;
  AS_instrList p = as_instrList1;
  while(p->tail!=NULL){p=p->tail;}
  p->tail = as_instrList2;
}
AS_instrList prologue(F_frame f,AS_instrList as_instrList){
  //assert(as_instrList&&as_instrList->head->kind==I_LABEL);
  AS_instr as_instr_0 = as_instrList->head;
  as_instrList = as_instrList->tail;
  char instr_1[100];
  sprintf(instr_1,"pushl `s0\n");
  char instr_2[100];
  sprintf(instr_2,"movl `s0,`d0\n");
  char instr_3[100];
  sprintf(instr_3,"pushl `s0\n");
  char instr_4[100];
  sprintf(instr_4,"subl $%d,`s0\n",60);
  AS_instr as_instr_1 = AS_Oper(String(instr_1),NULL,Temp_TempList(F_EBP(),NULL),NULL);
  AS_instr as_instr_2 = AS_Move(String(instr_2),Temp_TempList(F_EBP(),NULL),Temp_TempList(F_ESP(),NULL));
  AS_instr as_instr_3 = AS_Oper(String(instr_3),NULL,Temp_TempList(F_EBX(),NULL),NULL);
  AS_instr as_instr_7 = AS_Oper(String(instr_3),NULL,Temp_TempList(F_ECX(),NULL),NULL);
  AS_instr as_instr_5 = AS_Oper(String(instr_3),NULL,Temp_TempList(F_EDX(),NULL),NULL);
  AS_instr as_instr_6 = AS_Oper(String(instr_3),NULL,Temp_TempList(F_ESI(),NULL),NULL);
  AS_instr as_instr_4 = AS_Oper(String(instr_4),NULL,Temp_TempList(F_ESP(),NULL),NULL);
  //if(f->local_count!=0){
    return AS_InstrList(as_instr_0,
                        AS_InstrList(as_instr_1,
                                     AS_InstrList(as_instr_2,
                                                  AS_InstrList(as_instr_3,
                                                    //AS_InstrList(as_instr_7,
                                                      AS_InstrList(as_instr_5,
                                                        AS_InstrList(as_instr_6,
                                                               AS_InstrList(as_instr_4,as_instrList)))))));
}
AS_instrList epilogue(F_frame f,AS_instrList as_instrList){
  assert(as_instrList&&as_instrList->head->kind==I_LABEL);
  char instr_1[100];
  sprintf(instr_1,"addl $%d,`s0\n",60);
  char instr_2[100];
  sprintf(instr_2,"popl `d0\n");
  char instr_4[100];
  sprintf(instr_4,"movl `s0,`d0\n");
  char instr_5[100];
  sprintf(instr_5,"popl `d0\n");
  char instr_6[100];
  sprintf(instr_6,"ret\n");
  AS_instr as_instr_1 = AS_Oper(String(instr_1),NULL,Temp_TempList(F_ESP(),NULL),NULL);
  AS_instr as_instr_7 = AS_Oper(String(instr_2),Temp_TempList(F_ESI(),NULL),NULL,NULL);
  AS_instr as_instr_2 = AS_Oper(String(instr_2),Temp_TempList(F_EDX(),NULL),NULL,NULL);
  AS_instr as_instr_8 = AS_Oper(String(instr_2),Temp_TempList(F_ECX(),NULL),NULL,NULL);
  AS_instr as_instr_3 = AS_Oper(String(instr_2),Temp_TempList(F_EBX(),NULL),NULL,NULL);
  AS_instr as_instr_4 = AS_Move(String(instr_4),Temp_TempList(F_ESP(),NULL),Temp_TempList(F_EBP(),NULL));
  AS_instr as_instr_5 = AS_Oper(String(instr_5),Temp_TempList(F_EBP(),NULL),NULL,NULL);
  AS_instr as_instr_6 = AS_Oper(String(instr_6),F_registers(),NULL,NULL);
  AS_instrListAppend(as_instrList,
                     AS_InstrList(as_instr_1,
                                AS_InstrList(as_instr_7,
                                  AS_InstrList(as_instr_2,
                                    //AS_InstrList(as_instr_8,
                                    AS_InstrList(as_instr_3,
                                               AS_InstrList(as_instr_4,
                                                            AS_InstrList(as_instr_5,
                                                                         AS_InstrList(as_instr_6,NULL))))))));
  return as_instrList;

}
T_stm F_procEntryExit1(F_frame frame, T_exp body)
{
    T_stm label = T_Label(frame->name);
    T_stm move = T_Move(T_Temp(F_RV()),body);
    return T_Seq(label,move);

    //return stm;
}

static Temp_tempList returnSink = NULL;
AS_instrList F_procEntryExit2(AS_instrList body);//not used

AS_proc F_procEntryExit3(F_frame frame, AS_instrList body){
  string pro = NULL;
  body = prologue(frame,body);
  body = epilogue(frame,body);
  return AS_Proc(pro,body,NULL);
}

string F_string(Temp_label lab,string str){
  return NULL;
}
