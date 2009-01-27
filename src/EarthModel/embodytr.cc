/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        K. Tingdahl
 Date:          May 2002
________________________________________________________________________

-*/
static const char* rcsID = "$Id: embodytr.cc,v 1.4 2009-01-27 21:40:25 cvsyuancheng Exp $";

#include "embodytr.h"
#include "embody.h"

mDefSimpleTranslatorSelector(EMBody,EMBodyTranslatorGroup::sKeyword() )

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
