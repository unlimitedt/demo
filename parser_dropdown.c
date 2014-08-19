// parser_dropdown.c
//
/******************************************************************************
 * Implementace interpretu imperativniho jazyka IFJ12                         *
 * Autori:                                                                    *
 *          Tomas Nestrojil (xnestr03)                                        *
 *                                                                            *
 * Dropdown parser for processing source code file using syntax analyzer      *
 ******************************************************************************
 */

#include "parser_dropdown.h"
#include "ial.h"		// Abstraktni datove typy
#include "scanner.h" 	// Lexikalni analyzator
#include "parser_expressions.h"	// Syntakticka analyza vyrazu
#include "ilist.h"	// Seznam instrukci
#include "global.h"	// Globalni promenne
#include "variable.h"	// Promenna
#include "string.h"

// Makra pro urceni typu tokenu
#define isStatementList(x) ((x) == TT_IDENTIFIER || (x) == TT_IF || (x) == TT_WHILE || (x) == TT_RETURN || (x) == TT_ELSE || (x) == TT_EOL || TT_END)
#define isStatement(x) ((x) == TT_IDENTIFIER || (x) == TT_IF || (x) == TT_WHILE || (x) == TT_RETURN || (x) == TT_EOL)
#define isFuncDef(x) ((x) == TT_FUNCTION)
#define isIdentifier(x) ((x) == TT_IDENTIFIER)
#define isEOL(x) ((x) == TT_EOL)
#define isEOF(x) ((x) == TT_EOF)
#define isLeftBracket(x) ((x) == TT_BRACKET_LEFT)
#define isRightBracket(x) ((x) == TT_BRACKET_RIGHT)
#define isAssign(x) ((x) == TT_ASSIGN)
#define isComma(x) ((x) == TT_COMMA)
#define isElse(x) ((x) == TT_ELSE)
#define isEnd(x) ((x) == TT_END)
#define isIf(x) ((x) == TT_IF)
#define isWhile(x) ((x) == TT_WHILE)
#define isReturn(x) ((x) == TT_RETURN)
#define isTerm(x) ((x) == TT_IDENTIFIER || (x) == TT_NUMBER || (x) == TT_STRING || (x) == TT_LOGIC || (x) == TT_NIL)
#define isLeftSquareBracket(x) ((x) == TT_SQUARE_BRACKET_LEFT)
#define isRightSquareBracket(x) ((x) == TT_SQUARE_BRACKET_RIGHT)
#define isLiteral(x) ((x) == TT_STRING || (x) == TT_NUMBER || (x) == TT_LOGIC || (x) == TT_NIL)
#define isString(x) ((x) == TT_STRING)
#define isColon(x) ((x) == TT_COLON)
#define isNumeric(x) ((x) == TT_NUMBER)
#define STACK_CALL_SPACE 2

typedef enum { FUNCTION_PRINT, FUNCTION_TYPEOF, FUNCTION_OTHER} ParsedFunction;

ecode determineError(TokenType token);
ecode parseFirstPass(FILE *file);
ecode parseFuncDef(FILE *file);
ecode parseParams(FILE *file, tFunctionData *functionRecord);
ecode parseParamsNext(FILE *file, tFunctionData *functionRecord);
ecode parseParamsFirstPass(FILE *file, int *paramsCount);
ecode parseStatementList(FILE *file, tFunctionData *functionRecord);
ecode parseStatement(FILE *file, tFunctionData *functionRecord);
ecode parseFile(FILE *file);
ecode parseProgram(FILE *file);
ecode parseIf(FILE *file, tFunctionData *functionRecord);
ecode parseWhile(FILE *file, tFunctionData *functionRecord);
ecode parseReturn(FILE *file, tFunctionData *functionRecord);
ecode parseAssignment(FILE *file, tFunctionData *functionRecord);
ecode parseRightHandSide(FILE *file, tFunctionData *functionRecord, int **offset, int *numberOfTemporaryItems);
ecode parseFunctionCall(FILE *file, tFunctionData *functionRecord, int **offset, int *numberOfTemporaryItems);
ecode parseSubstring(FILE *file, tFunctionData *functionRecord, int **offset, int *numberOfTemporaryItems);
ecode parseFunctionCallParams(FILE *file, tFunctionData *functionRecord, ParsedFunction function, int paramsToPush);
ecode parseParamsTerm(FILE *file, tFunctionData *functionRecord, ParsedFunction function, int paramsToPush, int *pushedParams);

ecode generateMainFunctionRecord();
ecode generateAndInsertInstruction(tIList *list, InstructionType type, void *target, void *op1, void *op2);
ecode generateAndInsertTemporaryVariable(tIList *instrList, tTableHead *symTable, SemanticType type, void *data, int **offset);
SemanticType tokenTypeToSemanticType(TokenType type);

String *generateLabelName();
String *generateVariableName();
ecode insertParamToSymbolTable(tTableHead* symbolTable, String* name);

ecode determineError(TokenType token)
{
	if (token == TT_MEMORY_ERROR)
		return ERR_MEMORY;
	else if (token == TT_LEXICAL_ERROR)
		return ERR_LEXICAL;
	else
		return ERR_SYNTAX;
}




/**
 * Provede syntaktickou analyzu zdrojoveho souboru metodou rekurzivniho sestupu,
 * pro syntaktickou analyzu vyrazu vola prislusne funkce precedencni syntakticke
 * analyzy. Zaroven pomoci prislusnych semantickych akci generuje tri adresny kod,
 * ktery bude dale interpretovan.
 * @param  file Zdrojovy souboru
 * @return      ERR_OK pokud je vse v poradku, jinak prislusny kod
 */
ecode parseFile(FILE *file)
{
	ecode error;

	// Prvni pruchod
	// Slouzi pouze k pridani identifikatoru funkci do tabulky funkci
	error = parseFirstPass(file);
	if (error)
		return error;

	// Pretoceni souboru na zacatek
	rewind(file);
	_lineNumber = 1;

	// Druhy pruchod, tentokrat uz se bude zpracovavat kazde pravidlo a pomoci
	// semantickych akci se budou generovat prislusne instrukce 3AK
	error = parseProgram(file);

	return error;
}


/**
 * Funkce pro prvni pruchod syntakticke analyzy, spocivajici pouze v tom,
 * ze hleda definice funkci, pro ktere vytvari zaznamy v tabulce funkci
 * a pocita jejich parametry, pro pozdejsi vyuziti.
 * @param  file Zdrojovy kod
 * @return      ERR_OK pokud je vse v poradku, jinak prislusny chybovy kod
 */
ecode parseFirstPass(FILE *file)
{
	ecode error;
	Token currentToken;
	tTableItem *functionRecord;
	tFunctionData *data;
	int paramsCount = 0;

	currentToken = getToken(file);
	// --------------- Token nepredstavuje funkci, prikaz ani konec souboru-----------
	if (!isStatement(currentToken.type) && !isFuncDef(currentToken.type) && !isEOF(currentToken.type))
	{   // -------------- Pro token neni pravidlo ------------------------------------
		deallocToken(currentToken);
		return determineError(currentToken.type);
	}

	// ------------------ Pruchod celym souborem -------------------------------------
	while (!isEOF(currentToken.type))
	{
		// --------------- Token function --------------------------------------------
		if (isFuncDef(currentToken.type))
		{
			// Vytvoreni zaznamu o funkci
			data = malloc(sizeof(tFunctionData));
			if (data == NULL)
				return ERR_MEMORY;
			data->firstInstruction = NULL;
			data->paramsCount = 0;
			data->varTabHead = NULL;

			// --------------- Token identifikator ------------------------------------
			currentToken = getToken(file);
			if (!isIdentifier(currentToken.type))
			{   // -------------- Pro token neni pravidlo ------------------------------
				deallocToken(currentToken);
				free(data);
				return determineError(currentToken.type);
			}

			// -------------- Kontrola predchozi definice funkce -----------------------
			functionRecord = searchItem(functionTable, currentToken.item);
			if (functionRecord != NULL)
			{
				// ---------- Funkce jiz byla definovana -------------------------------
				deallocToken(currentToken);
				free(data);
				return ERR_SEMANTICS_OTHER;
			}

			// -------------- Vlozeni jmena funkce do tabulky funkci -------------------
			error = insertItem(functionTable, currentToken.item, ITEM_FUNCTION, data);
			if (error)
			{   // ---------- Nepodarilo se pridat funkci ------------------------------
				deallocToken(currentToken);
				free(data);
				return error;
			}

			// -------------- Token leva zavorka --------------------------------------
			currentToken = getToken(file);
			if (!isLeftBracket(currentToken.type))
			{   // -------------- Pro token neni pravidlo ------------------------------
				deallocToken(currentToken);
				return determineError(currentToken.type);
			}

			// -------------- Analyza paramteru funkce - prvni pruchod -----------------
			error = parseParamsFirstPass(file, &paramsCount);
			if (error)
				return error;

			// -------------- Token prava zavorka --------------------------------------
			currentToken = getToken(file);
			if (!isRightBracket(currentToken.type))
			{   // -------------- Pro token neni pravidlo ------------------------------
				deallocToken(currentToken);
				return determineError(currentToken.type);
			}

			// -------------- Do zaznamu funkce vlozi pocet parametru --------------------
			data->paramsCount = paramsCount;
			paramsCount = 0;    // Nulovani pocitadla parametru
		}
		else
		{   // -------------- Nejedna se o definici funkce -------------------------------
			deallocToken(currentToken);
			currentToken = getToken(file);
		}
	}

	return ERR_OK;
}


