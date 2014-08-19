// interpretec.c
/******************************************************************************
 * Implementace interpretu imperativniho jazyka IFJ12                         *
 * Autori:                                                                    *
 *          Tomas Nestrojil (xnestr03)                                        *
 *                                                                            *
 * Interpreter of instruction list                                            *
 ******************************************************************************
 */

#include <math.h>
#include "interpreter.h"
#include "errnum.h"
#include "global.h"
#include "libstring.h"
#include "runtime_stack.h"
#include "variable.h"

ecode interpreterInit(tIList *instrList);
ecode insertNewVariable(int offset);
String *convertVariableToString(tVariable *srcVar);
String *stringPower(String *base, double power);

// Funkce pro jednotlive instrukce
ecode instructionGoto(tIList *instrList, tInstruction *instruction);
ecode instructionIfGoto(tIList *instrList, tInstruction *instruction);
ecode instructionCall(tIList *instrList, tInstruction *instruction);
ecode instructionRet(tIList *instrList, tInstruction *instruction);
ecode instructionAdd(tInstruction *instruction);
ecode instructionSubtract(tInstruction *instruction);
ecode instructionMultiply(tInstruction *instruction);
ecode instructionDivide(tInstruction *instruction);
ecode instructionPower(tInstruction *instruction);
ecode operationRelational(tInstruction *instruction);
ecode instructionSubstring(tInstruction *instruction);
ecode instructionPush(tInstruction *instruction);
ecode instructionPushStack(tInstruction *instruction);
ecode instructionPop(tInstruction *instruction);
ecode instructionInput(tInstruction *instruction);
ecode instructionNumeric(tInstruction *instruction);
ecode instructionPrint(tInstruction *instruction);
ecode instructionTypeOf(tInstruction *instruction);
ecode instructionLen(tInstruction *instruction);
ecode instructionFind(tInstruction *instruction);
ecode instructionSort(tInstruction *instruction);
ecode instructionMov(tInstruction *instruction);
ecode instructionMovStack(tInstruction *instruction);
ecode instructionRemoveStack(tInstruction *instruction);

tRuntimeStack *runtimeStack;

/**
 * Nastaveni aktivni instrukce na prvni. Inicializace runtime stack a
 * interpretace instrukci dokud nenarazi na instrukci INSTR_HALT
 * @param *instrList Ukazatel na seznam instrukci pro interpretaci
 * @return ERR_OK pokud nenastala zadana chyba, jinak chybovy kod
 */
ecode interpreter(tIList *instrList)
{
	ecode error;
	runtimeStack = tRuntimeStackInit();

	tInstruction *currentInstruction;

	// ------------------ Nastavi prvni instrukci na aktivni -------------------------
	error = interpreterInit(instrList);
	if (error != ERR_OK) return error;

	currentInstruction = tIListGetActiveInstruction(instrList);
	if (currentInstruction == NULL) return ERR_LIST;

	// ------------------ Nastavi prvni instrukci na aktivni -------------------------
	while (currentInstruction->instruction != INSTR_HALT)
	{
		switch (currentInstruction->instruction)
		{
			case INSTR_GOTO:
				error = instructionGoto(instrList, currentInstruction);
			break;
			case INSTR_IFGOTO:
				error = instructionIfGoto(instrList, currentInstruction);
			break;
			case INSTR_CALL:
				error = instructionCall(instrList, currentInstruction);
			break;
			case INSTR_RET:
				error = instructionRet(instrList, currentInstruction);
			break;
			case INSTR_HALT:
			case INSTR_LABEL:
				// Tyto instrukce se neinterpretuji, maji funkci navseti
			break;
			case INSTR_ADD:
				error = instructionAdd(currentInstruction);
			break;
			case INSTR_SUBTRACT:
				error = instructionSubtract(currentInstruction);
			break;
			case INSTR_MULTIPLY:
				error = instructionMultiply(currentInstruction);
			break;
			case INSTR_DIVIDE:
				error = instructionDivide(currentInstruction);
			break;
			case INSTR_POWER:
				error = instructionPower(currentInstruction);
			break;
			case INSTR_LESSER:
			case INSTR_GREATER:
			case INSTR_EQUAL:
			case INSTR_LESSER_OR_EQUAL:
			case INSTR_GREATER_OR_EQUAL:
			case INSTR_NOT_EQUAL:
				error = operationRelational(currentInstruction);
			break;
			case INSTR_SUBSTRING:
				error = instructionSubstring(currentInstruction);
			break;
			case INSTR_PUSH:
				error = instructionPush(currentInstruction);
			break;
			case INSTR_PUSH_STACK:
				error = instructionPushStack(currentInstruction);
			break;
			case INSTR_POP:
				error = instructionPop(currentInstruction);
			break;
			case INSTR_INPUT:
				error = instructionInput(currentInstruction);
			break;
			case INSTR_NUMERIC:
				error = instructionNumeric(currentInstruction);
			break;
			case INSTR_PRINT:
				error = instructionPrint(currentInstruction);
			break;
			case INSTR_TYPEOF:
				error = instructionTypeOf(currentInstruction);
			break;
			case INSTR_LEN:
				error = instructionLen(currentInstruction);
			break;
			case INSTR_FIND:
				error = instructionFind(currentInstruction);
			break;
			case INSTR_SORT:
				error = instructionSort(currentInstruction);
			break;
			case INSTR_MOV:
				error = instructionMov(currentInstruction);
			break;
			case INSTR_MOV_STACK:
				error = instructionMovStack(currentInstruction);
			break;
			case INSTR_REMOVE_STACK:
				error = instructionRemoveStack(currentInstruction);
			break;
		}
		if(error != ERR_OK)
		{
			tRuntimeStackDispose(runtimeStack);
			return error;
		}

		if (currentInstruction->instruction != INSTR_RET && instrList->active->instruction->instruction != INSTR_HALT)
		{
			error = tIListNext(instrList);
			if(error != ERR_OK)
				return error;
		}

		currentInstruction = tIListGetActiveInstruction(instrList);
		if (currentInstruction == NULL)
		{
			tRuntimeStackDispose(runtimeStack);
			return ERR_LIST;
		}
	}

	tRuntimeStackDispose(runtimeStack);
	runtimeStack = NULL;

	return ERR_OK;
}

/**
 * Zinicializuje interpret pro interpretaci, nastveni prvni instrukce
 * hlavni funkce a nastaveni zasobniku
 * @param *instrList Ukazatel na seznam instrukci pro interpretaci
 * @return ERR_OK pokud nenastala zadana chyba, jinak chybovy kod
 */
ecode interpreterInit(tIList *instrList)
{
	ecode error;
	tTableItem *functionRecord;
	tIListItem *firstInstruction;
	String *mainFunctionName;
	// ------------------ Ziskani prvni instrukce ------------------------------------
	mainFunctionName = charToString(MAIN_FUNCTION_NAME);
	if (mainFunctionName == NULL)
		return ERR_MEMORY;

	functionRecord = searchItem(functionTable, mainFunctionName);
	if (functionRecord == NULL)
	{
		deallocString(mainFunctionName);
		return ERR_INTERNAL;
	}

	deallocString(mainFunctionName);
	firstInstruction = ((tFunctionData *) functionRecord->data)->firstInstruction;

	// ------------------ Nastavi prvni instrukci na aktivni -------------------------
	error = tIListGoto(instrList, firstInstruction);
	if (error != ERR_OK)
		return error;

	error = tRuntimeStackMoveSP(runtimeStack,((tFunctionData *) functionRecord->data)->varTabHead->itemCount + 1);
	if (error)
		return error;

	return ERR_OK;
}

