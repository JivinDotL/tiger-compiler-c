
/*Lab5: This header file is not complete. Please finish it with more definition.*/

#ifndef FRAME_H
#define FRAME_H

#include "tree.h"
#include "assem.h"

typedef struct F_frame_ *F_frame;
typedef struct F_access_ *F_access;

Temp_map F_tempMap;
void F_initRegs();

/* declaration for fragments */
typedef struct F_frag_ *F_frag;
struct F_frag_ {enum {F_stringFrag, F_procFrag} kind;
			union {
				struct {Temp_label label; string str;} stringg;
				struct {T_stm body; F_frame frame;} proc;
			} u;
};

F_frag F_StringFrag(Temp_label label, string str);
F_frag F_ProcFrag(T_stm body, F_frame frame);
T_stm F_procEntryExit1(F_frame frame, T_stm stm);

typedef struct F_accessList_ *F_accessList;
struct F_accessList_ {F_access head; F_accessList tail;};

F_frame F_newFrame(Temp_label name, U_boolList formals);
Temp_label F_name(F_frame f);
F_accessList F_formals(F_frame f);
F_access F_allocLocal(F_frame f, bool escape);


typedef struct F_fragList_ *F_fragList;
struct F_fragList_ {F_frag head; F_fragList tail;};
F_fragList F_FragList(F_frag head, F_fragList tail);

/* --- IR --- */
Temp_temp F_FP(void);
extern const int F_wordSize;
T_exp F_Exp(F_access acc, T_exp framePtr);
T_exp F_externalCall(string, T_expList);
Temp_temp F_RV();

F_fragList F_getFragList();
//F_frag F_procEntryExit(F_frame frame, T_stm stm);

/* make it work */
Temp_tempList F_registers();
Temp_temp F_SP(void);
Temp_temp F_RA(void);
Temp_temp F_RV(void);
Temp_tempList F_CallerSaves(void);
Temp_tempList F_CalleeSaves(void);

F_frag F_newProcFrag(T_stm body, F_frame frame);
T_stm F_procEntryExit2(AS_instrList body);
AS_proc F_procEntryExit3(F_frame frame, AS_instrList body);
F_frag F_string(Temp_label lab, string lit);

int F_frameOffset(F_access);


#endif