/**
 * Druhy pruchod syntakticke analyzy. Jedna se o plnohodnotny pruchod, ktery
 * zaroven pomoci prislusnych semantickych akci provadi generovani triadresneho
 * kodu, pridava identifikatory promennych a navesti do TS.
 * @param  file Zdrojovy souboru
 * @return      ERR_OK pokud je vse v poradku, jinak prislusny chybovy kod
 */
ecode parseProgram(FILE *file)
{
	Token currentToken;
	ecode error;

	tFunctionData *functionRecord;  // Zaznam funkce
	tTableItem *varItem;                // Polozka tabulky symbolu
	String *name;                   // Jmeno noveho zaznamu
	String *labelName;              // Jmeno navesti
	tIListItem **label = NULL;      // Ukazatel na navesti

	// -------------- Vytvoreni zaznamu pro hlavni pseudofunkci ------------------
	error = generateMainFunctionRecord();
	if (error)
		return error;

	// -------------- Vyhledani zaznamu hlavni funkce ----------------------------
	name = charToString(MAIN_FUNCTION_NAME);
	functionRecord = searchItem(functionTable, name)->data;
	deallocString(name);

	currentToken = getToken(file);
	while(!isEOF(currentToken.type))
	{
		// -------------- Token function ---------------------------------------------
		if (isFuncDef(currentToken.type))
		{
			// -------------- Generovani navesti funkce ----------------------------------
			labelName = generateLabelName();
			if (labelName == NULL)  // Nedostatek pameti
				return ERR_MEMORY;

			// -------------- Navesti se prida do lokalni tabulky symbolu ----------------
			error = insertItem(functionRecord->varTabHead, labelName, ITEM_LABEL, NULL);
			if (error)
			{
				deallocString(labelName);
				return error;
			}
			// -------------- Vyhledani navesti v TS -------------------------------------
			varItem = searchItem(functionRecord->varTabHead, labelName);
			// -------------- Ziska se ukazatel na ukazatel ulozeny v TS -----------------
			label = (tIListItem **) &(varItem->data);

			// -------------- Generovani intrukce pro skok za definici funkce ------------
			error = generateAndInsertInstruction(instructionList, INSTR_GOTO, label, NULL, NULL);
			if (error)
			{
				return error;
			}

			// -------------- Analyza definice funkce ------------------------------------
			returnToken(currentToken);
			error = parseFuncDef(file);
			if (error)
				return error;


			// -------------- Vygeneruje se instrukce navesti pro obskoceni --------------
			error = generateAndInsertInstruction(instructionList, INSTR_LABEL, NULL, NULL, NULL);
			if (error)
			{
				return error;
			}
			// -------------- Do TS se k navesti priradi adresa instrukce ----------------
			*label = tIListGetLast(instructionList);

		}
		// -------------- Token patrici do statement ---------------------------------
		else if (isStatement(currentToken.type))
		{
			// -------------- Analyza prikazu --------------------------------------------
			returnToken(currentToken);
			error = parseStatement(file, functionRecord);
			if (error)
				return error;
		}
		else
		{   // -------------- Pro token neni pravidlo ------------------------------
			deallocToken(currentToken);
			return determineError(currentToken.type);
		}

		currentToken = getToken(file);
	}

	// -------------- Token je EOF - konec analyzy zdrojoveho kodu ---------------
	// -------------- Generovani instrukce pro konec kodu ------------------------
	error = generateAndInsertInstruction(instructionList, INSTR_HALT, NULL, NULL, NULL);
	if (error)
		return error;

	functionRecord->lastInstruction = tIListGetLast(instructionList);

	return ERR_OK;
}

// Syntakticka analyza pro neterminal <func_def>
/**
 * Syntakticka analyza definice funkce (neterminal <func_def>).
 * Generuje navesti zacatku a konce funkce, pro pozdejsi pouziti.
 * Vytvari a inicializuje tabulku symbolu pro danou funkci, nastavuje zaznam
 * funkce v tabulce funkci.
 * Ocekavany tvar podle gramatiky je
 * 		function id (<params>) EOL
 * 			<statement_list>
 *    	end EOL
 * @param  file Zdrojovy soubor
 * @return      ERR_OK pokud je vse v poradku, jinak prislusny chybovy kod
 */
ecode parseFuncDef(FILE *file)
{
	Token currentToken;
	ecode error;
	int mainVarCount;
	tFunctionData *functionRecord;
	tTableItem *functionItem;
	String *labelName, *returnName;

	// -------------- Ulozeni poctu pomocnych promennych v hlavnim tele --------
	// -------------- Vynulovani poctu pomocnych promennych pro funkci ---------
	mainVarCount = internalVariableCount;
	internalVariableCount = 0;

	// -------------- Token function -------------------------------------------
	currentToken = getToken(file);
	if (!isFuncDef(currentToken.type))
	{   // -------------- Pro token neni pravidlo ------------------------------
		deallocToken(currentToken);
		return determineError(currentToken.type);
	}

	// -------------- Token identifikator ----------------------------------------
	currentToken = getToken(file);
	if (!isIdentifier(currentToken.type))
	{   // -------------- Pro token neni pravidlo ------------------------------
		deallocToken(currentToken);
		return determineError(currentToken.type);
	}

	// -------------- Nastaveni zaznamu funkce v tabulce funkci ------------------

	functionItem = searchItem(functionTable, currentToken.item);
	// Zaznam jiz je v tabulce od prvniho pruchodu
	functionRecord = functionItem->data;
	deallocToken(currentToken); // Uvolneni pameti retezce s identifikatorem

	// -------------- Vytvori tabulku symbolu pro soucasnou funkci ---------------
	functionRecord->varTabHead = initTable();
	if (functionRecord->varTabHead == NULL)
	{
		return ERR_MEMORY;
	}


	// Inicializace poctu prvku na -pocet parametru, aby se zapocitaly pouze vnitrni promenne
	functionRecord->varTabHead->itemCount = -functionRecord->paramsCount - 1;



	// -------------- Vygeneruje instrukci navesti uvozujici funkci --------------
	error = generateAndInsertInstruction(instructionList, INSTR_LABEL, NULL, NULL, NULL);
	if (error)
	{
		return error;
	}

	// -------------- Vlozi instrukci do zaznamu funkce --------------------------
	functionRecord->firstInstruction = tIListGetLast(instructionList);

	// -------------- Token leva zavorka -----------------------------------------
	currentToken = getToken(file);
	if (!isLeftBracket(currentToken.type))
	{   // -------------- Pro token neni pravidlo ------------------------------
		deallocToken(currentToken);
		return determineError(currentToken.type);
	}

	// -------------- Analyza parametru funkce -----------------------------------
	error = parseParams(file, functionRecord);
	if (error)
		return error;

	// -------------- Vlozeni navratove hodnoty do TS ----------------------------
	returnName = charToString(RETURN_VAR_NAME);
	if (returnName == NULL)
		return ERR_MEMORY;
	error = insertParamToSymbolTable(functionRecord->varTabHead, returnName);
	if (error)
		return error;

	// -------------- Token prava zavorka ----------------------------------------
	currentToken = getToken(file);
	if (!isRightBracket(currentToken.type))
	{   // -------------- Pro token neni pravidlo ------------------------------
		deallocToken(currentToken);
		return determineError(currentToken.type);
	}

	// -------------- Token EOL za hlavickou funkce ------------------------------
	currentToken = getToken(file);
	if (!isEOL(currentToken.type))
	{   // -------------- Pro token neni pravidlo ------------------------------
		deallocToken(currentToken);
		return determineError(currentToken.type);
	}

	// -------------- Token reprezentujici sekvenci prikazu ----------------------
	currentToken = getToken(file);
	if (!isStatementList(currentToken.type))
	{   // -------------- Pro token neni pravidlo ------------------------------
		deallocToken(currentToken);
		return determineError(currentToken.type);
	}

	// -------------- Analyza sekvence prikazu -----------------------------------
	returnToken(currentToken);
	error = parseStatementList(file, functionRecord);
	if (error)
		return error;

	// -------------- Vygeneruje se navesti pro skok na konec funkce -------------
	labelName = generateLabelName();
	if (labelName == NULL)
		return ERR_MEMORY;

	// -------------- Ulozi navesti do TS ----------------------------------------
	error = insertItem(functionRecord->varTabHead, labelName, ITEM_LABEL, NULL);
	if (error)
	{
		deallocString(labelName);
		return error;
	}

	// -------------- Nastavi navesti jako posledni instrukci funkce -------------
	functionRecord->lastInstruction = tIListGetLast(instructionList);


	// -------------- Vygeneruje instrukci navratu z volani funkce ---------------
	error = generateAndInsertInstruction(instructionList, INSTR_RET, &(functionRecord->paramsCount), NULL, NULL);
	if (error)
		return error;

	// -------------- Nastavi pocitadlo promennych zpet pro hlavni telo --------
	internalVariableCount = mainVarCount;

	// -------------- Token end --------------------------------------------------
	currentToken = getToken(file);
	if (!isEnd(currentToken.type))
	{   // -------------- Pro token neni pravidlo --------------------------------
		deallocToken(currentToken);
		return determineError(currentToken.type);
	}

	// -------------- Token EOL --------------------------------------------------
	currentToken = getToken(file);
	if (!isEOL(currentToken.type))
	{   // -------------- Pro token neni pravidlo --------------------------------
		deallocToken(currentToken);
		return determineError(currentToken.type);
	}

	return ERR_OK;
}