/**
 * Vytvori novou promennou tVariable typu UNDEFINED a vlozi ji na offset do
 * runtimeStack
 * @param offset Cele cislo urcujici posunuti na zasobniku
 * @return ERR_OK pokud nenastala zadana chyba, jinak chybovy kod
 */
ecode insertNewVariable(int offset)
{
	ecode error;
	tVariable *newVar;

	// -------------- Vytvoreni nove nedefinovane promenne -----------------------
	error = createNewVariable(&newVar, UNDEFINED, NULL);
	if (error != ERR_OK)
		return error;

	// -------------- Vlozeni promenne na zasobnik -------------------------------
	error = tRuntimeStackInsert(runtimeStack, offset, newVar);
	if (error != ERR_OK)
	{
		freeVariable(&newVar);
		return error;
	}
	return ERR_OK;
}

/**
 * Funkce pro prevod promenne na retezec
 * @param *srcVar Ukazatel na prevadenou promennou
 * @return Vraci ukazatel na String obsahujici obsah promenne, pri chybe NULL
 */
String *convertVariableToString(tVariable *srcVar)
{
	String *result;
	if (srcVar->semantic == STRING)
	{
		result = stringCopy(srcVar->value);
	}
	else if (srcVar->semantic == NUMERIC)
	{
		result = doubleToString( *((double *) srcVar->value) );
	}
	else if (srcVar->semantic == LOGICAL)
	{
		if (*((bool *)srcVar->value) == true)
			result = charToString("true");
		else
			result = charToString("false");
	}
	else if (srcVar->semantic == NIL)
		result = charToString("Nil");
	else
		result = NULL;

	return result;
}

/**
 * Funkce provadi mocninu retezce
 * @param *base Umocnovany retezec
 * @param power Mocnina retezce
 * @return Vraci ukazatel na String, pokud nastane chyba tak NULL
 */
String *stringPower(String *base, double power)
{
	String *newString;
	newString = charToString("");
	if (newString == NULL)
		return NULL;

	int i = power;
	while (i--)
	{
		newString = stringConcatenate(newString, base);
		if (newString == NULL)
		{
			deallocString(newString);
			return NULL;
		}
	}

	return newString;
}

/**
 * Změna aktivni instrukce - skok na instrukci LABEL v op1
 * @param *instrList    Ukazatel na seznam instrukci
 * @param *instruction  Ukazatel na provadenou instrukci
 * @return ERR_OK pokud nenastala zadana chyba, jinak chybovy kod
 */
ecode instructionGoto(tIList *instrList, tInstruction *instruction)
{
	ecode error;
	tIListItem **label;

	if (instruction->op1 == NULL || instruction->op2 != NULL || instruction->op3 != NULL)
		return ERR_INSTR_WRONG_OPERANDS;

	label = (tIListItem **) instruction->op1;
	error = tIListGoto(instrList, *label);
	if (error != ERR_OK)
		return error;

	return ERR_OK;
}

/**
 * Zmena aktivni instrukce na instrukci LABEL v op1 na zaklade podminky ulozene
 * na zasobniku na offsetu v op2, skace se pokud je podminka splnena
 * @param *instrList    Ukazatel na seznam instrukci
 * @param *instruction  Ukazatel na provadenou instrukci
 * @return ERR_OK pokud nenastala zadana chyba, jinak chybovy kod
 */
ecode instructionIfGoto(tIList *instrList, tInstruction *instruction)
{
	ecode error;
	tVariable *condition;
	tIListItem **jumpLabel;
	jumpLabel = (tIListItem **) instruction->op1;
	bool doJump = false;

	if (instruction->op1 == NULL || instruction->op2 == NULL || instruction->op3 != NULL)
		return ERR_INSTR_WRONG_OPERANDS;

	// -------------- Nacteni promenne podminky ----------------------------------
	error = tRuntimeStackRead(runtimeStack, *((int *) instruction->op2), (void **) &condition);
	if (error != ERR_OK)
		return error;

	// Kontrola datoveho typu vyhodnoceneho vyrazu
	if (condition->semantic == LOGICAL)
	{
		if ( ! *((bool *) condition->value))
			doJump = true;
	}
	else if (condition->semantic == NUMERIC)
	{
		if ( *( (double*) condition->value ) == 0.0 )
			doJump = true;
	}
	else if (condition->semantic == STRING)
	{
		if ( ((String*) condition->value)->length == 0)
			doJump = true;
	}
	else if (condition->semantic == NIL)
	{
		doJump = true;
	}
	else
	{
		return ERR_INTERNAL;
	}

	if ( doJump )
	{
		error = tIListGoto(instrList, *jumpLabel);
		if (error != ERR_OK)
			return error;
	}

	return ERR_OK;
}

/**
 * Provedeni instrukce call, vlozeni IP a pote BP na runtimeStack (BP == SP),
 * posunuti SP pro lokani promenne a skok na prvni instrukci funkce v op1
 * @param *instrList    Ukazatel na seznam instrukci
 * @param *instruction  Ukazatel na provadenou instrukci
 * @return ERR_OK pokud nenastala zadana chyba, jinak chybovy kod
 */
ecode instructionCall(tIList *instrList, tInstruction *instruction)
{
	ecode error;
	int *basePointer;
	tVariable *pVariable = NULL;
	tFunctionData *functionRecord = NULL;

	if (instruction->op1 == NULL || instruction->op2 != NULL || instruction->op3 != NULL)
		return ERR_INSTR_WRONG_OPERANDS;

	// -------------- Nacteni zaznamu volane funkce ---------------------------
	functionRecord = ((tFunctionData *) instruction->op1);

	// -------------- Vytvoreni promenne instruction pointeru ------------------
	error = createNewVariable(&pVariable, INSTRUCTION_POINTER, instrList->active->nextItem);
	if (error != ERR_OK)
		return error;

	// -------------- Vlozeni instruction pointeru na runtime stack ------------
	error = tRuntimeStackPush(runtimeStack, pVariable);
	if (error != ERR_OK)
		return error;

	basePointer = (int*) malloc(sizeof(int));
	*basePointer = *runtimeStack->bp;

	// -------------- Vytvoreni promenne base ponteru -------------------------
	error = createNewVariable(&pVariable, BASE_POINTER, basePointer);
	if (error != ERR_OK)
		return error;

	// -------------- Vlozeni base pointeru na runtime stack ------------------
	error = tRuntimeStackPush(runtimeStack, pVariable);
	if (error != ERR_OK)
		return error;

	*runtimeStack->bp = runtimeStack->sp;

	// -------------- Vytvoreni mista pro lokalni promenne --------------------
	error = tRuntimeStackMoveSP(runtimeStack, functionRecord->varTabHead->itemCount + 1);
	if (error != ERR_OK)
		return error;

	// -------------- Skok na prvni instrukci funkce -------------------------
	error = tIListGoto(instrList, functionRecord->firstInstruction);
	if (error != ERR_OK)
		return error;

	return ERR_OK;
}

/**
 * Provedeni instrukce navratu z funkce
 * Odstraneni dat vytvorenych funkci na zasobniku jejich pocet je v op1
 * obnoveni hodnoty BP, vycteni a nastaveni IP, nacteni navratove hodnoty ze
 * zasobniku, mazani parametru a opetovne vlozeni navratove hodnoty na zasobnik
 * @param *instrList    Ukazatel na seznam instrukci
 * @param *instruction  Ukazatel na provadenou instrukci
 * @return ERR_OK pokud nenastala zadana chyba, jinak chybovy kod
 */
