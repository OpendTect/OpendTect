/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        K. Tingdahl
 Date:          May 2002
________________________________________________________________________

-*/
static const char* rcsID = "$Id: embodytr.cc,v 1.6 2009-07-22 16:01:31 cvsbert Exp $";

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