/**
 * Syntakticka analyza pro seznam parametru v definici funkce.
 * Zpracovava neterminal <params>, pokud ma funkce vice jak jeden parametr,
 * zavola funkci parseParamsNext(FILE *file, tFunctionData *functionRecord)
 * pro jejich zpracovani
 * Pridava polozky identifikatoru parametru do TS definovane funkce
 *
 * @param  file           Zdrojovy soubor
 * @param  functionRecord Zaznam z tabulky funkci pro zpracovavanou funkci
 * @return                ERR_OK pokud je vse v poradku, jinak prislusny chybovy kod
 */
ecode parseParams(FILE *file, tFunctionData *functionRecord)
{
	Token currentToken;
	ecode error;

	// -------------- Token prava zavorka nebo identifikator ---------------------
	currentToken = getToken(file);
	if (!isIdentifier(currentToken.type) && !isRightBracket(currentToken.type))
	{   // -------------- Pro token neni pravidlo ------------------------------
		deallocToken(currentToken);
		return determineError(currentToken.type);
	}
	// -------------- Token identifikator ----------------------------------------
	else if (isIdentifier(currentToken.type))
	{
		// -------------- Vlozi identifikator parametru do TS ------------------
		error = insertParamToSymbolTable(functionRecord->varTabHead, currentToken.item);
		if (error)
		{
			deallocToken(currentToken);
			return error;
		}
	}
	// -------------- Token prava zavorka ----------------------------------------
	else    // currentToken.type == TT_BRACKET_RIGHT
	{
		// -------------- Token prava zavorka -----------------------------------
		// -------------- Vrati token scanneru, protoze nepatri do pravidla -----
		returnToken(currentToken);
		return ERR_OK;
	}


	error = ERR_OK;
	// -------------- Analyza dalsich parametru ----------------------------------
	currentToken = getToken(file);
	if (isRightBracket(currentToken.type))
		returnToken(currentToken); // Vraceni tokenu prave zavorky
	else	// Token neni prava zavorka, parametry pokracuji
	{
		returnToken(currentToken);
		error = parseParamsNext(file, functionRecord);
	}


	return error;
}

/**
 * Syntakticka analyza parametru definovane funkce, krome prvniho
 * Pridava polozky identifikatoru parametru do TS definovane funkce
 * Zpracovava neterminal <params_n>
 * @param  file           Zdrojovy soubor
 * @param  functionRecord Zaznam z tabulky funkci pro zpracovavanou funkci
 * @return                ERR_OK pokud je vse v poradku, jinak prislusny chybovy kod
 */
ecode parseParamsNext(FILE *file, tFunctionData *functionRecord)
{
	ecode error;
	Token currentToken;
	currentToken = getToken(file);

	while (!isRightBracket(currentToken.type))
	{
		// -------------- Token carka --------------------------------------------
		if (!isComma(currentToken.type))
		{   // -------------- Pro token neni pravidlo ----------------------------
			deallocToken(currentToken);
			return determineError(currentToken.type);
		}

		// -------------- Token identifikator ------------------------------------
		currentToken = getToken(file);
		if (!isIdentifier(currentToken.type))
		{   // -------------- Pro token neni pravidlo ----------------------------
			deallocToken(currentToken);
			return determineError(currentToken.type);
		}

		// -------------- Pridani identifikatoru do TS ---------------------------
		error = insertParamToSymbolTable(functionRecord->varTabHead, currentToken.item);
		if (error)
		{
			deallocToken(currentToken);
			return error;
		}
		// Nacti dalsi token
		currentToken = getToken(file);
	}

	// -------------- Token prava zavorka vrati se scanneru ----------------------
	returnToken(currentToken);

	return ERR_OK;
}



/**
 * Syntakticka analyza sekvence prikazu uvnitr bloku - telo funkce, if, else
 * nebo while.
 * V pripade tokenu pro ktere jsou epsilon pravidla dojde k dokonceni zpracovani
 * sekvence prikazu, jinak zavola funkci pro zpracovani prikazu
 * int parseStatement(FILE *file, tFunctionData *functionRecord)
 * @param  file           Zdrojovy soubor
 * @param  functionRecord Zaznam z tabulky funkci pro zpracovavanou funkci
 * @return ERR_OK pokud je vse v poradku, jinak prislusny chybovy kod
 */
ecode parseStatementList(FILE *file, tFunctionData *functionRecord)
{
	ecode error;
	Token currentToken;

	currentToken = getToken(file);

	// -------------- Dokud neni token else nebo end - epsilon pravidla ----------
	while (!isElse(currentToken.type) && !isEnd(currentToken.type))
	{
		// -------------- Token znamenajici prikaz -------------------------------
		if (!isStatement(currentToken.type))
		{   // -------------- Pro token neni pravidlo ----------------------------
			deallocToken(currentToken);
			return determineError(currentToken.type);
		}

		// -------------- Analyza prikazu --------------------------------------
		returnToken(currentToken);
		error = parseStatement(file, functionRecord);
		if (error)
			return error;

		currentToken = getToken(file);
	}

	returnToken(currentToken);
	return ERR_OK;
}

// Syntakticka analyza prikazu
/**
 * Syntakticka analyza prikazu. Pro jednotlive prikazy vola prislusne funkce
 * pro jejich syntaktickou analyzu
 * @param  file           Zdrojovy soubor
 * @param  functionRecord Zaznam z tabulky funkci pro zpracovavanou funkci
 * @return                ERR_OK pokud je vse v poradku, jinak prislusny chybovy kod
 */
ecode parseStatement(FILE *file, tFunctionData *functionRecord)
{
	Token currentToken;
	ecode error;
	// -------------- Token znamenajici prikaz -----------------------------------
	currentToken = getToken(file);
	if (!isStatement(currentToken.type))
	{   // -------------- Pro token neni pravidlo --------------------------------
		deallocToken(currentToken);
		return determineError(currentToken.type);
	}

	// -------------- Token if ---------------------------------------------------
	if (isIf(currentToken.type))
	{
		// -------------- Analyza prikazu if -------------------------------------
		returnToken(currentToken);
		error = parseIf(file, functionRecord);
	}
	else if (isWhile(currentToken.type))
	{
		// -------------- Analyza prikazu while ----------------------------------
		returnToken(currentToken);
		error = parseWhile(file, functionRecord);
	}
	else if (isReturn(currentToken.type))
	{
		// -------------- Analyza prikazu return ---------------------------------
		returnToken(currentToken);
		error = parseReturn(file, functionRecord);
	}
	else if (isIdentifier(currentToken.type))
	{
		// -------------- Analyza prikazu prirazeni ------------------------------
		returnToken(currentToken);
		error = parseAssignment(file, functionRecord);
	}
	else if (isEOL(currentToken.type))
	{
		// -------------- Analyza prikazu if -------------------------------------
		return ERR_OK;
	}


	return error;
}

// Syntakticka analyza prikazu if
/**
 * Syntakticka analyza prikazu if s nasledujici syntaxi
 * if expr EOL
 *    <statement-list>
 * else
 * 	  <statement-list>
 * end
 *
 * S vyuzitim pomocneho zasobniku pro navesti generuje navesti pro skoky
 * v ramci podminky - prvni skok na telo else, druhy skok na konec tela else
 * @param  file           Zdrojovy soubor
 * @param  functionRecord Zaznam z tabulky funkci pro zpracovavanou funkci
 * @return                ERR_OK pokud je vse v poradku, jinak prislusny chybovy kod
 */