ecode instructionRet(tIList *instrList, tInstruction *instruction)
{
	ecode error;
	int paramsCount;
	tVariable *pVariable;
	tVariable *retVal;

	if (instruction->op1 == NULL || instruction->op2 != NULL || instruction->op3 != NULL)
		return ERR_INSTR_WRONG_OPERANDS;

	// -------------- Nacteni poctu mazanych parametru ------------------------
	paramsCount = *((int *) instruction->op1) + 1;

	// -------------- Odstranovani dat z vrcholu  -----------------------------
	while (runtimeStack->sp != *(runtimeStack->bp))
	{
		error = tRuntimeStackPop(runtimeStack);
		if (error != ERR_OK)
			return error;
	}

	// -------------- Nacteni base pointeru z vrcholu -------------------------
	error = tRuntimeStackTop(runtimeStack, (void **) &pVariable);
	if (error != ERR_OK)
		return error;

	// Nastaveni puvodni hodnoty base pointeru
	*(runtimeStack->bp) = *((int *)pVariable->value);

	// -------------- Odstraneni base pointeru z vrcholu ----------------------
	error = tRuntimeStackPop(runtimeStack);
	if (error != ERR_OK)
		return error;

	// -------------- Nacteni instruction pointeru z vrcholu ------------------
	error = tRuntimeStackTop(runtimeStack, (void **) &pVariable);
	if (error != ERR_OK)
		return error;

	// -------------- Nastaveni instruction pointeru --------------------------
	tIListGoto(instrList, ((tIListItem *) pVariable->value));

	// -------------- Odstraneni instruction pointeru z vrcholu zasobniku -----
	error = tRuntimeStackPop(runtimeStack);
	if (error != ERR_OK)
		return error;

	// -------------- Nacteni navratove hodnoty z vrcholu zasobniku -----------
	error = tRuntimeStackTop(runtimeStack, (void **) &pVariable);
	if (error != ERR_OK)
		return error;

	// -------------- Zkopirovani navratove hodnoty ---------------------------
	error = copyVariable(pVariable, &retVal);
	if (error)
		return error;

	// -------------- Mazani parametru ----------------------------------------
	while (paramsCount != 0)
	{
		error = tRuntimeStackPop(runtimeStack);
		if (error != ERR_OK)
			return error;
		paramsCount--;
	}

	error = tRuntimeStackPush(runtimeStack, retVal);
	if (error)
		return error;

	return ERR_OK;
}

/**
 * Provedeni instrukce scitani nad operandy ulozenymi v zasobniku na
 * offsetu op2 a op3 vyledek je nastaven na offset op3
 * Jedna se bud o aritmeticke scitani nebo konkatenaci
 * @param *instruction Ukazatel na provadenou instrukci
 * @return ERR_OK pokud nenastala zadana chyba, jinak chybovy kod
 */
ecode instructionAdd(tInstruction *instruction)
{
	ecode error;
	tVariable *result, *op1, *op2;
	String *convertedVar;

	if (instruction->op1 == NULL || instruction->op2 == NULL || instruction->op3 == NULL)
		return ERR_INSTR_WRONG_OPERANDS;

	// -------------- Nacteni promenne vysledku ----------------------------------
	error = tRuntimeStackRead(runtimeStack, *((int *) instruction->op1), (void **) &result);
	if (error != ERR_OK)
		return error;

	// -------------- Nacteni prvniho operandu -----------------------------------
	error = tRuntimeStackRead(runtimeStack, *((int *) instruction->op2), (void **) &op1);
	if (error != ERR_OK)
		return error;

	// -------------- Nacteni druheho operandu -----------------------------------
	error = tRuntimeStackRead(runtimeStack, *((int *) instruction->op3), (void **) &op2);
	if (error != ERR_OK)
		return error;

	if (result == NULL)	// Promenna pro vysledek jeste nebyla definovana
	{
		// -------------- Vytvoreni nove nedefinovane promenne -----------------------
		error = insertNewVariable(*((int *) instruction->op1));
		if (error != ERR_OK)
			return error;
		// -------------- Nacteni promenne vysledku ----------------------------------
		error = tRuntimeStackRead(runtimeStack, *((int *) instruction->op1), (void **) &result);
		if (error != ERR_OK)
			return error;
	}

	if (op1 == NULL || op2 == NULL)
		return ERR_RUNTIME_OTHER;	// Promenna operandu nebyla definovana

	// -------------- Operace scitani nad dvema cisly ----------------------------
	if (op1->semantic == NUMERIC && op2->semantic == NUMERIC)
	{
		// -------------- Nastaveni datoveho typu vysledku -----------------------
		error = changeVariableType(result, NUMERIC);
		if (error != ERR_OK)
			return error;
		// -------------- Vysledek operace -------------------------------------------
		*((double *) result->value) = *((double *) op1->value) + *((double *) op2->value);
	}
	// -------------- Konkatenace retezce a druheho operandu ---------------------
	else if (op1->semantic == STRING)
	{
		// -------------- Nastaveni datoveho typu vysledku -----------------------
		error = changeVariableType(result, STRING);
		if (error != ERR_OK)
			return error;

		// Pokud neni druhy operand retezec, tak ho z nej vytvorime
		if (op2->semantic != STRING)
		{
			// -------------- Konverze druheho operandu na retezec -------------------
			convertedVar = convertVariableToString(op2);
			if (convertedVar == NULL)
				return ERR_MEMORY;

			// -------------- Konkatenace dvou retezcu -------------------------------
			result->value = stringConcatenateNew(op1->value, convertedVar);

			deallocString(convertedVar);
		}
		else
			result->value = stringConcatenateNew(op1->value, op2->value);


		if (result->value == NULL)
			return ERR_MEMORY;
	}
	else // Semanticka chyba - nepodporovane datove typy
	{
		return ERR_RUNTIME_INCOMPATIBLE_TYPES;
	}

	return ERR_OK;
}

/**
 * Provedeni instrukce odcitani nad operandy ulozenymi v zasobniku na
 * offsetu op2 a op3 vyledek je nastaven na offset op3
 * Platna pouze pro cisela
 * @param *instruction Ukazatel na provadenou instrukci
 * @return ERR_OK pokud nenastala zadana chyba, jinak chybovy kod
 */
ecode instructionSubtract(tInstruction *instruction)
{
	ecode error;
	tVariable *result, *op1, *op2;

	if (instruction->op1 == NULL || instruction->op2 == NULL || instruction->op3 == NULL)
		return ERR_INSTR_WRONG_OPERANDS;

	// -------------- Nacteni promenne vysledku ----------------------------------
	error = tRuntimeStackRead(runtimeStack, *((int *) instruction->op1), (void **) &result);
	if (error != ERR_OK)
		return error;

	// -------------- Nacteni prvniho operandu -----------------------------------
	error = tRuntimeStackRead(runtimeStack, *((int *) instruction->op2), (void **) &op1);
	if (error != ERR_OK)
		return error;

	// -------------- Nacteni druheho operandu -----------------------------------
	error = tRuntimeStackRead(runtimeStack, *((int *) instruction->op3), (void **) &op2);
	if (error != ERR_OK)
		return error;

	if (result == NULL)	// Promenna pro vysledek jeste nebyla definovana
	{
		// -------------- Vytvoreni nove nedefinovane promenne -----------------------
		error = insertNewVariable(*((int *) instruction->op1));
		if (error != ERR_OK)
			return error;
		// -------------- Nacteni promenne vysledku ----------------------------------
		error = tRuntimeStackRead(runtimeStack, *((int *) instruction->op1), (void **) &result);
		if (error != ERR_OK)
			return error;
	}

	if (op1 == NULL || op2 == NULL)
		return ERR_RUNTIME_OTHER;	// Promenna operandu nebyla definovana


	// -------------- Operace odcitani nad dvema cisly ---------------------------
	if (op1->semantic == NUMERIC && op2->semantic == NUMERIC)
	{
		// -------------- Nastaveni datoveho typu vysledku -----------------------
		error = changeVariableType(result, NUMERIC);
		if (error != ERR_OK)
			return error;

		// -------------- Rozdil dvou cisel --------------------------------------
		*((double *) result->value ) = *((double *) op1->value) - *((double *) op2->value);
	}
	else
	{
		return ERR_RUNTIME_INCOMPATIBLE_TYPES;
	}

	return ERR_OK;
}

