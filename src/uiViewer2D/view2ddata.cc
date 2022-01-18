
/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Umesh Sinha
 Date:		Apr 2010
________________________________________________________________________

-*/

#include "view2ddata.h"

#include "executor.h"
#include "keystrs.h"
#include "emmanager.h"
#include "emobject.h"
#include "emsurfaceiodata.h"
#include "iopar.h"

Vw2DDataObject::Vw2DDataObject()
    : id_( -1 )
{
    mDefineStaticLocalObject( Threads::Atomic<int>, vw2dobjid, (0) );
    id_ = vw2dobjid++;
}


Vw2DDataObject::~Vw2DDataObject()
{}


bool Vw2DDataObject::fillPar( IOPar& par ) const
{
    par.set( sKey::Type(), getClassName() );
    par.set( sKey::Name(), name() );
    par.set( sKeyMID(), toString(-1) );
    return true;
}


bool Vw2DDataObject::usePar( const IOPar& par )
{
    const FixedString nm = par.find( sKey::Name() );
    if ( !nm.isEmpty() )
	setName( nm );

    return true;
}


Vw2DEMDataObject::Vw2DEMDataObject( const EM::ObjectID& oid,uiFlatViewWin* win,
			     const ObjectSet<uiFlatViewAuxDataEditor>& eds)
    : Vw2DDataObject()
    , viewerwin_(win)
    , emid_(oid)
    , auxdataeditors_(eds)
{}


bool Vw2DEMDataObject::fillPar( IOPar& par ) const
{
    Vw2DDataObject::fillPar( par );
    par.set( sKeyMID(), EM::EMM().getMultiID( emid_ ) );
    return true;
}


bool Vw2DEMDataObject::usePar( const IOPar& par )
{
    if ( !Vw2DDataObject::usePar( par ) )
	return false;
    MultiID mid;
    if ( !par.get(sKeyMID(),mid) )
	return false;

    TaskRunner exectr;
    EM::EMObject* emobj = EM::EMM().loadIfNotFullyLoaded( mid, &exectr );
    if ( emobj )
    {
	emid_ = emobj->id();
	setEditors();
	return true;
    }

    return false;
}