ecode parseIf(FILE *file, tFunctionData *functionRecord)
{
	Token currentToken;
	ecode error;
	int numberOfTemporaryItems;
	int *condition;

	tTableItem *label;

	String *labelElseName;
	String *labelEndName;
	tIListItem **labelElse;
	tIListItem **labelEnd;

	// -------------- Token if ---------------------------------------------------
	currentToken = getToken(file);
	if (!isIf(currentToken.type))
	{   // -------------- Pro token neni pravidlo --------------------------------
		deallocToken(currentToken);
		return determineError(currentToken.type);
	}

	// -------------- Vygeneruje se navesti pro skok na else ---------------------
	labelElseName = generateLabelName();
	if (labelElseName == NULL)
		return ERR_MEMORY;
	// -------------- Ulozi navesti do TS ----------------------------------------
	error = insertItem(functionRecord->varTabHead, labelElseName, ITEM_LABEL, NULL);
	if (error)
	{
		deallocString(labelElseName);
		return error;
	}

	// -------------- Na zasobnik navesti ulozi ukazatel na data navesti ---------
	label = searchItem(functionRecord->varTabHead, labelElseName);
	labelElse = (tIListItem **)  &(label->data);


	// -------------- Vyhodnoceni vyrazu podminky --------------------------------
	error = parseExpression(file, instructionList, functionRecord->varTabHead, &condition, &numberOfTemporaryItems);
	if (error)
		return error;

	// TODO provest optimalizaci docasnych promennych - snizit pocet prvku TS a generovat instrukce pro mazani
	// Instrukce pro preskoceni tela if pokud neni splnena podminka
	// -------------- Vygeneruje se instrukce pro skok na telo else --------------
	error = generateAndInsertInstruction(instructionList, INSTR_IFGOTO, labelElse, condition, NULL);
	if (error)
		return error;


	// -------------- Token EOL --------------------------------------------------
	currentToken = getToken(file);
	if (!isEOL(currentToken.type))
	{   // -------------- Pro token neni pravidlo --------------------------------
		deallocToken(currentToken);
		return determineError(currentToken.type);
	}

	// -------------- Analyza sekvence prikazu v tele if -------------------------
	error = parseStatementList(file, functionRecord);
	if (error)
		return error;


	// -------------- Token else -------------------------------------------------
	currentToken = getToken(file);
	if (!isElse(currentToken.type))
	{   // -------------- Pro token neni pravidlo --------------------------------
		deallocToken(currentToken);
		return determineError(currentToken.type);
	}

	// -------------- Token EOL---------------------------------------------------
	currentToken = getToken(file);
	if (!isEOL(currentToken.type))
	{   // -------------- Pro token neni pravidlo --------------------------------
		deallocToken(currentToken);
		return determineError(currentToken.type);
	}


	// -------------- Vygeneruje jmeno pro navesti a vlozi ho do TS --------------
	labelEndName = generateLabelName();
	error = insertItem(functionRecord->varTabHead, labelEndName, ITEM_LABEL, NULL);
	if (error)
	{
		deallocString(labelEndName);
		return error;
	}
	// -------------- Na zasobnik se ulozi ukazatel na nove navesti --------------
	label = searchItem(functionRecord->varTabHead, labelEndName);
	labelEnd = (tIListItem **) &(label->data);

	// -------------- Vygeneruje se instrukce pro skok za telo else --------------
	error = generateAndInsertInstruction(instructionList, INSTR_GOTO, labelEnd, NULL, NULL);
	if (error)
		return error;

	// -------------- Vygeneruje se instrukce s navestim pro telo else -----------
	error = generateAndInsertInstruction(instructionList, INSTR_LABEL, NULL, NULL, NULL);
	if (error)
		return error;

	// -------------- Nastavi ukazatel skoku na danou instrukci ------------------
	*labelElse = tIListGetLast(instructionList);

	// -------------- Analyza sekvence prikazu v tele else -----------------------
	error = parseStatementList(file, functionRecord);
	if (error)
		return error;

	// -------------- Token end --------------------------------------------------
	currentToken = getToken(file);
	if (!isEnd(currentToken.type))
	{   // -------------- Pro token neni pravidlo --------------------------------
		deallocToken(currentToken);
		return determineError(currentToken.type);
	}

	// -------------- Token EOL --------------------------------------------------
	currentToken = getToken(file);
	if (!isEOL(currentToken.type))
	{   // -------------- Pro token neni pravidlo --------------------------------
		deallocToken(currentToken);
		return determineError(currentToken.type);
	}

	// -------------- Vygeneruje se instrukce navesti za telem else --------------
	error = generateAndInsertInstruction(instructionList, INSTR_LABEL, NULL, NULL, NULL);
	if (error)
		return error;

	// -------------- Nastavi ukazatel skoku na danou instrukci ------------------
	*labelEnd = tIListGetLast(instructionList);

	return ERR_OK;
}

/**
 * Syntakticka analyza prikazu while s nasledujici syntaxi
 * while expr EOL
 *    <statement-list>
 * end
 *
 * S vyuzitim pomocneho zasobniku pro navesti generuje navesti pro skoky
 * v ramci cyklu - prvni skok za telo while, druhy skok pred kontrolu podminky
 * @param  file           Zdrojovy soubor
 * @param  functionRecord Zaznam z tabulky funkci pro zpracovavanou funkci
 * @return                ERR_OK pokud je vse v poradku, jinak prislusny chybovy kod
 */
ecode parseWhile(FILE *file, tFunctionData *functionRecord)
{
	Token currentToken;
	ecode error;
	int numberOfTemporaryItems;
	int *condition; // Ukazatel na offset vysledku vyrazu v podmince

	String *labelConditionName;
	String *labelEndName;
	tTableItem *label;
	tIListItem **labelCondition;
	tIListItem **labelEnd;

	// -------------- Token while ------------------------------------------------
	currentToken = getToken(file);
	if (!isWhile(currentToken.type))
	{   // -------------- Pro token neni pravidlo --------------------------------
		deallocToken(currentToken);
		return determineError(currentToken.type);
	}

	// -------------- Vygeneruje se navesti pro skok na while --------------------
	labelConditionName = generateLabelName();
	// -------------- Ulozi navesti do TS ----------------------------------------
	error = insertItem(functionRecord->varTabHead, labelConditionName, ITEM_LABEL, NULL);
	if (error)
	{
		deallocString(labelConditionName);
		return error;
	}

	// -------------- Ulozi se ukazatel na navesti while -------------------------
	label = searchItem(functionRecord->varTabHead, labelConditionName);
	labelCondition = (tIListItem **) &(label->data);

	// -------------- Vygeneruje se instrukce navesti while ----------------------
	error = generateAndInsertInstruction(instructionList, INSTR_LABEL, NULL, NULL, NULL);
	if (error)
		return error;

	// -------------- Nastavi ukazatel skoku na danou instrukci ------------------
	*labelCondition = tIListGetLast(instructionList);

	// -------------- Vygeneruje se navesti pro skok za telo while ---------------
	labelEndName = generateLabelName();
	error = insertItem(functionRecord->varTabHead, labelEndName, ITEM_LABEL, NULL);
	if (error)
	{
		deallocString(labelEndName);
		return error;
	}

	// -------------- Na zasobnik se ulozi ukazatel na nove navesti --------------
	label = searchItem(functionRecord->varTabHead, labelEndName);
	labelEnd = (tIListItem **) &(label->data);

	// -------------- Vyhodnoceni vyrazu podminky --------------------------------
	error = parseExpression(file, instructionList, functionRecord->varTabHead, &condition, &numberOfTemporaryItems);
	if (error)
		return error;
	// TODO provest optimalizaci docasnych promennych - snizit pocet prvku TS a generovat instrukce pro mazani


	// -------------- Vygeneruje se instrukce pro skok za telo while -------------
	error = generateAndInsertInstruction(instructionList, INSTR_IFGOTO, labelEnd, condition, NULL);
	if (error)
		return error;

	// -------------- Token EOL --------------------------------------------------
	currentToken = getToken(file);
	if (!isEOL(currentToken.type))
	{   // -------------- Pro token neni pravidlo ------------------------------
		deallocToken(currentToken);
		return determineError(currentToken.type);
	}

	// -------------- Analyza sekvence prikazu v tele while ----------------------
	error = parseStatementList(file, functionRecord);
	if (error)
		return error;

	// -------------- Token end --------------------------------------------------
	currentToken = getToken(file);
	if (!isEnd(currentToken.type))
	{   // -------------- Pro token neni pravidlo --------------------------------
		deallocToken(currentToken);
		return determineError(currentToken.type);
	}

	// -------------- Token EOL --------------------------------------------------
	currentToken = getToken(file);
	if (!isEOL(currentToken.type))
	{   // -------------- Pro token neni pravidlo --------------------------------
		deallocToken(currentToken);
		return determineError(currentToken.type);
	}

	// -------------- Vygeneruje instrukci pro skok na navesti pred podminkou ----
	error = generateAndInsertInstruction(instructionList, INSTR_GOTO, labelCondition, NULL, NULL);
	if (error)
		return error;

	// -------------- Vygeneruje se instrukce navesti za telem while -------------
	error = generateAndInsertInstruction(instructionList, INSTR_LABEL, NULL, NULL, NULL);
	if (error)
		return error;

	// -------------- Nastavi ukazatel skoku na danou instrukci ------------------
	*labelEnd = tIListGetLast(instructionList);

	return ERR_OK;
}


/**
 * Syntakticka analyza prikazu return s nasledujici syntaxi
 * return expr EOL
 * Generuje skok na posledni instrukci funkce
 * @param  file           Zdrojovy soubor
 * @param  functionRecord Zaznam z tabulky funkci pro zpracovavanou funkci
 * @return                ERR_OK pokud je vse v poradku, jinak prislusny chybovy kod
 */
