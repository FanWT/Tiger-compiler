#include <stdio.h>
#include "util.h"
#include "symbol.h"
#include "table.h"
#include "temp.h"
#include "tree.h"
#include "assem.h"
#include "frame.h"
#include "graph.h"
#include "liveness.h"
#include "color.h"

static G_nodeList0 precolored = NULL;
static G_nodeList0 simplifyWorklist = NULL;
static G_nodeList0 freezeWorklist = NULL;
static G_nodeList0 spillWorklist = NULL;
static G_nodeList0 spilledNodes  = NULL;
static G_nodeList0 coalescedNodes = NULL;
static G_nodeList0 coloredNodes = NULL;
static G_nodeList0 selectStack = NULL;

static Live_moveList2 coalescedMoves = NULL;
static Live_moveList2 constraintMoves = NULL;
static Live_moveList2 frozenMoves = NULL;
static Live_moveList2 worklistMoves = NULL;
static Live_moveList2 activeMoves = NULL;

static int length;
static int K;
static int* degree = NULL;
static G_nodeList0* adjList = NULL;
static bool* adjSet = NULL;
static TAB_table moveList = NULL;
static TAB_table alias = NULL;
static Temp_map color = NULL;
static TAB_table G_nodeMapG_node0 = NULL;
static Temp_tempList registers = NULL;

void initAdjSet(int n){
    adjSet = checked_malloc(n*n*sizeof(bool));
    int i;
    for(i=0; i<n*n; i++) {
        adjSet[i] = FALSE;
    }
}
void initAdjList(int n){
    adjList = checked_malloc(n*sizeof(G_nodeList0));
    int i;
    for(i = 0; i < n; i++) {
        adjList[i] = NULL;
    }
}
void initDegree(int n){
    degree = checked_malloc(n*sizeof(int));
    int i;
    for(i = 0; i < n; i++) {
        degree[i] = 0;
    }
}
void init(int n, Temp_map inital,Temp_tempList regs){
    precolored = NULL;
    simplifyWorklist = NULL;
    freezeWorklist = NULL;
    spillWorklist = NULL;
    spilledNodes  = NULL;
    coalescedNodes = NULL;
    coloredNodes = NULL;
    selectStack = NULL;
    coalescedMoves = NULL;
    constraintMoves = NULL;
    frozenMoves = NULL;
    worklistMoves = NULL;
    activeMoves = NULL;
    length = n;
    K = lengthOfTempList(regs);
    initAdjSet(n);
    initAdjList(n);
    initDegree(n);
    moveList = TAB_empty();
    alias = TAB_empty();
    color = inital;
    G_nodeMapG_node0 = TAB_empty();
    registers = regs;
}
G_node0 G_Node0(G_node node){
    G_node0 g_node0 = checked_malloc(sizeof(*g_node0));
    g_node0->node = node;
    g_node0->kind = DEFAULT1;
    TAB_enter(G_nodeMapG_node0, node, g_node0);
    return g_node0;
}
G_nodeList0 G_NodeList0(G_node0 node,G_nodeList0 pre,G_nodeList0 next){
    G_nodeList0 g_nodeList0 = checked_malloc(sizeof(*g_nodeList0));
    g_nodeList0->value = node;
    g_nodeList0->pre = pre;
    g_nodeList0->next = next;
    return g_nodeList0;
}
Live_moveList2node Live_MoveList2node(Live_moveList move){
    Live_moveList2node live_moveList2node = checked_malloc(sizeof(*live_moveList2node));
    live_moveList2node->move = move;
    live_moveList2node->kind = DEFAULT2;
    return live_moveList2node;
}
Live_moveList2 Live_MoveList2(Live_moveList2node value, Live_moveList2 pre, Live_moveList2 next){
    Live_moveList2 live_moveList2 = checked_malloc(sizeof(*live_moveList2));
    live_moveList2->value = value;
    live_moveList2->pre = pre;
    live_moveList2->next = next;
    return live_moveList2;
}
static bool isEmpty1(G_nodeList0 set){
    return set==NULL;
}
static bool isEmpty2(Live_moveList2 set){
    return set==NULL;
}

static bool isContain1(G_nodeList0*,G_node0);
static bool isContain2(Live_moveList2*,Live_moveList2node);

