// ilist.h

/******************************************************************************
 * Implementace interpretu imperativniho jazyka IFJ12                         *
 * Autori:                                                                    *
 *          Tomas Nestrojil (xnestr03)                                        *
 *                                                                            *
 * List structure for saving instructions of parsed program                   *
 ******************************************************************************
 */

#ifndef ILIST_H
#define ILIST_H




#include "errnum.h"
// Seznam instrukci
typedef enum {
    // Pokud neni dano jinak, adresuji instrukce pomoci offset na stacku
    // kde offset je int *offset

    // Ridici instrukce
    // LABEL je tIListItem **
    // Instrukce skoku
    INSTR_GOTO,             // op1 = LABEL, op2 = op3 = NULL
    // Instrukce podmineneho skoku
    INSTR_IFGOTO,           // op1 = LABEL, op2 = podminka, op3 = NULLa
    // --- Alternativa - op2 = NULL, condition na vrcholu zasobniku
    // Instrukce volani podprogramu
    INSTR_CALL,             // op1 = functionRecord *, op2 = op3 = NULL
    // Instrukce navratu z podprogramu, uvolni zadany pocet parametru,
    INSTR_RET,              // op1 = *int  op2 = op3 = NULL
    // Instrukce konce programu
    INSTR_HALT,             // op1 = op2 = op3 = NULL
    // Instrukce navesti
    INSTR_LABEL,            // op1 =  op2 = op3 = NULL


    // Aritmeticke instrukce
    // Instrukce scitani
    INSTR_ADD,              // op1 = op2 = op3 = offset
    // Instrukce odcitani
    INSTR_SUBTRACT,         // op1 = op2 = op3 = offset
    // Instrukce nasobeni
    INSTR_MULTIPLY,         // op1 = op2 = op3 = offset
    // Instrukce deleni
    INSTR_DIVIDE,           // op1 = op2 = op3 = offset
    // Instrukce mocniny
    INSTR_POWER,            // op1 = op2 = op3 = offset

    // Relacni instrukce - vraci logickou hodnotu
    // porovnani jestli je op2 < op3
    INSTR_LESSER,           // op1 = op2 = op3 = offset
    // porovnani jestli je op2 > op3
    INSTR_GREATER,          // op1 = op2 = op3 = offset
    // porovnani op2 == op3
    INSTR_EQUAL,            // op1 = op2 = op3 = offset
    // porovnani op2 <= op3
    INSTR_LESSER_OR_EQUAL,  // op1 = op2 = op3 = offset
    // porovnani op2 >= op3
    INSTR_GREATER_OR_EQUAL, // op1 = op2 = op3 = offset
    // porovnani op2 != op3
    INSTR_NOT_EQUAL,        // op1 = op2 = op3 = offset

    // Format instrukce op1 = offset, op2 = offset, op3 = offset
    // na offsetu op3 ulozen tRange*,
    // kde  pro [term1:term2]   off1 = offset(term1),   off2 = offset(term2)
    //      pro [term1:]        off1 = offset(term1),   NULL
    //      pro [:term2]        NULL                    off2 = offset(term2)
    //      pro [:]             NULL                    NULL
    INSTR_SUBSTRING,            // instrukce pro vyber podretezce

    // Instrukce pro praci nad zasobnikem
    // Na zasobnik se ukladaji void*
    INSTR_PUSH,             // Instrukce pro ulozeni hodnoty na zasobnik op1 = void *
    INSTR_PUSH_STACK,       // op1 = offset
    INSTR_POP,              // Instrukce pro odebrani hodnoty ze zasobniku


    // Instrukce pro vnitrni funkce
    INSTR_INPUT,            // Zpracovani vstupu
    INSTR_NUMERIC,          // Konverze na cislo
    INSTR_PRINT,            // Vypisuje hodnoty termu na standardni vystup
    INSTR_TYPEOF,           // Vrati ciselny identifikator datoveho typu
    INSTR_LEN,              // Vrati delku retezce
    INSTR_FIND,             // Hleda vyskyt podretezce v retezci
    INSTR_SORT,             // Seradi znaky v danem retezci


    // Instrukce pro vlozeni polozky na zasobnik
    INSTR_MOV,              // target = offset, op1 = void *, op2 = NULL
    // Instrukce pro vytvoreni kopie polozky na zasobniku
    // zaroven si alokuje prostor na zasobniku
    INSTR_MOV_STACK,            // target = offset, op1 = offset, op2 = NULL
    INSTR_REMOVE_STACK          // target = offset, op1 = op2 = NULL
} InstructionType;

// Struktura instrukce
typedef struct t_instruction
{
    InstructionType instruction;    // Typ instrukce
    void *op1;      // 1. operand
    void *op2;      // 2. operand
    void *op3;
} tInstruction;

// Polozka seznamu instrukci
typedef struct t_listItem
{
    tInstruction *instruction;
    int lineNumber;
    struct t_listItem *nextItem;
} tIListItem;

// Seznam instrukci
typedef struct
{
    tIListItem *first;
    tIListItem *last;
    tIListItem *active;
} tIList;

/**
 * Inicializace seznamu instrukci
 * @param  list Ukazatel na seznam
 * @return      ERR_OK pokud je vse v poradku, jinak prislusny chybovy kod
 */
ecode tIListInit(tIList *list);

/**
 * Uvolneni seznamu instrukci a literalu, ktere instrukce obsahuji
 * @param  list Ukazatel na seznam
 * @return      ERR_OK pokud je vse v poradku, jinak prislusny chybovy kod
 */
ecode tIListFree(tIList *list);

/**
 * Vlozeni instrukce na konec seznamu
 * @param  list        Ukazatel na seznam
 * @param  instruction Ukazatel na instrukci
 * @return             ERR_OK pokud je vse v poradku, jinak prislusny chybovy kod
 */
ecode tIListInsertLast(tIList *list, tInstruction *instruction);

/**
 * Zmena aktivni instrukce na nasleduji
 * @param  list Ukazatel na instrukci
 * @return      ERR_OK pokud je vse v poradku, jinak prislusny chybovy kod
 */
ecode tIListNext(tIList *list);

/**
 * Zmena aktivni instrukce na predanou instrukci
 * @param  list            Ukazatel na seznam
 * @param  gotoInstruction Ukazatel na zaznam instrukce, na kterou se skace
 * @return                 ERR_OK pokud je vse v poradku, jinak prislusny chybovy kod
 */
ecode tIListGoto(tIList *list, tIListItem *gotoInstruction);

/**
 * Ziskani zaznamu posledni instrukce v seznamu
 * @param  list Ukazatel na seznam
 * @return      Ukazatel na zaznam instrukce
 */
tIListItem *tIListGetLast(tIList *list);

/**
 * Ziskani aktivni instrukce
 * @param  list Ukazatel na seznam
 * @return      Ukazatel na instrukci
 */
tInstruction *tIListGetActiveInstruction(tIList *list);


// Vygeneruje instrukci podle zadanych parametru
tInstruction *generateInstruction(InstructionType type, void *op1, void *op2, void *op3);




#endif  // ILIST_H