ecode parseReturn(FILE *file, tFunctionData *functionRecord)
{
	Token currentToken;
	ecode error;
	int *expression;
	int numberOfTemporaryItems;
	String *returnName;
	tTableItem *varItem;

	// -------------- Token return -----------------------------------------------
	currentToken = getToken(file);
	if (!isReturn(currentToken.type))
	{   // -------------- Pro token neni pravidlo --------------------------------
		deallocToken(currentToken);
		return determineError(currentToken.type);
	}


	// -------------- Vyhodnoceni vyrazu -----------------------------------------
	error = parseExpression(file, instructionList, functionRecord->varTabHead, &expression, &numberOfTemporaryItems);
	// TODO: Optimalizace poctu pomocnych promennych
	if (error)
		return error;

	returnName = charToString(RETURN_VAR_NAME);
	if (returnName == NULL)
		return ERR_MEMORY;

	varItem = searchItem(functionRecord->varTabHead, returnName);
	// Pokud se nenajde, jedna se o pseudo funkci $main - neni kam vracet
	if (varItem != NULL)
	{
		error = generateAndInsertInstruction(instructionList, INSTR_MOV_STACK, varItem->data, expression, NULL);
		if (error)
		{
			deallocString(returnName);
			return error;
		}
	}

	deallocString(returnName);
	// Skok na konec funkce
	error = generateAndInsertInstruction(instructionList, INSTR_GOTO, &(functionRecord->lastInstruction), NULL, NULL);
	if (error)
		return error;

	// -------------- Token EOL---------------------------------------------------
	currentToken = getToken(file);
	if (!isEOL(currentToken.type))
	{   // -------------- Pro token neni pravidlo --------------------------------
		deallocToken(currentToken);
		return determineError(currentToken.type);
	}

	return ERR_OK;
}


/**
 * Syntakticka analyza prikazu prirazeni
 * Ulozi promennou na leve strane do TS a zavola funkci
 * parseRightHandSide(FILE *file, tFunctionData *functionRecord, int **offset, int *numberOfTemporaryItems)
 * pro zpracovani prave strany prirazeni
 * @param  file           Zdrojovy soubor
 * @param  functionRecord Zaznam z tabulky funkci pro zpracovavanou funkci
 * @return                ERR_OK pokud je vse v poradku, jinak prislusny chybovy kod
 */
ecode parseAssignment(FILE *file, tFunctionData *functionRecord)
{

	Token currentToken;
	ecode error;
	int *offset;
	int *offsetRHS;
	int numberOfTemporaryItems;
	tTableItem *varItem;

	// -------------- Token identifikator ----------------------------------------
	currentToken = getToken(file);
	if (!isIdentifier(currentToken.type))
	{   // -------------- Pro token neni pravidlo --------------------------------
		deallocToken(currentToken);
		return determineError(currentToken.type);
	}


	// -------------- Kontrola, zda neni identifikator v tabulce funkci ----------
	if (searchItem(functionTable, currentToken.item) != NULL)
	{   // -------------- Identifikator definovan jako funkce -> semanticka chyba
		deallocToken(currentToken);
		return ERR_SEMANTICS_OTHER;
	}

	// -------------- Pokud neni promenna v TS tak ji tam pridej -----------------
	varItem = searchItem(functionRecord->varTabHead, currentToken.item);
	if (varItem == NULL)
	{
		offset = malloc(sizeof(int));
		if (offset == NULL)
		{
			deallocToken(currentToken);
			return ERR_MEMORY;
		}
		*offset = functionRecord->varTabHead->itemCount + 1;
		error = insertItem(functionRecord->varTabHead, currentToken.item, ITEM_VAR, offset);
		if (error)
		{
			deallocToken(currentToken);
			free(offset);
			return error;
		}
	}
	else
	{
		offset = varItem->data;
		deallocToken(currentToken);
	}

	// -------------- Token prirazeni --------------------------------------------
	currentToken = getToken(file);
	if (!isAssign(currentToken.type))
	{   // -------------- Pro token neni pravidlo ------------------------------
		deallocToken(currentToken);
		return determineError(currentToken.type);
	}

	// -------------- Zpracovat pravou stranu prirazeni --------------------------
	error = parseRightHandSide(file, functionRecord, &offsetRHS, &numberOfTemporaryItems);
	if (error)
		return error;
	// TODO: optimalizace pomocnych promennych

	error = generateAndInsertInstruction(instructionList, INSTR_MOV_STACK, offset, offsetRHS, NULL);
	if (error)
		return error;

	// -------------- Token EOL---------------------------------------------------
	currentToken = getToken(file);
	if (!isEOL(currentToken.type))
	{   // -------------- Pro token neni pravidlo --------------------------------
		deallocToken(currentToken);
		return determineError(currentToken.type);
	}

	return ERR_OK;
}


/**
 * Analyza prave strany prirazeni, na zaklad max. 2 tokenu se rozhodne, zda se
 * jedna o vyraz, volani funkce nebo vyber podretezce
 * @param  file                   Zdrojovy soubor
 * @param  functionRecord         Zaznam z tabulky funkci pro zpracovavanou funkci
 * @param  offset                 Pointer na pointer pro ulozeni offsetu vysledku prave strany
 * @param  numberOfTemporaryItems Pocet docasnych promennych (slouzi k optimalizaci)
 * @return                        ERR_OK pokud je vse v poradku, jinak prislusny chybovy kod
 */
ecode parseRightHandSide(FILE *file, tFunctionData *functionRecord, int **offset, int *numberOfTemporaryItems)
{
	ecode error;
	Token currentToken, previousToken;


	// -------------- Token leva zavorka -> vyraz --------------------------------
	currentToken = getToken(file);
	if (isLeftBracket(currentToken.type))
	{
		returnToken(currentToken);
		error = parseExpression(file, instructionList, functionRecord->varTabHead, offset, numberOfTemporaryItems);
		if (error)
			return error;
		// TODO: optimalizace docasnych promennych
	}
	// -------------- Token reprezentujici term ----------------------------------
	else if (isTerm(currentToken.type))
	{
		previousToken = currentToken;
		currentToken = getToken(file);

		// -------------- Identifikator a leva zavorka -> volani funkce --------------
		if (isIdentifier(previousToken.type) && isLeftBracket(currentToken.type))
		{
			returnToken(currentToken);
			returnToken(previousToken);
			error = parseFunctionCall(file, functionRecord, offset, numberOfTemporaryItems);
			if (error)
				return error;
		}
		// -------------- Term a leva hranata zavorka -> podretezec ------------------
		else if(isLeftSquareBracket(currentToken.type))
		{
			returnToken(currentToken);
			returnToken(previousToken);
			error = parseSubstring(file, functionRecord, offset, numberOfTemporaryItems);
			if (error)
				return error;
		}
		// -------------- Term a jiny znak -> vyraz ----------------------------------
		else
		{
			returnToken(currentToken);
			returnToken(previousToken);
			error = parseExpression(file, instructionList, functionRecord->varTabHead, offset, numberOfTemporaryItems);
			if (error)
				return error;
		}
	}
	// -------------- Token nevyhovuje pravidlu ----------------------------------
	else
	{
		deallocToken(currentToken);
		return determineError(currentToken.type);
	}

	return ERR_OK;
}

/**
 * Syntakticka analyza volani funkce
 * Provadi syntaktickou analyzu volani funkce a pripojene semantikcke akce pro
 * kontrolu, zda je identifikator funkce v tabulce funkci
 * Generuje instrukce pro volani funkce, vlozeni navratove hodnoty na zasobnik,
 * a nacteni navratove hodnoty ze zasobniku
 * @param  file                   Zdrojovy soubor
 * @param  functionRecord         Zaznam z tabulky funkci pro zpracovavanou funkci
 * @param  offset                 Pointer na pointer pro ulozeni offsetu vysledku prave strany
 * @param  numberOfTemporaryItems Pocet docasnych promennych (slouzi k optimalizaci)
 * @return                        ERR_OK pokud je vse v poradku, jinak prislusny chybovy kod
 */
ecode parseFunctionCall(FILE *file, tFunctionData *functionRecord, int **offset, int *numberOfTemporaryItems)
{
	ecode error;

	Token currentToken;
	*numberOfTemporaryItems = 0;
	String *functionName;
	tTableItem *calledFunction;
	ParsedFunction function;

	// -------------- Token identifikator ----------------------------------------
	currentToken = getToken(file);
	if (!isIdentifier(currentToken.type))
	{   // -------------- Pro token neni pravidlo --------------------------------
		deallocToken(currentToken);
		return determineError(currentToken.type);
	}

	// Ulozeni jmena promenne
	functionName = currentToken.item;

	// -------------- Token leva zavorka -----------------------------------------
	currentToken = getToken(file);
	if (!isLeftBracket(currentToken.type))
	{   // -------------- Pro token neni pravidlo --------------------------------
		deallocToken(currentToken);
		deallocString(functionName);
		return determineError(currentToken.type);
	}

	// -------------- Nalezeni zaznamu funkce ------------------------------------
	calledFunction = searchItem(functionTable, functionName);

	// -------------- Nedefinovana funkce ----------------------------------------
	if (calledFunction == NULL)
	{
		deallocString(functionName);
		return ERR_SEMANTICS_UNDEFINED_FUNCTION;
	}

	// -------------- Zjisteni, jaka funkce se zpracovava ------------------------
	if (strcmp(functionName->data, "typeOf") == 0)
	{
		function = FUNCTION_TYPEOF;
	}
	else if (strcmp(functionName->data, "print") == 0)
	{
		function = FUNCTION_PRINT;
	}
	else
	{
		function = FUNCTION_OTHER;
	}

	deallocString(functionName);

	// -------------- Zpracovani parametru ---------------------------------------
	error = parseFunctionCallParams(file, functionRecord, function, ((tFunctionData *) calledFunction->data)->paramsCount);
	if (error)
		return error;

	// -------------- Token prava zavorka ----------------------------------------
	currentToken = getToken(file);
	if (!isRightBracket(currentToken.type))
	{   // -------------- Pro token neni pravidlo --------------------------------
		deallocToken(currentToken);
		return determineError(currentToken.type);
	}

	 // -------------- Vytvoreni docasne promenne pro navratovou hodnotu ----------
    error = generateAndInsertTemporaryVariable(instructionList, functionRecord->varTabHead, NIL, NULL, offset);
    if (error)
            return error;

    // -------------- Vlozeni docasne promenne na zasobnik -----------------------
	error = generateAndInsertInstruction(instructionList, INSTR_PUSH_STACK, *offset, NULL, NULL);
	if (error)
		return error;

	// -------------- Generovani instrukce pro zavolani funkce -------------------

	error = generateAndInsertInstruction(instructionList, INSTR_CALL, calledFunction->data, NULL, NULL);
	if (error)
		return error;

	// -------------- Nacteni navratove hodnoty ze zasobniku ---------------------
	error = generateAndInsertInstruction(instructionList, INSTR_POP, *offset, NULL, NULL);
	if (error)
		return error;


	return ERR_OK;
}

