#include <stdio.h>
#include "util.h"
#include "table.h"
#include "symbol.h"
#include "absyn.h"
#include "escape.h"

typedef struct Esc_binding_ *Esc_binding;

struct Esc_binding_{
  int depth;
  bool* escape;
};
Esc_binding Esc_newBinding(int depth,bool* escape){
  Esc_binding esc_binding = checked_malloc(sizeof(*esc_binding));
  esc_binding->depth = depth;
  esc_binding->escape = escape;
  return esc_binding;
}
static void traverseExp(S_table env,int depth,A_exp a);
static void traverseDec(S_table env,int depth,A_dec d);
static void traverseVar(S_table env,int depth,A_var v);

static void traverseExp(S_table env,int depth,A_exp a){
  switch(a->kind){
  case A_opExp:{
    traverseExp(env,depth,a->u.op.left);
    traverseExp(env,depth,a->u.op.right);
    break;
  }
  case A_callExp:{
    A_expList args = a->u.call.args;
    while(args!=NULL){
      traverseExp(env,depth,args->head);
      args = args->tail;
    }
    break;
  }
  case A_letExp:{
    A_decList decs = a->u.let.decs;
    S_beginScope(env);
    while(decs!=NULL){
      traverseDec(env,depth,decs->head);
      decs = decs->tail;
    }
    A_exp body = a->u.let.body;
      traverseExp(env,depth,body);

    S_endScope(env);
    break;
  }
  case A_nilExp:{
    break;
  }
  case A_recordExp:{
    A_efieldList fields = a->u.record.fields;
    while(fields!=NULL){
      traverseExp(env,depth,fields->head->exp);
      fields = fields->tail;
    }
    break;
  }
  case A_seqExp:{
    A_expList seq = a->u.seq;
    while(seq!=NULL){
      traverseExp(env,depth,seq->head);
      seq = seq->tail;
    }
    break;
  }
  case A_intExp:{
    break;
  }
  case A_stringExp:{
    break;
  }
  case A_assignExp:{
    traverseVar(env,depth,a->u.assign.var);
    traverseExp(env,depth,a->u.assign.exp);
    break;

  }
  case A_ifExp:{
    traverseExp(env,depth,a->u.iff.test);
    traverseExp(env,depth,a->u.iff.then);
    if(a->u.iff.elsee){
      traverseExp(env,depth,a->u.iff.elsee);
    }
    break;
  }
  case A_whileExp:{
    traverseExp(env,depth,a->u.whilee.test);
    traverseExp(env,depth,a->u.whilee.body);
    break;
  }
  case A_forExp:{
    S_enter(env,a->u.forr.var,Esc_newBinding(depth,&a->u.forr.escape));
    traverseExp(env,depth,a->u.forr.lo);
    traverseExp(env,depth,a->u.forr.hi);
    traverseExp(env,depth,a->u.forr.body);
    break;
  }
  case A_breakExp:{
    break;
  }
  case A_arrayExp:{
    traverseExp(env,depth,a->u.array.size);
    traverseExp(env,depth,a->u.array.init);
    break;
  }
  case A_varExp:{
    traverseVar(env,depth,a->u.var);
    break;
  }
  }
}
static void traverseDec(S_table env,int depth,A_dec d){
  switch(d->kind){
  case A_varDec:{
    S_symbol var = d->u.var.var;
    A_exp init = d->u.var.init;
    traverseExp(env,depth,init);
    printf("escape %d\n",d->u.var.escape==FALSE);
    S_enter(env,var,Esc_newBinding(depth,&d->u.var.escape));
    break;
  }
  case A_functionDec:{
    A_fundecList a_fundecList = d->u.function;
    while(a_fundecList!=NULL){
      A_fundec a_fundec = a_fundecList->head;
      A_fieldList a_fieldList = a_fundec->params;
      S_beginScope(env);
      while(a_fieldList!=NULL){
        bool* strangeBool = checked_malloc(sizeof(*strangeBool));
        S_enter(env,a_fieldList->head->name,Esc_newBinding(depth+1,strangeBool));
        a_fieldList = a_fieldList->tail;
      }
      traverseExp(env,depth+1,a_fundec->body);
      S_endScope(env);
      a_fundecList = a_fundecList->tail;
    }
    break;
  }
  case A_typeDec:
    break;
  }
}
static void traverseVar(S_table env,int depth,A_var v){
  switch(v->kind){
  case A_simpleVar:{
    Esc_binding esc_binding = S_look(env,v->u.simple);
    if(esc_binding==NULL){

    }else{
      if(esc_binding->depth < depth){
        *esc_binding->escape = TRUE;
      }
    }
    break;
  }
  case A_fieldVar:
    traverseVar(env,depth,v->u.field.var);
    break;
  case A_subscriptVar:
    traverseVar(env,depth,v->u.subscript.var);
    break;
  }
}
void Esc_findEscape(A_exp exp){
  S_table escape_env = TAB_empty();
  traverseExp(escape_env,0,exp);
}
