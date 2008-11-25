/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        K. Tingdahl
 Date:          May 2002
________________________________________________________________________

-*/
static const char* rcsID = "$Id: embodytr.cc,v 1.3 2008-11-25 15:35:22 cvsbert Exp $";

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
