/*
 * temp.c - functions to create and manipulate temporary variables which are
 *          used in the IR tree representation before it has been determined
 *          which variables are to go into registers.
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "util.h"
#include "symbol.h"
#include "temp.h"
#include "table.h"

struct Temp_temp_ {int num; };

string Temp_labelstring(Temp_label s)
{
    return S_name(s);
}

static int labels = 0;

Temp_label Temp_newlabel(void)
{
    char buf[100];
    sprintf(buf,"L%d",labels++);
    return Temp_namedlabel(String(buf));
}

/* The label will be created only if it is not found. */
Temp_label Temp_namedlabel(string s)
{
    return S_Symbol(s);
}

static int temps = 100;

Temp_temp Temp_newtemp(void)
{
    Temp_temp p = (Temp_temp) checked_malloc(sizeof (*p));
    p->num=temps++;
    {char r[16];
     sprintf(r, "%d", p->num);
     Temp_enter(Temp_name(), p, String(r)); }
    return p;
}



struct Temp_map_ {TAB_table tab; Temp_map under; };


Temp_map Temp_name(void) {
    static Temp_map m = NULL;
    if (!m) m=Temp_empty();
    return m;
}

Temp_map newMap(TAB_table tab, Temp_map under) {
    Temp_map m = checked_malloc(sizeof(*m));
    m->tab=tab;
    m->under=under;
    return m;
}

Temp_map Temp_empty(void) {
    return newMap(TAB_empty(), NULL);
}

Temp_map Temp_layerMap(Temp_map over, Temp_map under) {
    if (over==NULL)
        return under;
    else return newMap(over->tab, Temp_layerMap(over->under, under));
}

void Temp_enter(Temp_map m, Temp_temp t, string s) {
    assert(m && m->tab);
    TAB_enter(m->tab,t,s);
}

string Temp_look(Temp_map m, Temp_temp t) {
    string s;
    assert(m && m->tab);
    s = TAB_look(m->tab, t);
    if (s) return s;
    else if (m->under) return Temp_look(m->under, t);
    else return NULL;
}

Temp_tempList Temp_TempList(Temp_temp h, Temp_tempList t)
{
    Temp_tempList p = (Temp_tempList) checked_malloc(sizeof (*p));
    p->head=h; p->tail=t;
    return p;
}

Temp_tempList TL(Temp_temp h, Temp_tempList t)
{
    return Temp_TempList(h,t);
}
Temp_labelList Temp_LabelList(Temp_label h, Temp_labelList t)
{
    Temp_labelList p = (Temp_labelList) checked_malloc(sizeof (*p));
    p->head=h; p->tail=t;
    return p;
}

static FILE *outfile;
void showit(Temp_temp t, string r) {
    fprintf(outfile, "t%d -> %s\n", t->num, r);
}

void Temp_dumpMap(FILE *out, Temp_map m) {
    outfile=out;
    TAB_dump(m->tab,(void (*)(void *, void*))showit);
    if (m->under) {
        fprintf(out,"---------\n");
        Temp_dumpMap(out,m->under);
    }
}

static void Temp_appendTail(Temp_temp a, Temp_tempList * tl)
{
    Temp_tempList append = Temp_TempList(a, NULL);
    append->tail = *tl;
    *tl = append;
}

Temp_tempList Temp_copyList(Temp_tempList tl)
{
    Temp_tempList r = NULL, last;
    for (; tl; tl = tl->tail) {
        Temp_appendTail(tl->head, &r);
    }
    return r;
}

Temp_tempList Temp_unionSet(Temp_tempList set1_,Temp_tempList set2_){
  Temp_tempList re_head = NULL,re_tail = NULL,set1 = set1_,set2=set2_;
  while(set1!=NULL){
    Temp_tempList temp = Temp_TempList(set1->head,NULL);
    if(re_tail==NULL){
      re_head = re_tail = temp;
    }else{
      re_tail = re_tail->tail = temp;
    }
    set1 = set1->tail;
  }
  while(set2!=NULL){
    Temp_tempList temp = Temp_TempList(set2->head,NULL);
    if(!inTemp_tempList(set2->head, set1_)){
      if(re_tail == NULL){
        re_head = re_tail = temp;
      }else{
        re_tail = re_tail->tail = temp;
      }
    }
    set2 = set2->tail;
  }
  return re_head;
}

Temp_tempList Temp_differenceSet(Temp_tempList set1_,Temp_tempList set2_){
  Temp_tempList re_head = NULL,re_tail = NULL,set1=set1_;
  while(set1!=NULL){
    if(!inTemp_tempList(set1->head, set2_)){
      Temp_tempList temp = Temp_TempList(set1->head,NULL);
      if(re_tail==NULL){
        re_head = re_tail = temp;
      }else{
        re_tail->tail = temp;
        re_tail = re_tail->tail;
      }
    }
    set1 = set1->tail;
  }
  return re_head;
}

bool inTemp_tempList(Temp_temp temp, Temp_tempList list)
{
	while (list != NULL){
		if (temp == list->head){
			return TRUE;
		}
		list = list->tail;
	}
	return FALSE;
}

int lengthOfTempList(Temp_tempList tempList){
	int count = 0;
	while (tempList != NULL){
		tempList = tempList->tail;
		count++;
	}
	return count;
}

bool isEqualTempList(Temp_tempList t1, Temp_tempList t2)
{
    Temp_tempList tmp, t22 = t2, pre;

    while( t1 && t2) {
        tmp = t2;
        pre = NULL;
        for (; tmp; tmp = tmp->tail)
        {

            if (t1->head == tmp->head)
                goto lp;
            pre = tmp;
        }

        return FALSE;

lp:     t1 = t1->tail;
        if (!pre){
          t2 = t2->tail;
          continue;
        }
        pre->tail = tmp->tail;
    }

    if (t1 || t2) return FALSE;
    return TRUE;
}
