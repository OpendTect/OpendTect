/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        K. Tingdahl
 Date:          May 2002
 RCS:           $Id: embodytr.cc,v 1.2 2008-09-08 17:41:28 cvskris Exp $
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
    }
    ctxt->trgroup = &theInst();
    return *ctxt;
}