static G_nodeList0 unionSet1(G_nodeList0 set1_,G_nodeList0 set2_){
    G_nodeList0 head = NULL,tail = NULL,set1=set1_,set2=set2_;
    while(set1!=NULL) {
        G_nodeList0 node = G_NodeList0(set1->value,tail,NULL);
        if(tail==NULL) {
            head = tail = node;
        }else{
            tail = tail->next = node;
        }
        set1 = set1->next;
    }
    while(set2!=NULL) {
        G_nodeList0 node = G_NodeList0(set2->value,tail,NULL);
        //a bug at here
        if(!isContain1(&set1_,set2->value)) {
            if(tail==NULL) {
                head = tail = node;
            }else{
                tail = tail->next = node;
            }
        }
        set2 = set2->next;
    }
    return head;
}
static Live_moveList2 unionSet2(Live_moveList2 set1_, Live_moveList2 set2_){
    Live_moveList2 head = NULL, tail = NULL,set1 = set1_,set2=set2_;
    while (set1 != NULL) {
        Live_moveList2 node = Live_MoveList2(set1->value, tail, NULL);
        if (tail == NULL) {
            head = tail = node;
        }
        else{
            tail = tail->next = node;
        }
        set1 = set1->next;
    }
    while (set2 != NULL) {
        Live_moveList2 node = Live_MoveList2(set2->value, tail, NULL);
        if(!isContain2(&set1_,set2->value)) {
            if (tail == NULL) {
                head = tail = node;
            }
            else{
                tail = tail->next = node;
            }
        }
        set2 = set2->next;
    }
    return head;
}
static void append1(G_nodeList0* set,G_node0 node){
    /*
     * node<------->(node2<----->node1)==set
     */
    if (*set == precolored) {
        node->kind = PRECOLORED;
    }
    else if (*set == simplifyWorklist) {
        node->kind = SIMPLIFYWORKLIST;
    }
    else if (*set == freezeWorklist) {
        node->kind = FREEZEWORKLIST;
    }
    else if (*set == spillWorklist) {
        node->kind = SPILLWORKLIST;
    }
    else if (*set == spilledNodes) {
        node->kind = SPILLEDNODES;
    }
    else if (*set == coalescedNodes) {
        node->kind = COALESCEDNODES;
    }
    else if (*set == coloredNodes) {
        node->kind = COLOREDNODES;
    }
    else if (*set == selectStack) {
        node->kind = SELECTSTACK;
    }
    G_nodeList0 g_nodeList0 = G_NodeList0(node, NULL, NULL);
    if (*set == NULL) {
        *set = g_nodeList0;
    }
    else{
        (*set)->pre = g_nodeList0;
        g_nodeList0->next = (*set);
        (*set) = g_nodeList0;
    }
}
static void append2(Live_moveList2* set, Live_moveList2node node){
    /*
     * node<------->(node2<----->node1)==set
     */
    if (*set == coalescedMoves) {
        node->kind = COALESCEDMOVES;
    }
    else if (*set == constraintMoves) {
        node->kind = CONSTRAINTMOVES;
    }
    else if (*set == frozenMoves) {
        node->kind = FROZENMOVES;
    }
    else if (*set == worklistMoves) {
        node->kind = WORKLISTMOVES;
    }
    else if (*set == activeMoves) {
        node->kind = ACTIVEMOVES;
    }
    Live_moveList2 live_moveList2 = Live_MoveList2(node, NULL, NULL);
    if (*set == NULL) {
        *set = live_moveList2;
    }
    else{
        (*set)->pre = live_moveList2;
        live_moveList2->next = (*set);
        (*set) = live_moveList2;
    }
}
static bool isContain1(G_nodeList0* set, G_node0 node){
    assert(set&&node);
    if (set == &precolored) {
        return node->kind == PRECOLORED;
    }
    else if (set == &simplifyWorklist) {
        return node->kind == SIMPLIFYWORKLIST;
    }
    else if (set == &freezeWorklist) {
        return node->kind == FREEZEWORKLIST;
    }
    else if (set == &spillWorklist) {
        return node->kind == SPILLWORKLIST;
    }
    else if (set == &spilledNodes) {
        return node->kind == SPILLEDNODES;
    }
    else if (set == &coalescedNodes) {
        return node->kind == COALESCEDNODES;
    }
    else if (set == &coloredNodes) {
        return node->kind == COLOREDNODES;
    }
    else if (set == &selectStack) {
        return node->kind == SELECTSTACK;
    }
    G_nodeList0 set_ = *set;
    while (set_ != NULL) {
        if (set_->value == node) {
            return TRUE;
        }
        set_ = set_->next;
    }
    return FALSE;
}
static bool isContain2(Live_moveList2* set, Live_moveList2node node){
    assert(set&&node);
    if (set == &coalescedMoves) {
        return node->kind == COALESCEDMOVES;
    }
    else if (set == &constraintMoves) {
        return node->kind == CONSTRAINTMOVES;
    }
    else if (set == &frozenMoves) {
        return node->kind == FROZENMOVES;
    }
    else if (set == &worklistMoves) {
        return node->kind == WORKLISTMOVES;
    }
    else if (set == &activeMoves) {
        return node->kind == ACTIVEMOVES;
    }
    Live_moveList2 set_ = *set;
    while (set_ != NULL) {
        if (set_->value == node) {
            return TRUE;
        }
        set_ = set_->next;
    }
    return FALSE;
}
static G_nodeList0 diffSet1(G_nodeList0 set1_,G_nodeList0 set2_){
    G_nodeList0 head = NULL, tail=NULL,set1=set1_;
    while (set1 != NULL) {
        if (!isContain1(&set2_, set1->value)) {
            if (tail == NULL) {
                head = tail = G_NodeList0(set1->value, NULL, NULL);
            }
            else{
                G_nodeList0 node = G_NodeList0(set1->value, tail, NULL);
                tail = tail->next = node;
            }
        }
        set1 = set1->next;
    }
    return head;
}
static Live_moveList2 interSet2(Live_moveList2 set1, Live_moveList2 set2){
    Live_moveList2 head=NULL, tail=NULL;
    while (set1 != NULL) {
        if (isContain2(&set2, set1->value)) {
            if (tail == NULL) {
                head = tail = Live_MoveList2(set1->value, NULL, NULL);
            }
            else{
                Live_moveList2 node = Live_MoveList2(set1->value, tail, NULL);
                tail = tail->next = node;
            }
        }
        set1 = set1->next;
    }
    return head;
}
static void delete1(G_nodeList0* set_, G_node0 node){
    assert(*set_);
    G_nodeList0 set = *set_;
    while (set != NULL) {
        if (set->value == node) {
            if (set->pre != NULL) {
                set->pre->next = set->next;
            }
            if (set->next != NULL) {
                set->next->pre = set->pre;
            }
            break;
        }
        set = set->next;
    }
    if ((*set_)->value == node) {
        *set_ = (*set_)->next;
    }
}
static void delete2(Live_moveList2* set_, Live_moveList2node node){
    assert(*set_);
    Live_moveList2 set = *set_;
    while (set != NULL) {
        if (set->value == node) {
            if (set->pre != NULL) {
                set->pre->next = set->next;
            }
            if (set->next != NULL) {
                set->next->pre = set->pre;
            }
            break;
        }
        set = set->next;
    }
    if ((*set_)->value == node) {
        *set_ = (*set_)->next;
    }
}
static void delete3(Stringlist * set_, string node){
    assert(*set_);
    Stringlist set = *set_;
    while (set != NULL) {
        if (set->node == node) {
            if (set->pre != NULL) {
                set->pre->next = set->next;
            }
            if (set->next != NULL) {
                set->next->pre = set->pre;
            }
            break;
        }
        set = set->next;
    }
    if ((*set_)->node == node) {
        *set_ = (*set_)->next;
    }
}

