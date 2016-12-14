#include "regalloc.h"
#include "absyn.h"
#include "assem.h"
#include "color.h"
#include "frame.h"
#include "graph.h"
#include "liveness.h"
#include "symbol.h"
#include "table.h"
#include "temp.h"
#include "tree.h"
#include "util.h"
#include <stdio.h>

#include "codegen.h"
#include "flowgraph.h"

static G_node temp2node(Temp_temp temp);
static double heuristic(Temp_temp t);
static void init_usesdefs(AS_instrList il);
static TAB_table tempuse_;
static TAB_table tempdef_;
static void insert_after(AS_instrList il, AS_instr i);
static void rewrite_inst(Temp_tempList tl, Temp_temp oldt, Temp_temp newt);

static void
insert_after(AS_instrList il, AS_instr i)
{
  AS_instrList tmp = il->tail;
  il->tail->head = i;
  il->tail->tail = tmp;
}

static void
rewrite_inst(Temp_tempList tl, Temp_temp oldt, Temp_temp newt)
{
  for (; tl; tl = tl->tail) {
    if (tl->head == oldt) {
      tl->head = newt;
    }
  }
}

static int
temp_uses(Temp_temp t)
{
  return (int)TAB_look(tempuse_, t);
}
static int
temp_defs(Temp_temp t)
{
  return (int)TAB_look(tempdef_, t);
}

static double
heuristic(Temp_temp t)
{
  assert(t);
  G_node n = temp2node(t);
  int uses_defs = temp_uses(t) + temp_defs(t); // uses and defs in flowgraph
  return 1.0 / G_degree(n);
}

extern TAB_table tempMap;
static G_node
temp2node(Temp_temp temp)
{
  return (G_node)TAB_look(tempMap, temp);
}

void
init_usesdefs(AS_instrList il)
{
  AS_instr i;
  Temp_tempList uses;
  Temp_tempList defs;
  tempuse_ = TAB_empty();
  tempdef_ = TAB_empty();
  for (; il; il = il->tail) {
    i = il->head;
    switch (i->kind) {
      case I_MOVE: // fall through
        uses = i->u.MOVE.src;
        defs = i->u.MOVE.dst;
      case I_OPER: {
        uses = i->u.OPER.src;
        defs = i->u.OPER.dst;
      }
      case I_LABEL:
        continue;
        break;
    }
    for (; uses; uses = uses->tail) {
      TAB_enter(tempuse_, uses->head, TAB_look(tempuse_, uses->head) + 1);
    }
    for (; defs; defs = defs->tail) {
      TAB_enter(tempdef_, uses->head, TAB_look(tempdef_, uses->head) + 1);
    }
  }
}

static Temp_temp
select_spill(Temp_tempList tl)
{
  Temp_temp mintemp;
  TAB_table heur_table;
  double minval = -1, val;

  for (; tl; tl = tl->tail) {
    val = heuristic(tl->head);
    if (minval < 0 || minval < val) {
      mintemp = tl->head;
      minval = val;
    }
  }
  printf("spill ");
  Temp_print(mintemp);
  printf("value:%f\n", minval);
  return mintemp;
}

enum
{
  AS_USE,
  AS_DEF
};
static void
rewrite(Temp_temp spill, AS_instrList il, F_frame f)
{
  // re-write code
  AS_instr i;
  AS_instrList last = NULL;
  Temp_tempList uses;
  Temp_tempList defs;

  F_access inmem = F_allocLocal(f, TRUE); // ESCAPE=TRUE. it's on stack
  bool DEBUG_use_before_def = TRUE;
  for (; il; il = il->tail) {
    i = il->head;
    switch (i->kind) {
      case I_MOVE:
        uses = i->u.MOVE.src;
        defs = i->u.MOVE.dst;
      case I_OPER: {
        uses = i->u.OPER.src;
        defs = i->u.OPER.dst;
      }
      case I_LABEL:
        continue;
        break;
    }
    for (; uses; uses = uses->tail) {
      assert(!DEBUG_use_before_def && "Use before def!");
      if (spill == uses->head) {
        // load before use.
        char* buffer = checked_malloc(64);
        Temp_temp t0 = Temp_newtemp();
        sprintf(buffer, "#spill\nmovl %d(%%ebp), `d0", F_frameOffset(inmem));
        insert_after(last, AS_Oper(buffer, L(t0, NULL), NULL, NULL));
        // replace spill with t0 in the remaining of uses list.
        rewrite_inst(uses, spill, t0);
        break;
      }
    }
    for (; defs; defs = defs->tail) {
      DEBUG_use_before_def = FALSE;
      if (spill == defs->head) {
        // store after use
        char* buffer = checked_malloc(64);
        Temp_temp t0 = Temp_newtemp();
        // replace spill with t0.
        rewrite_inst(defs, spill, t0);
        // move it to memory
        sprintf(buffer, "#spill\nmovl `s0, %d(%%ebp)", F_frameOffset(inmem));
        insert_after(last, AS_Oper(buffer, NULL, L(t0, NULL), NULL));
        // replace spill with t0 in the remaining of uses list.
        break;
      }
    }
    last = il;
  }
}

struct RA_result
RA_regAlloc(F_frame f, AS_instrList il)
{
  // your code here.
  struct RA_result ret;

  for (;;) {
    G_graph flowgraph = FG_AssemFlowGraph(il, f);
    struct Live_graph livegraph = Live_liveness(flowgraph);

    Temp_tempList regs = F_registers();
    struct COL_result cr = COL_color(livegraph.graph, F_tempMap, regs);

    ret.coloring = cr.coloring;

    if (!cr.spills) break;

    printf("====need spilling====\n");
    Temp_printList(cr.spills);
    assert(0); // debug. stop here

    init_usesdefs(il);
    Temp_temp t = select_spill(cr.spills);
    rewrite(t, il, f);
    printf("=======\n");
  }

  ret.il = il;
  return ret;
}
