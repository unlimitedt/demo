// parser_dropdown.h

/******************************************************************************
 * Implementace interpretu imperativniho jazyka IFJ12                         *
 * Autori:                                                                    *
 *          Tomas Nestrojil (xnestr03)                                        *
 *                                                                            *
 * Dropdown parser for processing source code file using syntax analyzer      *
 ******************************************************************************
 */

#ifndef PARSER_DROPDOWN_H
#define PARSER_DROPDOWN_H

#include <stdio.h>
#include "errnum.h"
/**
 * Provede syntaktickou analyzu zdrojoveho souboru metodou rekurzivniho sestupu,
 * pro syntaktickou analyzu vyrazu vola prislusnou funkci precedencni syntakticke
 * analyzy. Zaroven pomoci prislusnych semantickych akci generuje tri adresny kod,
 * ktery bude dale interpretovan
 * @param  f Zdrojovy soubor
 * @return	Vraci 	ERR_OK 		pokud je vse v poradku
 *                	prislusny chybovy kod v ostatnich pripadech
 */
ecode parseFile(FILE *f);

#endif // PARSER_H
