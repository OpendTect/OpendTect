/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        K. Tingdahl
 Date:          May 2002
________________________________________________________________________

-*/
static const char* rcsID = "$Id: embodytr.cc,v 1.5 2009-05-29 02:44:15 cvskris Exp $";

#include "embodytr.h"
#include "embody.h"

int EMBodyTranslatorGroup::selector( const char* s )
{
    int res = defaultSelector( EMBodyTranslatorGroup::sKeyword(), s );
    if ( res==mObjSelUnrelated )
	res = defaultSelector( "MarchingCubesSurface", s );

    return res;
}



const IOObjContext& EMBodyTranslatorGroup::ioContext()
{
    static PtrMan<IOObjContext> ctxt = 0;
    if ( !ctxt )
    {
	ctxt = new IOObjContext( 0 );
	ctxt->stdseltype = IOObjContext::Surf;
	ctxt->allownonreaddefault = true;
    }

    ctxt->trgroup = &theInst();
    return *ctxt;
}