static G_node0 pop(G_nodeList0* stack){
    assert(*stack);
    G_node0 node = (*stack)->value;
    delete1(stack, node);
    return node;
}

static void push(G_nodeList0* stack, G_node0 node){
    append1(stack, node);
}
static bool isLink(int m, int n){
    return adjSet[m*length + n];
}
// static void addEdge_(int m,int n){
//     adjSet[m*length + n] = TRUE;
// }
static void addEdge(G_node0 node1, G_node0 node2){
    int m = node1->node->mykey;
    int n = node2->node->mykey;
    if (m != n&&!isLink(m,n)) {
        adjSet[m*length + n] = TRUE;
        adjSet[n*length + m] = TRUE;
        if (node1->kind != PRECOLORED) {
            append1(&adjList[m], node2);
        }
        if (node2->kind != PRECOLORED) {
            append1(&adjList[n], node1);
        }
    }
}
static Live_moveList2 NodeMoves(G_node0 node){
    return interSet2(TAB_look(moveList, node), unionSet2(activeMoves, worklistMoves));
}
static bool moveRelated(G_node0 node){
    return !isEmpty2(NodeMoves(node));
}
static void makeWorklist(G_nodeList0 initial){
    while (initial != NULL) {
        G_node0 g_node0 = initial->value;
        int pos = g_node0->node->mykey;
        if (degree[pos] >= K) {
            append1(&spillWorklist, g_node0);
        }
        else if (moveRelated(g_node0)) {
            append1(&freezeWorklist, g_node0);
        }
        else{
            append1(&simplifyWorklist, g_node0);
        }
        initial = initial->next;
    }
}
static G_nodeList0 adjacent(G_node0 node){
    return diffSet1(adjList[node->node->mykey], unionSet1(selectStack, coalescedNodes));
}
static void enableMoves(G_nodeList0 g_nodeList0){
    while (g_nodeList0 != NULL) {
        Live_moveList2 live_moveList2 = NodeMoves(g_nodeList0->value);
        while (live_moveList2 != NULL) {
            if (live_moveList2->value->kind == ACTIVEMOVES) {
                delete2(&activeMoves, live_moveList2->value);
                append2(&worklistMoves, live_moveList2->value);
            }
            live_moveList2 = live_moveList2->next;
        }
        g_nodeList0 = g_nodeList0->next;
    }
}
static void decrementDegree(G_node0 node){
    int d = degree[node->node->mykey];
    degree[node->node->mykey]--;
    if (d == K) {
        enableMoves(unionSet1(adjacent(node), G_NodeList0(node, NULL, NULL)));
        delete1(&spillWorklist, node);
        if (moveRelated(node)) {
            append1(&freezeWorklist, node);
        }
        else{
            append1(&simplifyWorklist, node);
        }
    }
}
static void simplify(){
    if (simplifyWorklist != NULL) {
        G_node0 node = pop(&simplifyWorklist);
        push(&selectStack, node);
        G_nodeList0 g_nodeList = adjacent(node);
        while (g_nodeList != NULL) {
            decrementDegree(g_nodeList->value);
            g_nodeList = g_nodeList->next;
        }
    }
}
static G_node0 getAlias(G_node0 node){
    assert(node);
    if (isContain1(&coalescedNodes, node)) {
        getAlias(TAB_look(alias, node));
    }
    return node;
}
static void addWorkList(G_node0 node){
    if (!isContain1(&precolored,node) && !moveRelated(node) && degree[node->node->mykey] < K) {
        delete1(&freezeWorklist, node);
        append1(&simplifyWorklist, node);
    }
}
static bool OK(G_node0 t, G_node0 r){
    return degree[t->node->mykey] < K || isContain1(&precolored, t) || isLink(t->node->mykey, r->node->mykey);
}
static bool conservative(G_nodeList0 nodes){
    int k = 0;
    while (nodes != NULL) {
        if (degree[nodes->value->node->mykey] >= K) {
            k++;
        }
        nodes = nodes->next;
    }
    return k < K;
}

