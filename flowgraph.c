#include <stdio.h>
#include "util.h"
#include "symbol.h"
#include "temp.h"
#include "tree.h"
#include "absyn.h"
#include "assem.h"
#include "frame.h"
#include "graph.h"
#include "flowgraph.h"
#include "errormsg.h"
#include "table.h"


Temp_tempList FG_def(G_node n) {
    //your code here.
    if (!n) return NULL;
    AS_instr inst = G_nodeInfo(n);
    switch (inst->kind) {
    case I_OPER: return inst->u.OPER.dst;
    case I_LABEL: break;
    case I_MOVE: return inst->u.MOVE.dst;
    }
    return NULL;
}

Temp_tempList FG_use(G_node n) {
    //your code here.
    if (!n) return NULL;
    AS_instr inst = G_nodeInfo(n);
    switch (inst->kind) {
    case I_OPER: return inst->u.OPER.src;
    case I_LABEL: break;
    case I_MOVE: return inst->u.MOVE.src;
    }
    return NULL;
}

bool FG_isMove(G_node n) {
    //your code here.
    if (!n) return FALSE;
    AS_instr inst = G_nodeInfo(n);
    switch (inst->kind) {
    case I_OPER:  break;
    case I_LABEL: break;
    case I_MOVE: return TRUE;
    }
    return FALSE;
}

static void FG_addJumpEdges(TAB_table t, G_node n)
{
    AS_instr i = (AS_instr)G_nodeInfo(n);
    if (!i->u.OPER.jumps) return;
    Temp_labelList tl = i->u.OPER.jumps->labels;
    G_node neighbour = NULL;
    for (; tl; tl = tl->tail) {
        neighbour = (G_node)TAB_look(t, tl->head);
        if (neighbour && !G_goesTo(n, neighbour)) G_addEdge(n, neighbour);
    }
}

G_graph FG_AssemFlowGraph(AS_instrList il) {
    //your code here.
    G_graph g = G_Graph();
    G_node current = NULL, prev = NULL;
    G_nodeList nodes = NULL;
    TAB_table tb = TAB_empty();
    AS_instr i;

    for (; il; il = il->tail) {
        i = il->head;
        current = G_Node(g, i);
        if (prev)
            G_addEdge(prev, current);
        prev = current;
        switch (i->kind) {
        case I_LABEL: TAB_enter(tb, i->u.LABEL.label, current); break;
        case I_MOVE: break;
        case I_OPER: nodes = G_NodeList(current, nodes); break;
        default: assert(0 && "invalid instr kind");
        }
    }

    for (; nodes; nodes = nodes->tail) {
        current = nodes->head;
        FG_addJumpEdges(tb, current);
    }
    return g;

}
