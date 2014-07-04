/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        K. Tingdahl
 Date:          May 2002
________________________________________________________________________

-*/
static const char* rcsID mUsedVar = "$Id$";

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
    mDefineStaticLocalObject( PtrMan<IOObjContext>, ctxt, = 0 );
    if ( !ctxt )
    {
	IOObjContext* newctxt = new IOObjContext( 0 );
	newctxt->stdseltype = IOObjContext::Surf;

	if ( !ctxt.setIfNull(newctxt) )
	    delete newctxt;
    }

    ctxt->trgroup = &theInst();
    return *ctxt;
}
