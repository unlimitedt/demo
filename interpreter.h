// interpreter.h
/******************************************************************************
 * Implementace interpretu imperativniho jazyka IFJ12                         *
 * Autori:                                                                    *
 *          Tomas Nestrojil (xnestr03)                                        *
 *                                                                            *
 * Interpreter of instruction list                                            *
 ******************************************************************************
 */

#ifndef INTERPRETER_H
#define INTERPRETER_H

#include "ilist.h"
#include "errnum.h"

/**
 * Nastaveni aktivni instrukce na prvni. Inicializace runtime stack a
 * interpretace instrukci dokud nenarazi na instrukci INSTR_HALT
 * @param *instrList Ukazatel na seznam instrukci pro interpretaci
 * @return ERR_OK pokud nenastala zadana chyba, jinak chybovy kod
 */
ecode interpreter(tIList *instrLit);

#endif // INTERPRETER_H
