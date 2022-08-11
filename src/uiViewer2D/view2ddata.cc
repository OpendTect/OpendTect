
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

namespace View2D
{

DataObject::DataObject()
{
    mDefineStaticLocalObject( Threads::Atomic<int>, vw2dobjid, (0) );
    id_.set( vw2dobjid++ );
}


DataObject::~DataObject()
{}


bool DataObject::fillPar( IOPar& par ) const
{
    par.set( sKey::Type(), getClassName() );
    par.set( sKey::Name(), name() );
    par.set( sKeyMID(), toString(-1) );
    return true;
}


bool DataObject::usePar( const IOPar& par )
{
    const StringView nm = par.find( sKey::Name() );
    if ( !nm.isEmpty() )
	setName( nm );

    return true;
}


EMDataObject::EMDataObject( uiFlatViewWin* win,
			     const ObjectSet<uiFlatViewAuxDataEditor>& eds )
    : DataObject()
    , viewerwin_(win)
    , auxdataeditors_(eds)
{}


void EMDataObject::setEMObjectID( const EM::ObjectID& emid )
{
    emid_ = emid;
    if ( emid_.isValid() )
	setEditors();
}


bool EMDataObject::fillPar( IOPar& par ) const
{
    DataObject::fillPar( par );
    par.set( sKeyMID(), EM::EMM().getMultiID( emid_ ) );
    return true;
}


bool EMDataObject::usePar( const IOPar& par )
{
    if ( !DataObject::usePar( par ) )
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

} // namespace View2D