/**
 * Provedeni instrukce nasobeni nad operandy ulozenymi v zasobniku na
 * offsetu op2 a op3 vyledek je nastaven na offset op3
 * Jedna se o aritmeticke nasobeni nad cisly nebo mocninu retezce
 * @param *instruction Ukazatel na provadenou instrukci
 * @return ERR_OK pokud nenastala zadana chyba, jinak chybovy kod
 */
ecode instructionMultiply(tInstruction *instruction)
{
	ecode error;
	tVariable *result, *op1, *op2;

	if (instruction->op1 == NULL || instruction->op2 == NULL || instruction->op3 == NULL)
		return ERR_INSTR_WRONG_OPERANDS;

	// -------------- Nacteni promenne vysledku ----------------------------------
	error = tRuntimeStackRead(runtimeStack, *((int *) instruction->op1), (void **) &result);
	if (error != ERR_OK)
		return error;

	// -------------- Nacteni prvniho operandu -----------------------------------
	error = tRuntimeStackRead(runtimeStack, *((int *) instruction->op2), (void **) &op1);
	if (error != ERR_OK)
		return error;

	// -------------- Nacteni druheho operandu -----------------------------------
	error = tRuntimeStackRead(runtimeStack, *((int *) instruction->op3), (void **) &op2);
	if (error != ERR_OK)
		return error;


	if (result == NULL)	// Promenna pro vysledek jeste nebyla definovana
	{
		// -------------- Vytvoreni nove nedefinovane promenne -----------------------
		error = insertNewVariable(*((int *) instruction->op1));
		if (error != ERR_OK)
			return error;
		// -------------- Nacteni promenne vysledku ----------------------------------
		error = tRuntimeStackRead(runtimeStack, *((int *) instruction->op1), (void **) &result);
		if (error != ERR_OK)
			return error;
	}

	if (op1 == NULL || op2 == NULL)
		return ERR_RUNTIME_OTHER;	// Promenna operandu nebyla definovana


	// -------------- Operace nasobeni nad dvema cisly ---------------------------
	if (op1->semantic == NUMERIC && op2->semantic == NUMERIC)
	{
		// -------------- Nastaveni datoveho typu vysledku -----------------------
		error = changeVariableType(result, NUMERIC);
		if (error != ERR_OK)
			return error;

		// -------------- Soucin dvou cisel --------------------------------------
		*((double *) result->value ) = *((double *) op1->value) * *((double *) op2->value);
	}
	// -------------- Operace nasobeni nad retezcem a cislem ---------------------
	else if (op1->semantic == STRING && op2->semantic == NUMERIC)
	{
		// -------------- Nastaveni datoveho typu vysledku -----------------------
		error = changeVariableType(result, STRING);
		if (error != ERR_OK)
			return error;
		// -------------- Mocnina retezce ----------------------------------------
		result->value = stringPower(op1->value, *((double *) op2->value));
		if (result->value == NULL)
			return ERR_MEMORY;
	}
	else
	{
		return ERR_RUNTIME_INCOMPATIBLE_TYPES;
	}

	return ERR_OK;
}

/**
 * Provedeni instrukce deleni nad operandy ulozenymi v zasobniku na
 * offsetu op2 a op3 vyledek je nastaven na offset op3
 * Plati poze pro cisla, kontrola deleni nulou
 * @param *instruction Ukazatel na provadenou instrukci
 * @return ERR_OK pokud nenastala zadana chyba, jinak chybovy kod
 */
ecode instructionDivide(tInstruction *instruction)
{
	ecode error;
	tVariable *result, *op1, *op2;

	if (instruction->op1 == NULL || instruction->op2 == NULL || instruction->op3 == NULL)
		return ERR_INSTR_WRONG_OPERANDS;

	// -------------- Nacteni promenne vysledku ----------------------------------
	error = tRuntimeStackRead(runtimeStack, *((int *) instruction->op1), (void **) &result);
	if (error != ERR_OK)
		return error;

	// -------------- Nacteni prvniho operandu -----------------------------------
	error = tRuntimeStackRead(runtimeStack, *((int *) instruction->op2), (void **) &op1);
	if (error != ERR_OK)
		return error;

	// -------------- Nacteni druheho operandu -----------------------------------
	error = tRuntimeStackRead(runtimeStack, *((int *) instruction->op3), (void **) &op2);
	if (error != ERR_OK)
		return error;


	if (result == NULL)	// Promenna pro vysledek jeste nebyla definovana
	{
		// -------------- Vytvoreni nove nedefinovane promenne -----------------------
		error = insertNewVariable(*((int *) instruction->op1));
		if (error != ERR_OK)
			return error;
		// -------------- Nacteni promenne vysledku ----------------------------------
		error = tRuntimeStackRead(runtimeStack, *((int *) instruction->op1), (void **) &result);
		if (error != ERR_OK)
			return error;
	}

	if (op1 == NULL || op2 == NULL)
		return ERR_RUNTIME_OTHER;	// Promenna operandu nebyla definovana


	// -------------- Operace deleni nad dvema cisly -----------------------------
	if (op1->semantic == NUMERIC && op2->semantic == NUMERIC)
	{
		// -------------- Deleni nulou -------------------------------------------
		if ( *( (double*) op2->value ) == 0)
			return ERR_RUNTIME_ZERO_DIVISION;

		// -------------- Nastaveni datoveho typu vysledku -----------------------
		error = changeVariableType(result, NUMERIC);
		if (error != ERR_OK)
			return error;

		// -------------- Podil dvou cisel ---------------------------------------
		*((double *) result->value ) = *((double *) op1->value) / *((double *) op2->value);
	}
	else
	{
		return ERR_RUNTIME_INCOMPATIBLE_TYPES;
	}

	return ERR_OK;
}

/**
 * Provedeni instrukce umocneni nad operandy ulozenymi v zasobniku na
 * offsetu op2 a op3 vyledek je nastaven na offset op3
 * Plati pouze pro cisla
 * @param *instruction Ukazatel na provadenou instrukci
 * @return ERR_OK pokud nenastala zadana chyba, jinak chybovy kod
 */
