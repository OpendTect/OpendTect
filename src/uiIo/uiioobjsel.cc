/*+
________________________________________________________________________

 CopyRight:     (C) de Groot-Bril Earth Sciences B.V.
 Author:        Bert Bril
 Date:          25/05/2000
 RCS:           $Id: uiioobjsel.cc,v 1.2 2001-05-05 16:32:52 bert Exp $
________________________________________________________________________

-*/

#include "uiioobjsel.h"
#include "iodirentry.h"

uiIOObjSelect::uiIOObjSelect( uiObject* p, CtxtIOObj& c )
	: uiIOSelect(p,mCB(this,uiIOObjSelect,doObjSel),"Select an object",true)
	, ctio(c)
{
}


uiIOObjSelect::~uiIOObjSelect()
{
}


void uiIOObjSelect::doObjSel( CallBacker* )
{
}
