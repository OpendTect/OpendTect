
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
    , name_( 0 )
{
    mDefineStaticLocalObject( Threads::Atomic<int>, vw2dobjid, (0) );
    id_ = vw2dobjid++;
}


Vw2DDataObject::~Vw2DDataObject()
{ delete name_; }


const char* Vw2DDataObject::name() const
{
    return !name_ || name_->isEmpty() ? 0 : name_->buf();
}


void Vw2DDataObject::setName( const char* nm )
{
    if ( !name_ ) name_ = new BufferString;
    (*name_) = nm;
}


bool Vw2DDataObject::fillPar( IOPar& par ) const
{
    par.set( sKey::Type(), getClassName() );
    const char* nm = name();
    if ( nm )
	par.set( sKey::Name(), nm );
    par.set( sKeyMID(), toString(-1) );
    return true;
}


bool Vw2DDataObject::usePar( const IOPar& par )
{
    const char* nm = par.find( sKey::Name() );
    if ( nm )
	setName( nm );
    return true;
}


Vw2DEMDataObject::Vw2DEMDataObject( const DBKey& oid,uiFlatViewWin* win,
			     const ObjectSet<uiFlatViewAuxDataEditor>& eds)
    : Vw2DDataObject()
    , viewerwin_(win)
    , emid_(oid)
    , auxdataeditors_(eds)
{}


bool Vw2DEMDataObject::fillPar( IOPar& par ) const
{
    Vw2DDataObject::fillPar( par );
    par.set( sKeyMID(), emid_ );
    return true;
}


bool Vw2DEMDataObject::usePar( const IOPar& par )
{
    if ( !Vw2DDataObject::usePar( par ) )
	return false;
    DBKey dbky;
    if ( !par.get(sKeyMID(),dbky) )
	return false;

    EM::Object* emobj = EM::MGR().loadIfNotFullyLoaded( dbky,
					SilentTaskRunnerProvider() );
    if ( !emobj )
	return false;

    emid_ = emobj->id();
    setEditors();
    return true;
}
