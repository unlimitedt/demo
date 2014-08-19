// runtime_stack.h
/******************************************************************************
 * Implementace interpretu imperativniho jazyka IFJ12                         *
 * Autori:                                                                    *
 *          Tomas Nestrojil (xnestr03)                                        *
 *                                                                            *
 * Stack structure that's used as runtime stack of interpreted programme      *
 ******************************************************************************
 */

#ifndef RUNTIME_STACK_H
#define RUNTIME_STACK_H

#include "errnum.h"
#define RUNTIME_STACK_ALLOC_STEP 4000

typedef struct
{
	void **array; // Pole pro ukladani polozek
	int size;	// Velikost zasobniku
	int sp;	// Ukazatel na vrchol zasobniku
	int *bp; // Ukazatel na base pointer
} tRuntimeStack;

/**
 * Funkce pro vytvoreni a inicializaci behoveho zasobniku
 * @return Vytvoreny a inicializovany zasobnik
 */
tRuntimeStack* tRuntimeStackInit();

/**
 * Vlozeni polozky na zadany offset vzhledem k base pointeru na zasobniku
 * @param  stack  Ukazatel na zasobnik
 * @param  offset Offset pro ulozeni polozky
 * @param  data   Data pro ulozeni
 * @return        ERR_OK pokud je vse v poradku
 *                ERR_STACK_UNDERFLOW pokud je offset + base pointer mensi
 *                                    nez velikost zasobniku
 *                ERR_STACK_OVERFLOW pokud je offset + base pointer vetsi
 *                                   nez velikost zasobniku
 */
ecode tRuntimeStackInsert(tRuntimeStack *stack, int offset, void *data);


/**
 * Funkce pro cteni hodnoty ze zadaneho offsetu vzhledem k base pointeru na zasobniku
 * @param  stack    Ukazatel na zasobnik
 * @param  offset   Offset ctene polozky
 * @param  readData Ukazatel na ukazatel na data, pro ulozeni prectenych dat
 * @return            ERR_OK pokud je vse v poradku
 *                    ERR_STACK_UNDERFLOW pokud je offset + base pointer mensi
 *                                        nez velikost zasobniku
 *                    ERR_STACK_OVERFLOW pokud je offset + base pointer vetsi
 *                                       nez velikost zasobniku
 */
ecode tRuntimeStackRead(tRuntimeStack *stack, int offset, void **readData);

/**
 * Funkce pro posunuti vrcholu zasobniku o zadanou hodnotu, pokud
 * neni zasobnik pro pozadovany posun dostatecne velky, dojde k jeho
 * zvetseni
 * @param  stack    Ukazatel na zasobnik
 * @param  increase Hodnota o kterou se zasobnik posune
 * @return          ERR_OK pokud je vse v poradku, jinak prislusny
 *                         chybovy kod
 */
ecode tRuntimeStackMoveSP(tRuntimeStack *stack, int increase);

/**
 * Funkce pro precteni hodnoty na vrcholu zasobniku
 * @param  stack    Ukazatel na zasobnik
 * @param  readData Ukazatel na ukazatel na data, pro ulozeni prectenych dat
 * @return          ERR_OK pokud je vse v poradku
 *                  ERR_STACK_UNDERFLOW pokud je zasobnik prazdny
 */
ecode tRuntimeStackTop(tRuntimeStack *stack, void **readData);

/**
 * Vlozeni polozky na vrchol zasobniku
 * @param  stack Ukazatel na zasobnik
 * @param  data  Polozka pro ulozeni
 * @return       ERR_OK pokud je vse v poradku, jinak prislusny
 *                      chybovy kod
 */
ecode tRuntimeStackPush(tRuntimeStack *stack, void *data);

/**
 * Funkce pro odstraneni hodnoty na vrcholu zasobniku
 * @param  stack Ukazatel na zasobnik
 * @return       ERR_OK pokud je vse v poradku, jinak prislusny
 *                      chybovy kod
 */
ecode tRuntimeStackPop(tRuntimeStack *stack);

/**
 * Uvolneni zasobniku a vsech polozek na nem
 * @param stack Ukazatel na zasobnik
 */
void tRuntimeStackDispose(tRuntimeStack *stack);

#endif // RUNTIME_STACK_H
