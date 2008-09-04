/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        K. Tingdahl
 Date:          May 2002
 RCS:           $Id: embodytr.cc,v 1.1 2008-09-04 13:25:07 cvskris Exp $
________________________________________________________________________

-*/

#include "embodytr.h"
#include "embody.h"

mDefSimpleTranslatorSelector(EMBody,EMBodyTranslatorGroup::sKeyword() )

const char* EMBodyTranslatorGroup::sKeyword()
{ return "Body"; }


const IOObjContext& EMBodyTranslatorGroup::ioContext()
{
    static IOObjContext* ctxt = 0;
    if ( !ctxt )
    {
	ctxt = new IOObjContext( 0 );
	ctxt->stdseltype = IOObjContext::Surf;
	ctxt->trglobexpr = mDGBKey;
    }
    ctxt->trgroup = &theInst();
    return *ctxt;
}
