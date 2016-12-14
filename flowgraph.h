#ifndef FLOWGRAPH_H
#define FLOWGRAPH_H
#include "assem.h"

#include "frame.h"
#include "graph.h"

/*
 * flowgraph.h - Function prototypes to represent control flow graphs.
 */

Temp_tempList FG_def(G_node n);
Temp_tempList FG_use(G_node n);
bool FG_isMove(G_node n);
G_graph FG_AssemFlowGraph(AS_instrList il, F_frame f);
#endif // FLOWGRAPH_H
