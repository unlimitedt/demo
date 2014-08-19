// label_stack.c

/******************************************************************************
 * Implementace interpretu imperativniho jazyka IFJ12                         *
 * Autori:                                                                    *
 *          Tomas Nestrojil (xnestr03)                                        *
 *                                                                            *
 * Stack structure for storing and manipulation with labels                   *
 ******************************************************************************
 */
#include "label_stack.h"
#include "stdlib.h"
#include "errnum.h"
// Konstruktor pro tLabelStack
tLabelStack* tLabelStackInit()
{
	tLabelStack *stack;
	stack = malloc(sizeof(stack));
	if (stack == NULL) // Nedostatek pameti
		return NULL;

	// Inicializace stacku
	stack->top = NULL;

	return stack;
}
// Vlozi na vrchol zasobniku predana data, pokud je vse v poradku vrati ERR_OK
int tLabelStackPush(tLabelStack *stack, void *data)
{
	if (stack == NULL)
		return ERR_NULL;

	tLabelStackItem *newItem;	// Nova polozka zasobniku

	// Alokace pameti pro novou polozku
	newItem = malloc(sizeof(tLabelStackItem));
	if (newItem == NULL)
		return ERR_MEMORY;

	// Nastaveni nove polozky
	newItem->data = data;
	newItem->prev = stack->top;
	stack->top = newItem;

	return ERR_OK;
}

// Odstrani polozku na vrcholu zasobniku
void tLabelStackPop(tLabelStack *stack)
{
	tLabelStackItem *tmpItem;
	if (stack == NULL)
		return;


	if (stack->top != NULL)
	{	// Zasobnik neni prazdny
		tmpItem = stack->top->prev;
		free(stack->top);
		stack->top = tmpItem;
	}
}
// Vrati data ulozena na vrcholu zasobniku
void *tLabelStackTop(tLabelStack *stack)
{
	if (stack == NULL)
		return NULL;
	// Prazdny zasobnik
	if (stack->top == NULL)
		return NULL;

	return stack->top->data;
}
// Uvolni zasobnik
void tLabelStackFree(tLabelStack *stack)
{
	if (stack == NULL)
		return;
	tLabelStackItem *tmpItem;

	while (stack->top != NULL)
	{
		tmpItem = stack->top->prev;
		free(stack->top);
		stack->top = tmpItem;
	}

	free(stack);
}
