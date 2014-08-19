// label_stack.h
/******************************************************************************
 * Implementace interpretu imperativniho jazyka IFJ12                         *
 * Autori:                                                                    *
 *          Tomas Nestrojil (xnestr03)                                        *
 *                                                                            *
 * Stack structure for storing and manipulation with labels                   *
 ******************************************************************************
 */
#ifndef LABEL_STACK_H
#define LABEL_STACK_H

typedef struct labelStackItem
{
	struct labelStackItem* prev;
	void *data;
} tLabelStackItem;

typedef struct
{
	tLabelStackItem *top;
} tLabelStack;

// Konstruktor pro tLabelStack
tLabelStack* tLabelStackInit();
// Vlozi na vrchol zasobniku predana data, pokud je vse v poradku vrati ERR_OK
int tLabelStackPush(tLabelStack *stack, void *data);
// Odstrani polozku na vrcholu zasobniku
void tLabelStackPop(tLabelStack *stack);
// Vrati data ulozena na vrcholu zasobniku
void *tLabelStackTop(tLabelStack *stack);
// Uvolni zasobnik
void tLabelStackFree(tLabelStack *stack);

#endif // LABEL_STACK_H