static void coalesce(){
}
static void freezeMoves(G_node0 u){
    Live_moveList2 live_moveList2 = NodeMoves(u);
    while (live_moveList2 != NULL) {
        Live_moveList2node m = live_moveList2->value;
        G_node0 x = TAB_look(G_nodeMapG_node0,m->move->dst);
        G_node0 y = TAB_look(G_nodeMapG_node0, m->move->src);
        G_node0 v = getAlias(y);
        if (getAlias(y) == getAlias(u)) {
            v = getAlias(x);
        }
        delete2(&activeMoves, m);
        append2(&frozenMoves, m);
        if (isEmpty2(NodeMoves(v)) && degree[v->node->mykey] < K) {
            delete1(&freezeWorklist, v);
            append1(&simplifyWorklist, v);
        }
        live_moveList2 = live_moveList2->next;
    }
}
static void freeze(){

}
static void selectSpill(){
    G_node0 m = pop(&spillWorklist);
    append1(&simplifyWorklist, m);
    freezeMoves(m);
}
static Stringlist StringList(string node,Stringlist pre,Stringlist next){
    Stringlist stringlist = checked_malloc(sizeof(*stringlist));
    stringlist->node = node;
    stringlist->pre = pre;
    stringlist->next = next;
    return stringlist;
}
static Stringlist allColors(){
    Stringlist head = NULL,tail = NULL;
    Temp_tempList regs = registers;
    while(regs!=NULL) {
        string node = Temp_look(color,regs->head);
        Stringlist temp  = StringList(node,tail,NULL);
        if(tail == NULL) {
            head = tail = temp;
        }else{
            tail = tail->next = temp;
        }
        regs = regs->tail;
    }
    return head;
}
static bool isEmpty3(Stringlist stringlist){
    return stringlist == NULL;
}
static void assignColors(){
    while (!isEmpty1(selectStack)) {
        G_node0 n = pop(&selectStack);
        Stringlist okColors = allColors();
        G_nodeList0 g_nodeList0 = adjList[n->node->mykey];
        while (g_nodeList0) {
            G_node0 w = g_nodeList0->value;
            getAlias(w);
            G_nodeList0 tempNodeList = unionSet1(coloredNodes,precolored);
            if(isEmpty3(okColors)) {
                break;
            }
            if (isContain1(&tempNodeList,getAlias(w))) {
                string strColor = Temp_look(color, getAlias(w)->node->info);
                delete3(&okColors, strColor);
            }
            g_nodeList0 = g_nodeList0->next;
        }
        if (isEmpty3(okColors)) {
            append1(&spilledNodes, n);
        }
        else{
            append1(&coloredNodes, n);
            Temp_enter(color, n->node->info, okColors->node);
        }
    }
    G_nodeList0 g_nodeList0 = coalescedNodes;
    while (g_nodeList0 != NULL) {
        Temp_enter(color, g_nodeList0->value->node->info, Temp_look(color, getAlias(g_nodeList0->value)->node->info));
        g_nodeList0 = g_nodeList0->next;
    }
}
COL_result COL_color(Live_graph ig, Temp_map inital, Temp_tempList regs){
    G_graph graph = ig->graph;
    Live_moveList moves = ig->moves;
    G_nodeList g_nodeList = G_nodes(graph);
    init(graph->nodecount, inital,regs);
    G_nodeList0 g_nodeList0 = NULL;
    while (g_nodeList != NULL) {
        G_node g_node = g_nodeList->head;
        G_node0 g_node0 = G_Node0(g_node);
        if (Temp_look(inital,g_node->info)!=NULL) {
            append1(&precolored,g_node0);
        }else{
            append1(&g_nodeList0, g_node0);
        }
        g_nodeList = g_nodeList->tail;
    }
    g_nodeList = G_nodes(graph);
    //initial adjSet and adjList
    while (g_nodeList != NULL) {
        G_node g_node = g_nodeList->head;
        G_node0 g_node0 = TAB_look(G_nodeMapG_node0, g_node);
        G_nodeList adjNodeList = G_adj(g_node);

        while (adjNodeList != NULL) {
            G_node otherG_node = adjNodeList->head;
            G_node0 otherG_node0 = TAB_look(G_nodeMapG_node0, otherG_node);
            addEdge(g_node0, otherG_node0);
            adjNodeList = adjNodeList->tail;
        }
        g_nodeList = g_nodeList->tail;
    }
    //initial moveList and worklistMoves
    while (moves != NULL) {
        // Live_moveList2node live_moveList2node = NULL;
        // assert(moves->dst!=NULL&&moves->src!=NULL);
        //
        // live_moveList2node = Live_MoveList2node(moves);
        // append2(&worklistMoves, live_moveList2node);
        // G_node0 dst = TAB_look(G_nodeMapG_node0, moves->dst);
        // G_node0 src = TAB_look(G_nodeMapG_node0, moves->src);
        // TAB_enter(moveList, dst, unionSet2(TAB_look(moveList, dst), Live_MoveList2(live_moveList2node,NULL,NULL)));
        // TAB_enter(moveList, src, unionSet2(TAB_look(moveList, src), Live_MoveList2(live_moveList2node, NULL, NULL)));
        // moves = moves->tail;
    }
    makeWorklist(g_nodeList0);
    do {
        if (!isEmpty1(simplifyWorklist)) {
            simplify();
        }
        else if (!isEmpty2(worklistMoves)) {
            //coalesce();
        }
        else if (!isEmpty1(freezeWorklist)) {
            //freeze();
        }
        else{
            selectSpill();
        }
    } while (!isEmpty1(simplifyWorklist)||!isEmpty2(worklistMoves)||!isEmpty1(freezeWorklist)||!isEmpty1(spillWorklist));
    assignColors();
    COL_result col_result = checked_malloc(sizeof(*col_result));
    col_result->coloring = Temp_layerMap(color,F_temp2Name());
    Temp_tempList spills = NULL;
    while (spilledNodes != NULL) {
        spills = Temp_TempList(spilledNodes->value->node->info,spills);
        spilledNodes = spilledNodes->next;
    }
    col_result->spills = spills;
    return col_result;
}
