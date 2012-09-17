
/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Umesh Sinha
 Date:		Apr 2010
 RCS:		$Id: visvw2ddata.cc,v 1.4 2011/06/03 15:29:36 cvsbruno Exp $
________________________________________________________________________

-*/

#include "visvw2ddata.h"

#include "executor.h"
#include "keystrs.h"
#include "emmanager.h"
#include "emsurfaceiodata.h"
#include "iopar.h"

Vw2DDataObject::Vw2DDataObject()
    : id_( -1 )
    , name_( 0 )
{}


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
    par.set( sKey::Type, getClassName() );
    const char* nm = name();
    if ( nm )
	par.set( sKey::Name, nm );
    par.set( sKeyMID(), toString(-1) );
    return true;
}


bool Vw2DDataObject::usePar( const IOPar& par ) 
{
    const char* nm = par.find( sKey::Name );
    if ( nm )
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
    par.get( sKeyMID(), mid );
    EM::SurfaceIOData sd;
    EM::SurfaceIODataSelection sel( sd );
    Executor* exec = EM::EMM().objectLoader( mid, &sel);
    TaskRunner exectr;
    if ( exec && exectr.execute(*exec) )
    {
	emid_ = EM::EMM().getObjectID( mid );
	setEditors();
	return true;
    }
    return false;
}