ecode instructionPower(tInstruction *instruction)
{
	ecode error;
	tVariable *result, *op1, *op2;

	if (instruction->op1 == NULL || instruction->op2 == NULL || instruction->op3 == NULL)
		return ERR_INSTR_WRONG_OPERANDS;

	// -------------- Nacteni promenne vysledku ----------------------------------
	error = tRuntimeStackRead(runtimeStack, *((int *) instruction->op1), (void **) &result);
	if (error != ERR_OK)
		return error;

	// -------------- Nacteni prvniho operandu -----------------------------------
	error = tRuntimeStackRead(runtimeStack, *((int *) instruction->op2), (void **) &op1);
	if (error != ERR_OK)
		return error;

	// -------------- Nacteni druheho operandu -----------------------------------
	error = tRuntimeStackRead(runtimeStack, *((int *) instruction->op3), (void **) &op2);
	if (error != ERR_OK)
		return error;


	if (result == NULL)	// Promenna pro vysledek jeste nebyla definovana
	{
		// -------------- Vytvoreni nove nedefinovane promenne -----------------------
		error = insertNewVariable(*((int *) instruction->op1));
		if (error != ERR_OK)
			return error;
		// -------------- Nacteni promenne vysledku ----------------------------------
		error = tRuntimeStackRead(runtimeStack, *((int *) instruction->op1), (void **) &result);
		if (error != ERR_OK)
			return error;
	}

	if (op1 == NULL || op2 == NULL)
		return ERR_RUNTIME_OTHER;	// Promenna operandu nebyla definovana


	// -------------- Operace mocniny dvou cisel ---------------------------------
	if (op1->semantic == NUMERIC && op2->semantic == NUMERIC)
	{
		// -------------- Nastaveni datoveho typu vysledku -----------------------
		error = changeVariableType(result, NUMERIC);
		if (error != ERR_OK)
			return error;

		// -------------- Mocnina dvou cisel -------------------------------------
		*((double *) result->value ) = pow( *( (double *) op1->value ), *( (double *) op2->value ) );
	}
	else
	{
		return ERR_RUNTIME_INCOMPATIBLE_TYPES;
	}

	return ERR_OK;
}

/**
 * Provedeni relacni instrukce nad operandy ulozenymi v zasobniku na offsetu
 * op2 a op3 vyledek je nastaven na offset op3
 *
 * @param *instruction Ukazatel na provadenou instrukci
 * @return ERR_OK pokud nenastala zadana chyba, jinak chybovy kod
 */
ecode operationRelational(tInstruction *instruction)
{
	ecode error;
	tVariable *result, *op1, *op2;

	if (instruction->op1 == NULL || instruction->op2 == NULL || instruction->op3 == NULL)
		return ERR_INSTR_WRONG_OPERANDS;

	// -------------- Nacteni promenne vysledku ----------------------------------
	error = tRuntimeStackRead(runtimeStack, *((int *) instruction->op1), (void **) &result);
	if (error != ERR_OK)
		return error;

	// -------------- Nacteni prvniho operandu -----------------------------------
	error = tRuntimeStackRead(runtimeStack, *((int *) instruction->op2), (void **) &op1);
	if (error != ERR_OK)
		return error;

	// -------------- Nacteni druheho operandu -----------------------------------
	error = tRuntimeStackRead(runtimeStack, *((int *) instruction->op3), (void **) &op2);
	if (error != ERR_OK)
		return error;


	if (result == NULL)	// Promenna pro vysledek jeste nebyla definovana
	{
		// -------------- Vytvoreni nove nedefinovane promenne -----------------------
		error = insertNewVariable(*((int *) instruction->op1));
		if (error != ERR_OK)
			return error;
		// -------------- Nacteni promenne vysledku ----------------------------------
		error = tRuntimeStackRead(runtimeStack, *((int *) instruction->op1), (void **) &result);
		if (error != ERR_OK)
			return error;

		error = changeVariableType(result, LOGICAL);
		if (error)
			return error;
	}

	if (op1 == NULL || op2 == NULL)
		return ERR_RUNTIME_OTHER;	// Promenna operandu nebyla definovana


	// -------------- Relacni operace nad dvema cisly nebo retezci ---------------
	if ((op1->semantic == NUMERIC && op2->semantic == NUMERIC) ||
		(op1->semantic == STRING && op2->semantic == STRING))
	{
		// -------------- Nastaveni datoveho typu vysledku -----------------------
		error = changeVariableType(result, LOGICAL);
		if (error != ERR_OK)
			return error;

		if (op1->semantic == NUMERIC)
		{
			// -------------- Porovnani dvou cisel -----------------------------------
			switch (instruction->instruction)
			{
				case INSTR_LESSER:
					*( (bool *) result->value ) = *( (double *) op1->value ) < *( (double *) op2->value );
				break;
				case INSTR_GREATER:
					*( (bool *) result->value ) = *( (double *) op1->value ) > *( (double *) op2->value );
				break;
				case INSTR_LESSER_OR_EQUAL:
					*( (bool *) result->value ) = *( (double *) op1->value ) <= *( (double *) op2->value );
				break;
				case INSTR_GREATER_OR_EQUAL:
					*( (bool *) result->value ) = *( (double *) op1->value ) >= *( (double *) op2->value );
				break;
				case INSTR_EQUAL:
					*( (bool *) result->value ) = *( (double*) op1->value ) == *( (double*) op2->value );
				break;
				case INSTR_NOT_EQUAL:
					*( (bool *) result->value ) = *( (double*) op1->value ) != *( (double*) op2->value );
				break;
				default:
				break;
			}
		}
		else if(op1->semantic == STRING)
		{
			// -------------- Porovnani dvou retezcu ---------------------------------
			int comparison;
			comparison = stringCompare(op1->value, op2->value);

			switch (instruction->instruction)
			{
				case INSTR_LESSER:
					*((bool *) result->value) = (comparison < 0);
				break;
				case INSTR_GREATER:
					*((bool *) result->value) = (comparison > 0);
				break;
				case INSTR_LESSER_OR_EQUAL:
					*((bool *) result->value) = (comparison <= 0);
				break;
				case INSTR_GREATER_OR_EQUAL:
					*((bool *) result->value) = (comparison >= 0);
				break;
				case INSTR_EQUAL:
					*((bool *) result->value) = (comparison == 0);
				break;
				case INSTR_NOT_EQUAL:
					*((bool *) result->value) = (comparison != 0);
				break;
				default:
				break;
			}
		}
	}
	// -------------- Porovnani dvou logickych hodnot ------------------------
	else if ((op1->semantic == LOGICAL && op2->semantic == LOGICAL))
	{

		switch (instruction->instruction)
		{
			case INSTR_EQUAL:
				*((bool *) result->value) = (*((bool *) op1->value) == *((bool *) op2->value));
			break;
			case INSTR_NOT_EQUAL:
				*((bool *) result->value) = (*((bool *) op1->value) != *((bool *) op2->value));
			break;
			default:
				return ERR_RUNTIME_INCOMPATIBLE_TYPES;
			break;
		}
	}
	// -------------- Porovnani dvou nil promennych --------------------------
	else if (op1->semantic == NIL && op2->semantic == NIL)
	{
		switch (instruction->instruction)
		{
			case INSTR_EQUAL:
				*((bool *) result->value) = true;
			break;
			case INSTR_NOT_EQUAL:
				*((bool *) result->value) = false;
			break;
			default:
				return ERR_RUNTIME_INCOMPATIBLE_TYPES;
			break;
		}
	}
	else if (instruction->instruction == INSTR_EQUAL)
	{
		*((bool *) result->value) = false;
	}
	else if (instruction->instruction == INSTR_NOT_EQUAL)
	{
		*((bool *) result->value) = true;
	}
	else
	{
		return ERR_RUNTIME_INCOMPATIBLE_TYPES;
	}

	return ERR_OK;
}

/**
 * Provedeni instrukce vyberu podretezce
 *
 * @param *instruction Ukazatel na provadenou instrukci
 * @return ERR_OK pokud nenastala zadana chyba, jinak chybovy kod
 */
