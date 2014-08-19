// runtime_stack.c
/******************************************************************************
 * Implementace interpretu imperativniho jazyka IFJ12                         *
 * Autori:                                                                    *
 *          Tomas Nestrojil (xnestr03)                                        *
 *                                                                            *
 * Stack structure that's used as runtime stack of interpreted programme      *
 ******************************************************************************
 */
#include "runtime_stack.h"
#include "stdlib.h"
#include "errnum.h"
#include <string.h>
#include "variable.h"

/**
 * Funkce pro realokaci zasobniku. Zvetsi zasobnik o definovany prirustek
 * @param  stack Ukazatel na zasobnik
 * @return       ERR_OK pokud je vse v poradku
 *               ERR_MEMORY pokud se realokace neporadila
 */
ecode _reallocRuntimeStack(tRuntimeStack *stack);

tRuntimeStack* tRuntimeStackInit()
{
    tRuntimeStack *stack = malloc(sizeof(tRuntimeStack));
    if (stack == NULL)
        return NULL;

    stack->array = calloc(RUNTIME_STACK_ALLOC_STEP, sizeof(void*));
    stack->size = RUNTIME_STACK_ALLOC_STEP;
    // for (int i = 0; i < stack->size; i++)
    //  stack->array[i] = NULL;

    stack->bp = malloc(sizeof(int));
    stack->sp = 0;
    *(stack->bp) = 0;
    return stack;
}

ecode tRuntimeStackInsert(tRuntimeStack *stack, int offset, void *data)
{
    if ((offset + *(stack->bp)) < 0)
    {
        // Podteceni zasobniku
        return ERR_STACK_UNDERFLOW;
    }
    else if ((offset + *(stack->bp)) > stack->sp)
    {
        // Preteceni zasobniku
        return ERR_STACK_OVERFLOW;
    }
    else if (stack->sp >= (offset + *(stack->bp)))
    {
        stack->array[*(stack->bp) + offset] = data;
    }

    return ERR_OK;
}

ecode tRuntimeStackMoveSP(tRuntimeStack *stack, int increase)
{
    int error;

    // Dokud nebude zasobniku dostatecne velky, zvetsuj ho
    while (stack->size <= (stack->sp + increase))
    {
        error = _reallocRuntimeStack(stack);
        if (error != ERR_OK)
            return error;
    }

    // Inicializace hodnot zasobniku na NULL
    for (int i = stack->sp + 1; i <= stack->sp + increase; i++)
    {
        stack->array[i] = NULL;
    }

    stack->sp += increase;
    return ERR_OK;
}

ecode tRuntimeStackRead(tRuntimeStack *stack, int offset, void **readData)
{
    // Precteni hodnoty na offsetu
    if ((offset + *(stack->bp)) < 0)
        return ERR_STACK_UNDERFLOW; // mimo zasobnik
    else if (stack->size < (offset + *(stack->bp)))
        return ERR_STACK_OVERFLOW;  // za vrcholem zasobniku
    else
        *readData = stack->array[offset + *(stack->bp)];
    return ERR_OK;
}

ecode tRuntimeStackTop(tRuntimeStack *stack, void **readData)
{
    // Kontrola na prazdny zasobnik
    if (stack->sp < 0)
        return ERR_STACK_UNDERFLOW;

    *readData = stack->array[stack->sp];
    return ERR_OK;
}

ecode tRuntimeStackPop(tRuntimeStack *stack)
{
    int error;

    // uvolneni polozky na zasobniku
    freeVariable((tVariable **) &(stack->array[stack->sp]));
    // Snizeni vrcholu zasobniku
    error = tRuntimeStackMoveSP(stack, -1);
    if (error != ERR_OK)
        return error;

    return ERR_OK;
}

ecode tRuntimeStackPush(tRuntimeStack *stack, void *data)
{
    int error;

    // Inkrementace ukazatele na vrchol
    error = tRuntimeStackMoveSP(stack, 1);
    if (error != ERR_OK)
        return error;

    stack->array[stack->sp] = data;
    return ERR_OK;
}


ecode _reallocRuntimeStack(tRuntimeStack *stack)
{
    void **newStack;
    size_t newSize = stack->size + RUNTIME_STACK_ALLOC_STEP;

    // Realokace pole zasobniku
    newStack = realloc(stack->array, newSize * sizeof(void*));
    if (newStack == NULL)
        return ERR_MEMORY;

    stack->array = newStack;
    stack->size = newSize;

    return ERR_OK;
}

void tRuntimeStackDispose(tRuntimeStack *stack)
{
    // Uvolni vsechny polozky na zasobniku
    while (stack->sp >= 0)
        tRuntimeStackPop(stack);

    free(stack->bp);
    free(stack->array);
    free(stack);
}