/**
 * {Provadi syntaktickou analyzu parametru volane funkce
 * a kotroluje, zda nejsou parametry volani funkce nazvy funkci (s vyjimkou
 * funkce typeOf).
 * Generuje instrukce triadresneho kodu pro vlozeni parametru na zasobnik
 * Pokud je parametru mene, nez v definici funkce, na zasobnik vlozi
 * odpovidajici pocet vychozich promennych typu NIL
 * Pokud je parametru vice, nez v definici funkce, na zasobnik se vlozi
 * jen definovany pocet, ostatni parametry vlozeny nejsou, ale je provedena
 * jejich syntakticka analyza
 * Pro zpracovani parametru termu pouziva funkci
 * int parseParamsTerm(FILE *file, tFunctionData *functionRecord, ParsedFunction function, int paramsToPush, int *pushedParams)
 *
 * @param  file           Zdrojovy soubor
 * @param  functionRecord Zaznam z tabulky funkci pro zpracovavanou funkci
 * @param  function       Zpracovavana funkce
 * @param  paramsToPush   Pocet parametru, ktere ma se maji vlozit na zasobnik
 * @return                ERR_OK pokud je vse v poradku, jinak prislusny chybovy kod
 */
ecode parseFunctionCallParams(FILE *file, tFunctionData *functionRecord, ParsedFunction function, int paramsToPush)
{
	ecode error;
	Token currentToken;
	int pushedParams = 0;
	tVariable *paramVar;

	// -------------- Token reprezentujici term ----------------------------------
	currentToken = getToken(file);
	if (!isTerm(currentToken.type) && !isRightBracket(currentToken.type))
	{   // -------------- Pro token neni pravidlo --------------------------------
		deallocToken(currentToken);
		return determineError(currentToken.type);
	}

	// -------------- Token prava zavorka ------------------------------------
	if (isRightBracket(currentToken.type))
	{
		// Doplneni parametru pokud chybi
		while (pushedParams < paramsToPush && function != FUNCTION_PRINT)
		{
			// TODO: Zaradit do nejake struktury pro literaly
			error = createNewVariable(&paramVar, NIL, NULL);
			if (error)
				return error;

			error = generateAndInsertInstruction(instructionList, INSTR_PUSH, paramVar, NULL, NULL);
			if (error)
				return error;
			pushedParams++;
		}

		returnToken(currentToken);
		return ERR_OK;
	}

	returnToken(currentToken);

	error = parseParamsTerm(file, functionRecord, function, paramsToPush, &pushedParams);
	if (error)
		return error;

	currentToken = getToken(file);

	while (!isRightBracket(currentToken.type))
	{
		// -------------- Token carka --------------------------------------------
		if (!isComma(currentToken.type))
		{   // -------------- Pro token neni pravidlo ----------------------------
			deallocToken(currentToken);
			return determineError(currentToken.type);
		}

		// -------------- Parametr funkce ----------------------------------------
		error = parseParamsTerm(file, functionRecord, function, paramsToPush, &pushedParams);
		if (error)
			return error;

		// Nacti dalsi token
		currentToken = getToken(file);
	}

	// -------------- Token je prava zavorka - konec parametru -------------------
	returnToken(currentToken);

	// Doplneni parametru pokud chybi
	while (pushedParams < paramsToPush && function != FUNCTION_PRINT)
	{
		// TODO: Zaradit do nejake struktury pro literaly
		error = createNewVariable(&paramVar, NIL, NULL);
		if (error)
			return error;

		error = generateAndInsertInstruction(instructionList, INSTR_PUSH, paramVar, NULL, NULL);
		if (error)
			return error;
		pushedParams++;
	}

	// Pro fukci print prida za parametry promennou s poctem parametru
	if (function == FUNCTION_PRINT)
	{
		double *tmpDbl;
		tmpDbl = malloc(sizeof(double));
		if (tmpDbl == NULL)
			return ERR_MEMORY;
		*tmpDbl = pushedParams;
		// TODO: Zaradit do nejake struktury pro literaly
		error = createNewVariable(&paramVar, NUMERIC, tmpDbl);
		if (error)
			return error;

		error = generateAndInsertInstruction(instructionList, INSTR_PUSH, paramVar, NULL, NULL);
		if (error)
			return error;
		pushedParams++;
	}

	return ERR_OK;
}

/**
 * Funkce poskytujici semanticke akce pro kontrolu, zda je indentifikator
 * v TS dane funkce, pripadne zda se jedna o identifikator funkce (chyba
 * krome funkce typeOf)
 * Zaroven generuje triadresny kod pro vkladani termu na zasobnik
 * @param  file           Zdrojovy soubor
 * @param  functionRecord Zaznam z tabulky funkci pro zpracovavanou funkci
 * @param  function       Zpracovavana funkce (PRINT, TYPEOF, OTHER)
 * @param  paramsToPush   Pocet parametru, ktere se maji vlozit na zasobnik
 * @param  pushedParams   Pocet parametru, ktere uz byly vlozeny na zasobnik
 * @return                ERR_OK pokud je vse v poradku, jinak prislusny chybovy kod
 */
ecode parseParamsTerm(FILE *file, tFunctionData *functionRecord, ParsedFunction function, int paramsToPush, int *pushedParams)
{
	ecode error;
	Token currentToken;
	tVariable *paramVar;
	tTableItem *varRecord;

	currentToken = getToken(file);

	// -------------- Token reprezentujici identifikator -------------------------
	if (isIdentifier(currentToken.type))
	{
		// -------------- Pokud neni funkce typeOf, tak nesmi byt id funkce ------
		if (searchItem(functionTable, currentToken.item) != NULL)
		{
			deallocToken(currentToken);
			if (function != FUNCTION_TYPEOF)
				return ERR_SEMANTICS_OTHER;
			else
			{
				if ((*pushedParams) < paramsToPush)
				{
					// TODO: Zaradit do nejake struktury pro literaly
					// -------------- Promenna pro parametr --------------------------
					error = createNewVariable(&paramVar, FUNCTION, NULL);
					if (error)
						return error;

					// -------------- Vlozeni parametru na zasobnik ------------------
					error = generateAndInsertInstruction(instructionList, INSTR_PUSH, paramVar, NULL, NULL);
					if (error)
						return error;
					(*pushedParams)++;
				}
			}
		}
		else
		{
			// -------------- Identifikator neni funkce -> hledame v TS --------------
			varRecord = searchItem(functionRecord->varTabHead, currentToken.item);
			deallocToken(currentToken);
			// -------------- Identifikator nebyl nalezen ----------------------------
			if (varRecord == NULL)
			{
				return ERR_SEMANTICS_UNDEFINED_VARIABLE;
			}
			// -------------- Identifikator nalezen, vlozi se na zasobnik ------------
			else
			{
				if ((*pushedParams) < paramsToPush || function == FUNCTION_PRINT)
				{
					error = generateAndInsertInstruction(instructionList, INSTR_PUSH_STACK, varRecord->data, NULL, NULL);
					if (error)
						return error;
					(*pushedParams)++;
				}
			}
		}
	}
	else if (isLiteral(currentToken.type))
	{
		// TODO: Zaradit do nejake struktury pro literaly
		if ((*pushedParams) < paramsToPush || function == FUNCTION_PRINT)
		{
			error = createNewVariable(&paramVar, tokenTypeToSemanticType(currentToken.type), currentToken.item);
			if (error)
				return error;
			error = generateAndInsertInstruction(instructionList, INSTR_PUSH, paramVar, NULL, NULL);
			if (error)
				return error;
			(*pushedParams)++;
		}
		else
		{
			deallocToken(currentToken);
		}
	}
	else
	{
		deallocToken(currentToken);
		return determineError(currentToken.type);
	}

	return ERR_OK;
}

