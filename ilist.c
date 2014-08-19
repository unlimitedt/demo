// ilist.c

/******************************************************************************
 * Implementace interpretu imperativniho jazyka IFJ12                         *
 * Autori:                                                                    *
 *          Tomas Nestrojil (xnestr03)                                        *
 ******************************************************************************
 */

#include "ilist.h"
#include "errnum.h"
#include "global.h"
#include "variable.h"
#include <stdio.h>
#include <stdlib.h>

// Inicializace seznamu instrukci
ecode tIListInit(tIList *list)
{
	if (list == NULL)
		return ERR_LIST;

	list->active = list->first = list->last = NULL;
	return ERR_OK;
}

// Uvolneni seznamu instrukci
ecode tIListFree(tIList *list)
{
	tIListItem *tmp;
	if (list == NULL)
		return ERR_LIST;

	while (list->first != NULL)
	{
		tmp = list->first->nextItem;
		// Uvolneni literalu alokovanych na halde
		if (list->first->instruction->instruction == INSTR_PUSH)
		{
			freeVariable((tVariable **)&(list->first->instruction->op1));
		}
		else if (list->first->instruction->instruction == INSTR_MOV)
		{
			freeVariable((tVariable **) &(list->first->instruction->op2));
		}
		free(list->first->instruction);
		free(list->first);
		list->first = tmp;
	}
	return ERR_OK;
}
// Vlozeni posledni instrukce
ecode tIListInsertLast(tIList *list, tInstruction *instruction)
{
	if (list == NULL)
		return ERR_LIST;

	tIListItem *tmp = malloc(sizeof(tIListItem));
	if (tmp == NULL)
		return ERR_MEMORY;

	tmp->lineNumber = _lineNumber;
	tmp->nextItem = NULL;
	tmp->instruction = instruction;

	// Prvni prvek
	if (list->last == NULL)
	{
		list->first = tmp;
	}
	else
	{
		list->last->nextItem = tmp;
	}

	list->last = tmp;
	return ERR_OK;
}

// Nastaveni aktivni instrukce na nasledujici
ecode tIListNext(tIList *list)
{
	if (list == NULL)
		return ERR_LIST;

	if (list->active != NULL)
		list->active = list->active->nextItem;
	return ERR_OK;
}

// Skok na dalsi instrukci
ecode tIListGoto(tIList *list, tIListItem *gotoInstruction)
{
	if (list == NULL)
		return ERR_LIST;

	list->active = gotoInstruction;
	return ERR_OK;
}

// Vrati ukazatel na posledni instrukci
tIListItem *tIListGetLast(tIList *list)
{
	if (list == NULL)
		return NULL;

	return list->last;
}

// Vrati aktivni instrukci
tInstruction *tIListGetActiveInstruction(tIList *list)
{
	if (list == NULL)
		return NULL;
	if ( list->active == NULL )
		return NULL;

	return list->active->instruction;
}

tInstruction *generateInstruction(InstructionType type, void *op1, void *op2, void *op3)
{
	tInstruction *newInstruction = malloc(sizeof(tInstruction));

	if (newInstruction == NULL)
		return NULL;
	// Nastaveni operandu instrukce a typu instrukce
	newInstruction->instruction = type;
	newInstruction->op1 = op1;
	newInstruction->op2 = op2;
	newInstruction->op3 = op3;
	return newInstruction;
}
