/*+
________________________________________________________________________

 CopyRight:     (C) de Groot-Bril Earth Sciences B.V.
 Author:        Bert Bril
 Date:          25/05/2000
 RCS:           $Id: uiioobjsel.cc,v 1.1 2001-05-03 10:16:56 bert Exp $
________________________________________________________________________

-*/

#include "uiioobjsel.h"
#include "iodirentry.h"

uiIOObjSelect::uiIOObjSelect( uiObject* p )
	: uiIOFileSelect(p,"Select an object",true)
{
}


uiIOObjSelect::~uiIOObjSelect()
{
}
