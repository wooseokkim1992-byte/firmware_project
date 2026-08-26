#ifndef INC_MYBT_H_
#define INC_MYBT_H_

#include "main.h"

void bt_Reset(void);

void bt_AtIsOk(void);
void bt_SetName(void);
void bt_SetPassword(void);
void bt_SetRole(void);

void bt_SetSearchMode(void);
void bt_SearchType(void);
void bt_CanConnect(void);

void bt_GetState(void);

void bt_SearchSlave(void);
void bt_StopSearch(void);
void bt_GetSlaveName(void);

void bt_Pair(void);
void bt_Bind(void);
void bt_SetOnlyTarget(void);
void bt_Link(void);
void bt_test_1(void);
void bt_test_2(void);

#endif
