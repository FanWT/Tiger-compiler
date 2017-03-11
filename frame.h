#ifndef FRAME_H
#define FRAME_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "temp.h"
#include "tree.h"
#include "absyn.h"
#include "assem.h"

typedef struct F_frame_ *F_frame;



typedef struct F_access_ *F_access;

struct F_access_ {
    enum { inFrame, inReg } kind;
    union {
        int offset; /* inFrame */
        Temp_temp reg; /* inReg */
    } u;
};

typedef struct F_accessList_ *F_accessList;

struct F_accessList_ {
	F_access head;
	F_accessList tail;
};
struct F_frame_ {
    Temp_label name;
    F_accessList formals;
    int local_count;
};

typedef struct F_frag_ *F_frag;
struct F_frag_ {
	enum {F_stringFrag, F_procFrag} kind;
	union {
		struct {
			Temp_label label;
			string str;
		} stringg;
		struct {
			T_stm body;
			F_frame frame;
		} proc;
	} u;
};

typedef struct F_fragList_ *F_fragList;
struct F_fragList_ {
	F_frag head;
	F_fragList tail;
};

extern const int F_WORD_SIZE;
Temp_map F_tempMap;

F_frame F_newFrame(Temp_label name, U_boolList formals);
Temp_label F_name(F_frame frame);
F_accessList F_formals(F_frame frame);
F_access F_allocLocal(F_frame frame, bool escape);

/*
 * Return true if the argument referenced by access escapes.
 */
//bool F_doesEscape(F_access access);

F_frag F_StringFrag(Temp_label label, string str);
F_frag F_ProcFrag(T_stm body, F_frame frame);
F_fragList F_FragList(F_frag head, F_fragList tail);

Temp_tempList F_caller_saves();
Temp_tempList F_callee_saves();

Temp_temp F_FP(); /* Frame pointer */
Temp_temp F_RV();
Temp_temp F_EAX();
Temp_temp F_EBX();
Temp_temp F_ECX();
Temp_temp F_EDX();
Temp_temp F_ESI();
Temp_temp F_EDI();
Temp_temp F_EBP();
Temp_temp F_ESP();

T_exp F_Exp(F_access access, T_exp framePtr);

T_exp F_externalCall(string str, T_expList args);

Temp_map F_precolored();
Temp_map F_temp2Name();

string F_string(Temp_label lab,string str);

T_stm F_procEntryExit1(F_frame frame, T_exp body);
AS_instrList F_procEntryExit2(AS_instrList body);//not used
AS_proc F_procEntryExit3(F_frame frame, AS_instrList body);

//AS_instrList prologue(F_frame f,AS_instrList as_instrList);
//AS_instrList epilogue(F_frame f,AS_instrList as_instrList);
Temp_tempList F_registers();

#endif