/**
 * Syntakticka analyza prave strany prirazeni pro vyber podretezce.
 * Vlozi offsety termu do tabulky symbolu, generuje instrukce pro vlozeni termu
 * na prislusne offsety a vytvari pomocnou strukturu tRange, ktera obsahuje
 * offsety hranic podretezce, pokud je nektera z hranic vynechana, jeji offset je NULL
 * @param  file                   Zdrojovy soubor
 * @param  functionRecord         Zaznam z tabulky funkci pro zpracovavanou funkci
 * @param  offset                 Pointer na pointer pro ulozeni offsetu vysledku prave strany
 * @param  numberOfTemporaryItems Pocet docasnych promennych (slouzi k optimalizaci)
 * @return                        ERR_OK pokud je vse v poradku, jinak prislusny chybovy kod
 */
ecode parseSubstring(FILE *file, tFunctionData *functionRecord, int **offset, int *numberOfTemporaryItems)
{
	ecode error;
	Token currentToken;
	tRange *range;
	int *stringOffset, *rangeOffset, *beginOffset, *endOffset, *resultOffset;
	tTableItem *varRecord;
	String *resultName;

	// -------------- Token reprezentujici term ----------------------------------
	currentToken = getToken(file);
	if (!isTerm(currentToken.type))
	{   // -------------- Pro token neni pravidlo --------------------------------
		deallocToken(currentToken);
		return determineError(currentToken.type);
	}


	// -------------- Token neni identifikator ani retezec -----------------------
	if (!isString(currentToken.type) && !isIdentifier(currentToken.type))
	{
		deallocToken(currentToken);
		return ERR_SEMANTICS_OTHER;
	}

	// -------------- Token identifikator ------------------------------------
	if (isIdentifier(currentToken.type))
	{
		// Jedna se o identifikator funkce
		if (searchItem(functionTable, currentToken.item) != NULL)
		{
			deallocToken(currentToken);
			return ERR_SEMANTICS_OTHER;
		}

		// -------------- Identifikator neni funkce -> hledame v TS --------------
		varRecord = searchItem(functionRecord->varTabHead, currentToken.item);
		deallocToken(currentToken);
		if (varRecord != NULL)
		{
			stringOffset = varRecord->data;
		}
		else    // Identifikator nenalezen
		{
			return ERR_SEMANTICS_UNDEFINED_VARIABLE;
		}
	}
	// -------------- Token retezec ------------------------------------------
	else if (isString(currentToken.type))
	{
		error = generateAndInsertTemporaryVariable(instructionList,
					functionRecord->varTabHead, tokenTypeToSemanticType(currentToken.type),
					currentToken.item, &stringOffset);
		if (error)
			return error;
	}

	// -------------- Token leva hranata zavorka ---------------------------------
	currentToken = getToken(file);
	if (!isLeftSquareBracket(currentToken.type))
	{   // -------------- Pro token neni pravidlo --------------------------------
		deallocToken(currentToken);
		return determineError(currentToken.type);
	}

	// -------------- Dolni mez podretezce -----------------------------------

	currentToken = getToken(file);

	// -------------- Token identifikator ------------------------------------
	if (isIdentifier(currentToken.type))
	{
		// Jedna se o identifikator funkce
		if (searchItem(functionTable, currentToken.item) != NULL)
		{
			deallocToken(currentToken);
			return ERR_SEMANTICS_OTHER;
		}

		// -------------- Identifikator neni funkce -> hledame v TS --------------
		varRecord = searchItem(functionRecord->varTabHead, currentToken.item);

		deallocToken(currentToken);
		if (varRecord != NULL)
		{
			beginOffset = varRecord->data;
		}
		else    // Identifikator nenalezen
		{
			return ERR_SEMANTICS_UNDEFINED_VARIABLE;
		}
	}
	// -------------- Token reprezentujici ciselny literal -----------------
	else if (isLiteral(currentToken.type) && isNumeric(currentToken.type))
	{
		error = generateAndInsertTemporaryVariable(instructionList,
					functionRecord->varTabHead, tokenTypeToSemanticType(currentToken.type),
					currentToken.item, &beginOffset);
		if (error)
			return error;
	}
	// -------------- Token reprezentujici neciselnys literal ---------------
	else if (isLiteral(currentToken.type) && !isNumeric(currentToken.type))
	{
		deallocToken(currentToken);
		return ERR_SEMANTICS_OTHER;
	}
	// -------------- Token dvojtecka -> hranice vynechana -------------------
	else if (isColon(currentToken.type))
	{
		beginOffset = NULL;
		returnToken(currentToken);
	}
	else
	{   // -------------- Pro token neni pravidlo --------------------------------
		deallocToken(currentToken);
		return determineError(currentToken.type);
	}

	// -------------- Token dvojtecka ----------------------------------------
	currentToken = getToken(file);
	if (!isColon(currentToken.type))
	{   // -------------- Pro token neni pravidlo --------------------------------
		deallocToken(currentToken);
		return determineError(currentToken.type);
	}

	// -------------- Horni mez podretezce -----------------------------------

	currentToken = getToken(file);

	// -------------- Token identifikator ------------------------------------
	if (isIdentifier(currentToken.type))
	{
		// Jedna se o identifikator funkce
		if (searchItem(functionTable, currentToken.item) != NULL)
		{
			deallocToken(currentToken);
			return ERR_SEMANTICS_OTHER;
		}

		// -------------- Identifikator neni funkce -> hledame v TS --------------
		varRecord = searchItem(functionRecord->varTabHead, currentToken.item);

		deallocToken(currentToken);
		if (varRecord != NULL)
		{
			endOffset = varRecord->data;
		}
		else    // Identifikator nenalezen
		{
			return ERR_SEMANTICS_UNDEFINED_VARIABLE;
		}
	}
	// -------------- Token reprezentujici ciselny literal --------------------
	else if (isLiteral(currentToken.type) && isNumeric(currentToken.type))
	{
		error = generateAndInsertTemporaryVariable(instructionList,
					functionRecord->varTabHead, tokenTypeToSemanticType(currentToken.type),
					currentToken.item, &endOffset);
		if (error)
			return error;
	}
	// -------------- Token reprezentujici neciselny literal -----------------
	else if (isLiteral(currentToken.type) && !isNumeric(currentToken.type))
	{
		deallocToken(currentToken);
		return ERR_SEMANTICS_OTHER;
	}
	// -------------- Token prava hranata zavorka -> mez vynechana -----------
	else if (isRightSquareBracket(currentToken.type))
	{
		endOffset = NULL;
		returnToken(currentToken);
	}
	else
	{   // -------------- Pro token neni pravidlo --------------------------------
		deallocToken(currentToken);
		return determineError(currentToken.type);
	}

	// -------------- Token prava hranata zavorka --------------------------------
	currentToken = getToken(file);
	if (!isRightSquareBracket(currentToken.type))
	{   // -------------- Pro token neni pravidlo --------------------------------
		deallocToken(currentToken);
		return determineError(currentToken.type);
	}

	range = malloc(sizeof(tRange));
	if (range == NULL)
		return ERR_MEMORY;

	range->off1 = beginOffset;
	range->off2 = endOffset;

	error = generateAndInsertTemporaryVariable(instructionList, functionRecord->varTabHead, RANGE, range, &rangeOffset);
	if (error)
			return error;

	// Vytvoreni zaznamu pro pomocnou promennou s vysledkem
	resultName = generateVariableName();
	if (resultName == NULL)
		return ERR_MEMORY;
	// Vypocet offsetu vysledku
	resultOffset = malloc(sizeof(int));
	if (resultOffset == NULL)
	{
		deallocString(resultName);
		return ERR_MEMORY;
	}
	// Pridani promenne vysledku do TS
	*resultOffset = functionRecord->varTabHead->itemCount + 1;

	error = insertItem(functionRecord->varTabHead, resultName, ITEM_VAR, resultOffset);
	if (error)
	{
		deallocString(resultName);
		free(resultOffset);
		return error;
	}

	error = generateAndInsertInstruction(instructionList, INSTR_SUBSTRING, resultOffset, stringOffset, rangeOffset);
	if (error)
		return error;

	*offset = resultOffset;

	return ERR_OK;
}


/**
 * Syntakticka analyza parametru pri prvnim pruchodu programem. Pocita parametry
 * a vrati je analyzatoru funkce.
 * @param  file        Zdrojovy soubor
 * @param  paramsCount Ukazatel na pocet parametru
 * @return             ERR_OK pokud je vse v poradku, jinak prislusny chybovy kod
 */
ecode parseParamsFirstPass(FILE *file, int *paramsCount)
{
	Token currentToken;
	// -------------- Token prava zavorka nebo identifikator --------------------
	currentToken = getToken(file);
	if (!isIdentifier(currentToken.type) && !isRightBracket(currentToken.type))
	{
		deallocToken(currentToken);
		return determineError(currentToken.type);
	}
	else if (isIdentifier(currentToken.type))
	{
		deallocToken(currentToken);
		(*paramsCount)++;   // Zvysi se pocitadlo parametru
	}
	else if (isRightBracket(currentToken.type))
	{
		// -------------- Token prava zavorka -----------------------------------
		// -------------- Vrati token scanneru, protoze nepatri do pravidla -----
		returnToken(currentToken);
		return ERR_OK;
	}

	// -------------- Analyza dalsich parametru ----------------------------------

	currentToken = getToken(file);
	while (!isRightBracket(currentToken.type))
	{
		// -------------- Token carka --------------------------------------------
		if (!isComma(currentToken.type))
		{   // -------------- Pro token neni pravidlo ----------------------------
			deallocToken(currentToken);
			return determineError(currentToken.type);
		}

		// -------------- Token identifikator ------------------------------------
		currentToken = getToken(file);
		if (!isIdentifier(currentToken.type))
		{   // -------------- Pro token neni pravidlo ----------------------------
			deallocToken(currentToken);
			return determineError(currentToken.type);
		}
		// Token obsahuje identifikator
		(*paramsCount)++;
		deallocToken(currentToken);
		// Nacti dalsi token
		currentToken = getToken(file);

	}

	// -------------- Token je prava zavorka - konec parametru -------------------
	returnToken(currentToken);
	return ERR_OK;
}