ecode instructionSubstring(tInstruction *instruction)
{
	ecode error;
	tVariable *result, *varString, *varFrom, *varTo, *varRange;
	tRange *range;
	int from, to;
	// -------------- Nacteni promenne vysledku ----------------------------------
	error = tRuntimeStackRead(runtimeStack, *((int *) instruction->op1), (void **) &result);
	if (error != ERR_OK)
		return error;

	// -------------- Nacteni prvniho operandu -----------------------------------
	error = tRuntimeStackRead(runtimeStack, *((int *) instruction->op2), (void **) &varString);
	if (error != ERR_OK)
		return error;

	// -------------- Nacteni druheho operandu -----------------------------------
	// -------------- Rozsah podretezce ------------------------------------------
	error = tRuntimeStackRead(runtimeStack, *((int *) instruction->op3), (void **) &varRange);
	if (error != ERR_OK)
		return error;


	if (result == NULL)	// Promenna pro vysledek jeste nebyla definovana
	{
		// -------------- Vytvoreni nove nedefinovane promenne -----------------------
		error = insertNewVariable(*((int *) instruction->op1));
		if (error != ERR_OK)
			return error;
		// -------------- Nacteni promenne vysledku ----------------------------------
		error = tRuntimeStackRead(runtimeStack, *((int *) instruction->op1), (void **) &result);
		if (error != ERR_OK)
			return error;
	}

	if (varString == NULL || varRange == NULL)
		return ERR_RUNTIME_OTHER;	// Promenna operandu nebyla definovana

	range = varRange->value;

	// -------------- Nekompatibilni datovy typ ------------------------------
	if (varString->semantic != STRING)
		return ERR_RUNTIME_INCOMPATIBLE_TYPES;


	// -------------- Nastaveni rozsahu podretezce -------------------------------

	// -------------- Dolni mez retezce ------------------------------------------
	if (range->off1 != NULL)
	{
		error = tRuntimeStackRead(runtimeStack, *(range->off1), (void **) &varFrom);
		if (error != ERR_OK)
			return error;

		if (varFrom->semantic != NUMERIC)
			return ERR_RUNTIME_INCOMPATIBLE_TYPES;

		from = *((double *) varFrom->value);
	}
	else
	{
		from = 0;
	}

	// -------------- Horni mez retezce ------------------------------------------
	if (range->off2 != NULL)
	{
		error = tRuntimeStackRead(runtimeStack, *(range->off2), (void **) &varTo);
		if (error != ERR_OK)
			return error;

		if (varTo->semantic != NUMERIC)
			return ERR_RUNTIME_INCOMPATIBLE_TYPES;

		to = *((double *) varTo->value);
	}
	else
	{
		to = stringLength(varString->value);
	}


	// -------------- Nastaveni datoveho typu vysledku -----------------------
	error = changeVariableType(result, STRING);
	if (error != ERR_OK)
		return error;

	result->value = substring(varString->value, from, to);


	return ERR_OK;
}

/**
 * Vlozeni kopie promenne z op1 na vrchol zasobniku
 * @param *instruction Ukazatel na provadenou instrukci
 * @return ERR_OK pokud nenastala zadana chyba, jinak chybovy kod
 */
ecode instructionPush(tInstruction *instruction)
{
	ecode error;
	tVariable *newVar;
	if (instruction->op1 == NULL || instruction->op2 != NULL || instruction->op3 != NULL)
	{
		return ERR_INSTR_WRONG_OPERANDS;
	}

	// -------------- Vytvoreni kopie promenne -----------------------------------
	error = copyVariable(instruction->op1, &newVar);
	if (error != ERR_OK)
		return error;

	// -------------- Vlozeni promenne na zasobnik -------------------------------
	error = tRuntimeStackPush(runtimeStack, newVar);
	if (error != ERR_OK)
	{
		freeVariable(&newVar);
		return error;
	}
	return ERR_OK;
}

/**
 * Vlozeni kopie promenne ulozene na offsetu op1 v zasobniku na vrchol zasobniku
 * @param *instruction Ukazatel na provadenou instrukci
 * @return ERR_OK pokud nenastala zadana chyba, jinak chybovy kod
 */
ecode instructionPushStack(tInstruction *instruction)
{
	ecode error;
	tVariable *varToPush, *varSrc;

	if (instruction->op1 == NULL || instruction->op2 != NULL || instruction->op3 != NULL)
	{
		return ERR_INSTR_WRONG_OPERANDS;
	}

	// -------------- Nacteni promenne pro vlozeni na zasobnik -------------------
	error = tRuntimeStackRead(runtimeStack, *((int *) instruction->op1), (void **) &varSrc);
	if (error != ERR_OK)
		return error;

	// -------------- Vytvoreni kopie promenne -----------------------------------
	error = copyVariable(varSrc, &varToPush);
	if (error != ERR_OK)
		return error;

	// -------------- Vlozeni promenne na zasobnik -------------------------------
	error = tRuntimeStackPush(runtimeStack, varToPush);
	if (error != ERR_OK)
	{
		freeVariable(&varToPush);
		return error;
	}
	return ERR_OK;

}

/**
 * Vycteni promenne z vrcholu zasobniku a ulozeni kopie do promenne vyctene ze
 * ze zasobniku na offsetu op1
 * @param *instruction Ukazatel na provadenou instrukci
 * @return ERR_OK pokud nenastala zadana chyba, jinak chybovy kod
 */
ecode instructionPop(tInstruction *instruction)
{
	ecode error;
	tVariable *varPopped, *varResult, *tmp;

	if (instruction->op1 == NULL || instruction->op2 != NULL || instruction->op3 != NULL)
	{
		return ERR_INSTR_WRONG_OPERANDS;
	}

	// -------------- Nacteni promenne na vrcholu zasobniku ----------------------
	error = tRuntimeStackTop(runtimeStack, (void **) &varPopped);
	if (error)
		return error;

	// -------------- Nacteni promenne pro ulozeni vrcholu zasobniku -------------
	error = tRuntimeStackRead(runtimeStack, *((int *) instruction->op1), (void **) &varResult);
	if (error)
		return error;


	if (varResult == NULL)	// Promenna pro vysledek jeste nebyla definovana
	{
		// -------------- Vytvoreni nove nedefinovane promenne -----------------------
		error = insertNewVariable(*((int *) instruction->op1));
		if (error)
			return error;

		// -------------- Nacteni promenne vysledku ----------------------------------
		error = tRuntimeStackRead(runtimeStack, *((int *) instruction->op1), (void **) &varResult);
		if (error)
			return error;
	}

	// -------------- Nastaveni typu promenne vysledku ------------------------
	error = changeVariableType(varResult, varPopped->semantic);
	if (error)
		return error;

	// -------------- Vytvoreni kopie promenne -----------------------------------
	error = copyVariable(varPopped, &tmp);
	if (error)
		return error;

	if (varResult->value != NULL)
		free(varResult->value);

	varResult->value = tmp->value;
	free(tmp);


	// -------------- Odstraneni vrcholu zasobniku -------------------------------
	error = tRuntimeStackPop(runtimeStack);
	if (error)
		return error;

	return ERR_OK;
}

/**
 * Kopie promenne *tVraible z op2 na zasobnik na offset op1
 * @param *instruction Ukazatel na provadenou instrukci
 * @return ERR_OK pokud nenastala zadana chyba, jinak chybovy kod
 */
ecode instructionMov(tInstruction *instruction)
{
	ecode error;
	tVariable *result;
	if (instruction->op1 == NULL || instruction->op2 == NULL || instruction->op3 != NULL)
	{
		return ERR_INSTR_WRONG_OPERANDS;
	}

	// -------------- Nacteni promenne vysledku ----------------------------------
	error = tRuntimeStackRead(runtimeStack, *((int *) instruction->op1), (void **) &result);
	if (error != ERR_OK)
		return error;

	if (result != NULL)	// Promenna pro vysledek jiz byla definovana
	{
		// Uvolneni promenne
		freeVariable(&result);
	}

	// -------------- Vytvoreni kopie promenne -----------------------------------
	error = copyVariable(instruction->op2, &result);
	if (error != ERR_OK)
		return error;

	// -------------- Vlozeni promenne na zasobnik -------------------------------
	error = tRuntimeStackInsert(runtimeStack, *((int *) instruction->op1), result);
	if (error != ERR_OK)
	{
		freeVariable(&result);
		return error;
	}
	return ERR_OK;
}

