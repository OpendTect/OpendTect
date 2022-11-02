/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "embodytr.h"

#include "embody.h"
#include "emobject.h"
#include "emmanager.h"
#include "emmarchingcubessurface.h"
#include "empolygonbody.h"
#include "emrandomposbody.h"
#include "ioman.h"
#include "uistrings.h"

uiString EMBodyTranslatorGroup::sTypeName( int num )
{ return uiStrings::sGeobody( num ); }


int EMBodyTranslatorGroup::selector( const char* s )
{
    int res = defaultSelector( EMBodyTranslatorGroup::sGroupName(), s );
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
	newctxt->stdseltype_ = IOObjContext::Surf;

	ctxt.setIfNull(newctxt,true);
    }

    ctxt->trgroup_ = &theInst();
    return *ctxt;
}


odEMBodyTranslator::odEMBodyTranslator( const char* nm, const char* unm )
    : EMBodyTranslator(nm,unm)
    , readbody_(0)
{}


odEMBodyTranslator::~odEMBodyTranslator()
{}


Executor* odEMBodyTranslator::reader( const IOObj& ioobj )
{
    const IOPar& iopar = ioobj.pars();
    BufferString objtype;
    iopar.get( sKey::Type(), objtype );

    EM::EMObject* emobj = EM::EMM().createTempObject(objtype);
    mDynamicCastGet(EM::Body*,bdy,emobj);
    readbody_ = bdy;
    return emobj ? emobj->loader() : 0;
}


Executor* odEMBodyTranslator::writer( const EM::Body& body, IOObj& ioobj )
{
    IOPar& iopar = ioobj.pars();
    iopar.set( sKey::Type(), body.type() );
    ioobj.updateCreationPars();
    IOM().commitChanges( ioobj );

    mDynamicCastGet(EM::PolygonBody*,plgbdy,const_cast<EM::Body*>(&body));
    if ( plgbdy ) return plgbdy->saver(&ioobj);

    mDynamicCastGet(EM::MarchingCubesSurface*,mcbdy,
	    const_cast<EM::Body*>(&body));
    if ( mcbdy ) return mcbdy->saver(&ioobj);;

    mDynamicCastGet(EM::RandomPosBody*,rdpbdy,const_cast<EM::Body*>(&body));
    return rdpbdy ? rdpbdy->saver(&ioobj) : 0;
}