/**
 * Funkce pro vlozeni parametru do TS. Vypocita offset pro parametr a vlozi ho
 * do TS.
 * @param  symbolTable Tabulka symbolu
 * @param  name        Jmeno parametru
 * @return             ERR_OK pokud je vse v poradku, jinak prislusny chybovy kod
 */
ecode insertParamToSymbolTable(tTableHead* symbolTable, String* name)
{
	int *offset;
	ecode error;
	// Token obsahuje identifikator
	// Pridat parametr do tabulky symbolu
	// Podle poctu parametru vypocitat offset
	// Nebude se generovat zadna instrukce, protoze offset ukazuje na jiz existujici zaznam

	// -------------- Kontrola, zda je identifikator v tabulce funkci --------
	if (searchItem(functionTable, name) != NULL)
		return ERR_SEMANTICS_OTHER;

	// -------------- Kontrola zda nebyl identifikator definovan -------------
	if (searchItem(symbolTable, name) != NULL)
		return ERR_SEMANTICS_OTHER;

	// Jedna se o novou promennou, je treba vypocitat offset
	// -------------- Vypocet offsetu ----------------------------------------
	offset = malloc(sizeof(int));
	if (offset == NULL)
		return ERR_MEMORY;

	// Vypocet offsetu parametru funkce = pocet polozek - zaznam volani funkce
	*offset = symbolTable->itemCount - STACK_CALL_SPACE + 1;
	error = insertItem(symbolTable, name, ITEM_VAR, offset);
	if (error)
	{
		free(offset);
		return error;
	}
	return ERR_OK;
}


/**
 * Vytvori zaznam pro hlavni pseudofunkci -> priradi ji navesti jako prvni
 * instrukci, inicializuje jeji tabulku symbolu
 * @return ERR_OK pokud je vse v poradku, jinak prislusny chybovy kod
 */
ecode generateMainFunctionRecord()
{
	ecode error;
	tFunctionData *functionRecord;  // Zaznam funkce
	String *name;                   // Jmeno noveho zaznamu
	tInstruction *instruction;      // Instrukce

	// -------------- Nastaveni jmena funkce hlavniho programu -------------------
	name = charToString(MAIN_FUNCTION_NAME);
	if (name == NULL)
	{
		deallocString(name);
		return ERR_MEMORY;
	}

	// -------------- Vytvoreni zaznamu hlavni funkce v tabulce funkci -----------
	functionRecord = malloc(sizeof(tFunctionData));
	// -------------- Inicializace zaznamu funkce --------------------------------
	functionRecord->firstInstruction = NULL;
	functionRecord->paramsCount = -1;
	functionRecord->varTabHead = initTable();
	if (functionRecord->varTabHead == NULL)
	{
		free(functionRecord);
		deallocString(name);
		return ERR_MEMORY;
	}
	functionRecord->varTabHead->itemCount = -1;
	// -------------- Vlozeni do tabulky funkci-----------------------------------
	error = insertItem(functionTable, name, ITEM_FUNCTION, functionRecord);
	if (error)
	{
		freeTree(functionRecord->varTabHead);
		free(functionRecord);
		deallocString(name);
		return error;
	}

	// -------------- Instrukce navesti hlavniho tela programu -------------------
	instruction = generateInstruction(INSTR_LABEL, NULL, NULL, NULL);
	if (instruction == NULL)
	{
		return ERR_MEMORY;
	}

	// -------------- Vlozeni instrukce do seznamu instrukci ---------------------
	error = tIListInsertLast(instructionList, instruction);
	if (error)
	{
		return error;
	}

	// -------------- Vlozi ukazatel na navesti k zaznamu funkce -----------------
	functionRecord->firstInstruction = tIListGetLast(instructionList);

	return ERR_OK;
}

/**
 * Funkce pro vygenerovani docasne promenne, necha si vygenerovat unikatni
 * retezec se jmenem promenne, vypocita jeji offset, vlozi ji do tabulky
 * symbolu a vrati vypocitany offset pro dalsi pouziti
 * Vygeneruje instrukci pro zkopirovani literalu na vypocitany offset na
 * zasobniku
 * @param  instrList Seznam instrukci
 * @param  symTable  Tabulka symbolu
 * @param  type      Datovy typ promenne
 * @param  data      Hodnota promenne
 * @param  offset    Ukazatel pro ulozeni offsetu
 * @return           ERR_OK pokud je vse v poradku, jinak prislusny chybovy kod
 */
ecode generateAndInsertTemporaryVariable(tIList *instrList, tTableHead *symTable, SemanticType type, void *data, int **offset)
{
	ecode error;
	String *tmpVarName;
	int *tmpOffset;
	tVariable *tmpVar;

	// -------------- Vytvoreni docasne promenne pro navratovou hodnotu ----------
	tmpVarName = generateVariableName();

	tmpOffset = malloc(sizeof(int));
	if (tmpOffset == NULL)
		return ERR_MEMORY;

	*tmpOffset = symTable->itemCount + 1;

	// -------------- Vlozeni docasne promenne do TS -----------------------------
	error = insertItem(symTable, tmpVarName, ITEM_VAR, tmpOffset);
	if (error)
	{
		free(tmpOffset);
		return error;
	}

	// -------------- Vytvoreni promenne s danym typem a hodnotou ----------------
	error = createNewVariable(&tmpVar, type, data);
	if (error)
		return error;

	// -------------- Vlozeni promenne na zasobnik -------------------------------
	error = generateAndInsertInstruction(instructionList, INSTR_MOV, tmpOffset, tmpVar, NULL);
	if (error)
		return error;

	*offset = tmpOffset;

	return ERR_OK;
}


/**
 * Funkce pro vygenerovani instrukce a jeji vlozeni do seznamu instrukci
 * @param  list   Seznam instruci
 * @param  type   Typ instrukce
 * @param  target Prvni operand instrukce
 * @param  op1    Druhy operanc instrukce
 * @param  op2    Treti operand instrukce
 * @return        ERR_OK pokud je vse v poradku, jinak prislusny chybovy kod
 */
ecode generateAndInsertInstruction(tIList *list, InstructionType type, void *target, void *op1, void *op2)
{
	ecode error;
	tInstruction *instruction;
	// -------------- Generovani intrukce pro skok za definici funkce ------------
	instruction = generateInstruction(type, target, op1, op2);
	if (instruction == NULL)
		return ERR_MEMORY;

	// -------------- Instrukce se vlozi do seznamu instrukci --------------------
	error = tIListInsertLast(list, instruction);
	if (error)
	{
		free(instruction);
		return error;
	}

	return ERR_OK;
}


/**
 * Funkce pro generovani jmena pomocne promenne prekladace ve tvaru:
 * $T<cislo>
 * Cislo je ulozeno v globalni promenne internalVariableCount
 * @return Retezec se jmenem nebo NULL pri chybe
 */
String *generateVariableName()
{
	String *newName;
	String *suffix;

	newName = charToString("$T");
	if (newName == NULL)
		return NULL;

	suffix = numberToString(internalVariableCount);
	if (suffix == NULL)
		return NULL;

	newName = stringConcatenate(newName, suffix);
	if (newName == NULL)
		return NULL;

	deallocString(suffix);
	internalVariableCount++;
	return newName;
}

/**
 * Funkce pro generovani unikatniho jmena navesti ve tvaru:
 * $L<cislo>
 * @return Retezec se jmenem nebo NULL pri chybe
 */
String *generateLabelName()
{
	String *newName;
	String *suffix;
	static int labelCount = 0;
	newName = charToString("$L");
	if (newName == NULL)
		return NULL;

	suffix = numberToString(labelCount);
	if (suffix == NULL)
		return NULL;

	newName = stringConcatenate(newName, suffix);
	if (newName == NULL)
		return NULL;

	deallocString(suffix);
	labelCount++;
	return newName;
}


/**
 * Funkce, ktera z typu tokenu urci semanticky typ promenne a ten vrati
 * @param  type Typ tokenu
 * @return      Semanticky typ promenne
 */
SemanticType tokenTypeToSemanticType(TokenType type)
{
	switch (type)
	{
		case TT_STRING:
			return STRING;
		break;
		case TT_NUMBER:
			return NUMERIC;
		break;
		case TT_LOGIC:
			return LOGICAL;
		break;
		case TT_NIL:
			return NIL;
		break;
		default:
			return UNDEFINED;
		break;
	}
	return UNDEFINED;
}