/**
 * Kopie promenne *tVraible z offsetu op2 na offset op1
 * @param *instruction Ukazatel na provadenou instrukci
 * @return ERR_OK pokud nenastala zadana chyba, jinak chybovy kod
 */
ecode instructionMovStack(tInstruction *instruction)
{
	ecode error;
	tVariable *result, *srcVar;
	if (instruction->op1 == NULL || instruction->op2 == NULL || instruction->op3 != NULL)
	{
		return ERR_INSTR_WRONG_OPERANDS;
	}

	// -------------- Nacteni promenne vysledku ----------------------------------
	error = tRuntimeStackRead(runtimeStack, *((int *) instruction->op1), (void **) &result);
	if (error != ERR_OK)
		return error;

	// -------------- Nacteni promenne pro kopirovani ----------------------------
	error = tRuntimeStackRead(runtimeStack, *((int *) instruction->op2), (void **) &srcVar);
	if (error != ERR_OK)
		return error;

	if (srcVar == NULL)
	{
		// Promenna nebyla definovana, neni co presouvat
		return ERR_RUNTIME_INCOMPATIBLE_TYPES;
	}

	if (result != NULL)	// Promenna pro vysledek jiz byla definovana
	{
		freeVariable(&result);
		// -------------- Vlozeni NULL na zasobnik -----------------------------------
		error = tRuntimeStackInsert(runtimeStack, *((int *) instruction->op1), NULL);
		if (error != ERR_OK)
			return error;
	}

	// -------------- Vytvoreni kopie promenne -----------------------------------
	error = copyVariable(srcVar, &result);
	if (error != ERR_OK)
		return error;

	// -------------- Vlozeni promenne na zasobnik -------------------------------
	error = tRuntimeStackInsert(runtimeStack, *((int *) instruction->op1), result);
	if (error != ERR_OK)
	{
		freeVariable(&result);
		return error;
	}
	return ERR_OK;
}

/**
 * Odstraneni promenne na offsetu op1 ze zasobniku
 * @param *instruction Ukazatel na provadenou instrukci
 * @return ERR_OK pokud nenastala zadana chyba, jinak chybovy kod
 */
ecode instructionRemoveStack(tInstruction *instruction)
{
	ecode error;
	tVariable *target;

	if (instruction->op1 == NULL || instruction->op2 != NULL || instruction->op3 != NULL)
	{
		return ERR_INSTR_WRONG_OPERANDS;
	}

	// -------------- Nacteni mazane promenne ------------------------------------
	error = tRuntimeStackRead(runtimeStack, *((int *) instruction->op1), (void **) &target);
	if (error != ERR_OK)
		return error;

	freeVariable(&target);

	// -------------- Vlozeni NULL na zasobnik -----------------------------------
	error = tRuntimeStackInsert(runtimeStack, *((int *) instruction->op1), NULL);
	if (error != ERR_OK)
		return error;

	return ERR_OK;
}

/**
 * Provedeni vestavene fce input(), nacteni radky s escape sekvencemi ze stdin
 * @param *instruction Ukazatel na provadenou instrukci
 * @return ERR_OK pokud nenastala zadana chyba, jinak chybovy kod
 */
ecode instructionInput(tInstruction *instruction)
{
	ecode error;
    int c;
	tVariable *target;
    String *inputString = NULL;

	if (instruction->op1 != NULL || instruction->op2 != NULL || instruction->op3 != NULL)
		return ERR_INSTR_WRONG_OPERANDS;

    // -------------- Nacteni navratobe hodnoty -------------------------------
    error = tRuntimeStackRead(runtimeStack, -2, (void **) &target);
    if (error != ERR_OK) return error;

	// -------------- Nacteni radky ze stdin ----------------------------------
	inputString = charToString(""); // Nastaveti prazdneho retezce
	if (inputString == NULL) return ERR_MEMORY;
    while ((c = getchar()) != '\n')
    {
        if (c == EOF)
        {
            deallocString(inputString);
            return ERR_RUNTIME_OTHER;
        }
        inputString = addCharToString(inputString, c);
        if (inputString == NULL) return ERR_MEMORY;
    }

    // -------------- Nastaveni vysledku do navratove hodnoty -----------------
    target->value = inputString;
    target->semantic = STRING;

    return ERR_OK;
}

/**
 * Provedeni vestavene funkce numeric(), prevod retezce na cislo double vyuziva
 * fce stringToDouble, ignoruje pocatecni bile znaky nasledne prevadi cislo
 * @param *instruction Ukazatel na provadenou instrukci
 * @return ERR_OK pokud nenastala zadana chyba, jinak chybovy kod
 */
ecode instructionNumeric(tInstruction *instruction)
{
	ecode error;
	int success;
	tVariable *target;
	tVariable *arg;
	double *retValue;

	if (instruction->op1 != NULL || instruction->op2 != NULL || instruction->op3 != NULL)
		return ERR_INSTR_WRONG_OPERANDS;

	// -------------- Nacteni parametru ---------------------------------------
	error = tRuntimeStackRead(runtimeStack, -3, (void **) &arg);
	if (error != ERR_OK) return error;
	else if (arg == NULL) return ERR_RUNTIME_OTHER;

	// -------------- Nacteni navratobe hodnoty -------------------------------
	error = tRuntimeStackRead(runtimeStack, -2, (void **) &target);
	if (error != ERR_OK) return error;

	// -------------- Alokace mista pro navratovou hodnotu --------------------
	retValue = (double *) malloc(sizeof(double));
	if (retValue == NULL) return ERR_MEMORY;

	// -------------- Provedeni funkce ----------------------------------------
	switch (arg->semantic)
	{
		case NIL:
		case LOGICAL:
			free(retValue);
			return ERR_RUNTIME_NUMERIC_CONVERSION;
			break;
		case NUMERIC:
			*retValue = *((double *)arg->value);
			break;
		case STRING:
			success = stringToDouble((String *) arg->value, retValue);
			if (!success)
			{
				free(retValue);
				return ERR_RUNTIME_NUMERIC_CONVERSION;
			}

			break;
		default:
			free(retValue);
			return ERR_INTERNAL;
			break;
	}

	// -------------- Nastaveni vysledku do navratove hodnoty -----------------
	target->value = retValue;
	target->semantic = NUMERIC;

	return ERR_OK;
}

/**
 * Provede vestavenou funkci print, nacita parametry ze zasobniku podle
 * poctu vlozenem v prvnim parametru a tiskne na stdout.
 * @param *instruction Ukazatel na provadenou instrukci
 * @return ERR_OK pokud nenastala zadana chyba, jinak chybovy kod
 */
ecode instructionPrint(tInstruction *instruction)
{
	ecode error;
	int argCount;
	tVariable *numOfArgs;
	tVariable *arg;
	int offset = -3;

	if (instruction->op1 != NULL || instruction->op2 != NULL || instruction->op3 != NULL)
		return ERR_INSTR_WRONG_OPERANDS;

	// -------------- Nacteni poctu parametru ---------------------------------
	error = tRuntimeStackRead(runtimeStack, offset, (void **) &numOfArgs);
	if (error != ERR_OK) return error;
	else if (numOfArgs == NULL) return ERR_INTERNAL;

	argCount = *((double *)numOfArgs->value);

	// -------------- Nacitani parametru a provadeni vypisu -------------------
	for (int i = -argCount + offset ; i < offset; i++)
	{
		// argOffset = argOffset - i;
		error = tRuntimeStackRead(runtimeStack, i, (void **) &arg);
		if (error != ERR_OK) return error;
		else if (arg == NULL) return ERR_RUNTIME_OTHER;

		// -------------- Vypis termu -----------------------------------------
		switch (arg->semantic)
		{
			case NIL:
				printf("Nil");
				break;
			case LOGICAL:
				if (*((bool *)arg->value) == true)
					printf("true");
				else
					printf("false");
				break;
			case NUMERIC:
					printf("%g", *((double *)arg->value));
				break;
			case STRING:
					printf("%s", ((String *)arg->value)->data);
				break;
			default:
				return ERR_INTERNAL;
				break;
		}
	}

	return ERR_OK;
}

/**
 * Provede vestavenou fuknci typeOf(), vraci double do promenne na offset vysledku
 * @param *instruction Ukazatel na provadenou instrukci
 * @return ERR_OK pokud nenastala zadana chyba, jinak chybovy kod
 */
ecode instructionTypeOf(tInstruction *instruction)
{
	ecode error;
	tVariable *target;
	tVariable *arg;
	double *retValue;

	if (instruction->op1 != NULL || instruction->op2 != NULL || instruction->op3 != NULL)
		return ERR_INSTR_WRONG_OPERANDS;

	// -------------- Nacteni parametru ---------------------------------------
	error = tRuntimeStackRead(runtimeStack, -3, (void **) &arg);
	if (error != ERR_OK) return error;
	else if (arg == NULL) return ERR_RUNTIME_OTHER;

	// -------------- Nacteni navratove hodnoty -------------------------------
	error = tRuntimeStackRead(runtimeStack, -2, (void **) &target);
	if (error != ERR_OK) return error;

	// -------------- Alokace mista pro navratovou hodnotu --------------------
	retValue = (double *) malloc(sizeof(double));
	if (retValue == NULL) return ERR_MEMORY;

	switch (arg->semantic)
	{
		case NIL:
			*retValue = 0.0;
			break;
		case LOGICAL:
			*retValue = 1.0;
			break;
		case NUMERIC:
			*retValue = 3.0;
			break;
		case FUNCTION:
			*retValue = 6.0;
			break;
		case STRING:
			*retValue = 8.0;
			break;
		default:
			free(retValue);
			return ERR_INTERNAL;
			break;
	}

	// -------------- Nastaveni vysledku do navratove hodnoty -----------------
	target->value = retValue;
	target->semantic = NUMERIC;

	return ERR_OK;
}

/**
 * Provede vestavenou funkci len(), nastavuje delku retezce typu double do
 * promenne na offset vysledku
 * @param *instruction Ukazatel na provadenou instrukci
 * @return ERR_OK pokud nenastala zadana chyba, jinak chybovy kod
 */
ecode instructionLen(tInstruction *instruction)
{
	ecode error;
	tVariable *target;
	tVariable *arg;
	double *retValue = NULL;

	if (instruction->op1 != NULL || instruction->op2 != NULL || instruction->op3 != NULL)
		return ERR_INSTR_WRONG_OPERANDS;

	// -------------- Nacteni parametru ---------------------------------------
	error = tRuntimeStackRead(runtimeStack, -3, (void **) &arg);
	if (error != ERR_OK) return error;
	else if (arg == NULL) return ERR_RUNTIME_OTHER;

	// -------------- Nacteni navratobe hodnoty -------------------------------
	error = tRuntimeStackRead(runtimeStack, -2, (void **) &target);
	if (error != ERR_OK) return error;

	// -------------- Alokace mista pro navratovou hodnotu --------------------
	retValue = (double *) malloc(sizeof(double));
	if (retValue == NULL) return ERR_MEMORY;

	// -------------- Vypocet delky retezce -----------------------------------
	if (arg->semantic == STRING)
		*retValue = (double) stringLength(arg->value);
	else
		*retValue = 0.0;
	// -------------- Nastaveni vysledku do navratove hodnoty -----------------
	target->semantic = NUMERIC;
	target->value = retValue;

	return ERR_OK;
}

/**
 * Provede vestavenou funkci find(), nastavuje pozici vyskytu prvniho znaku
 * podretezce v hledanem retezci do promenne na offset vysledku vyuziva funkci
 * find z ial.h
 * @param *instruction Ukazatel na provadenou instrukci
 * @return ERR_OK pokud nenastala zadana chyba, jinak chybovy kod
 */
ecode instructionFind(tInstruction *instruction)
{
	ecode error;
	tVariable *target;
	tVariable *arg1;
	tVariable *arg2;
	double *retValue;

	if (instruction->op1 != NULL || instruction->op2 != NULL || instruction->op3 != NULL)
		return ERR_INSTR_WRONG_OPERANDS;

	// -------------- Nacteni parametru ---------------------------------------
	error = tRuntimeStackRead(runtimeStack, -4, (void **) &arg1);
	if (error != ERR_OK) return error;
	else if (arg1 == NULL) return ERR_RUNTIME_OTHER;

	error = tRuntimeStackRead(runtimeStack, -3, (void **) &arg2);
	if (error != ERR_OK) return error;
	else if (arg2 == NULL) return ERR_RUNTIME_OTHER;

	// Kontrola typu paramteru
	if (arg1->semantic != STRING || arg2->semantic != STRING)
		return ERR_RUNTIME_INCOMPATIBLE_TYPES;

	// -------------- Nacteni navratobe hodnoty -------------------------------
	error = tRuntimeStackRead(runtimeStack, -2, (void **) &target);
	if (error != ERR_OK) return error;

		// -------------- Alokace mista pro navratovou hodnotu --------------------
	retValue = (double *) malloc(sizeof(double));
	if (retValue == NULL) return ERR_MEMORY;

	*retValue = (double) find((String *) arg1->value, (String *) arg2->value);

	target->semantic = NUMERIC;
	target->value = retValue;

	return ERR_OK;
}

/**
 * Provede vestavenou funkci sort(), pomoci funkce sort() z ial.h (quicksort)
 * @param *instruction Ukazatel na provadenou instrukci
 * @return ERR_OK pokud nenastala zadana chyba, jinak chybovy kod
 */
ecode instructionSort(tInstruction *instruction)
{
	ecode error;
	tVariable *target;
	tVariable *arg;
	String *retValue;


	if (instruction->op1 != NULL || instruction->op2 != NULL || instruction->op3 != NULL)
		return ERR_INSTR_WRONG_OPERANDS;

	// -------------- Nacteni parametru ---------------------------------------
	error = tRuntimeStackRead(runtimeStack, -3, (void **) &arg);
	if (error != ERR_OK)
		return error;
	else if (arg == NULL)
		return ERR_RUNTIME_OTHER;
	else if (arg->semantic != STRING)
		return ERR_RUNTIME_INCOMPATIBLE_TYPES;

	// -------------- Nacteni navratobe hodnoty -------------------------------
	error = tRuntimeStackRead(runtimeStack, -2, (void **) &target);

	// -------------- Vytvoreni kopie retezce ---------------------------------
	retValue = stringCopy((String *) arg->value);

	// -------------- Provedeni razeni ----------------------------------------
	retValue = sort(retValue);
	if (retValue == NULL)
		return ERR_INTERNAL;

	target->semantic = STRING;
	target->value = retValue;

	return ERR_OK;
}
