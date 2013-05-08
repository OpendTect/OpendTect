/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Umesh Sinha
 Date:		Apr 2010
 RCS:		$Id$
________________________________________________________________________

-*/

#include "visvw2ddataman.h"

#include "iopar.h"
#include "keystrs.h"
#include "visvw2ddata.h"

#include "uiflatviewwin.h"
#include "emposid.h"


mImplFactory3Param(Vw2DDataObject,const EM::ObjectID&,uiFlatViewWin*,const ObjectSet<uiFlatViewAuxDataEditor>&,Vw2DDataManager::factory);

Vw2DDataManager::Vw2DDataManager()
    : selectedid_( -1 )
    , freeid_( 0 )
    , addRemove( this )
{}


Vw2DDataManager::~Vw2DDataManager()
{
    removeAll();
}


void Vw2DDataManager::addObject( Vw2DDataObject* obj )
{
    if ( objects_.isPresent(obj) ) return;

    objects_ += obj;
    obj->setID(freeid_++);
    obj->ref();
    
    if ( selectedid_ != -1 )
	deSelect( selectedid_ );

    selectedid_ = obj->id();
    addRemove.trigger();
}


void Vw2DDataManager::removeObject( Vw2DDataObject* dobj )
{
    if ( !objects_.isPresent(dobj) ) return;
    
    objects_ -= dobj;
    
    if ( dobj->id() == selectedid_ )
	selectedid_ = -1;

    dobj->unRef();
    addRemove.trigger();
}


void Vw2DDataManager::removeAll()
{
    while ( objects_.size() )
    {
	while ( objects_.size() && objects_[0]->nrRefs() )
	    objects_[0]->unRef();
    }

    selectedid_ = -1;
    freeid_ = 0;

    addRemove.trigger();
}


Vw2DDataObject* Vw2DDataManager::getObject( int id )
{
    for ( int idx=0; idx<objects_.size(); idx++ )
    {
	if ( objects_[idx]->id()==id ) return objects_[idx];
    }

    return 0;
}


void Vw2DDataManager::setSelected( Vw2DDataObject* sobj )
{
    if ( !sobj )
    {
	deSelect( selectedid_ );
	return;
    }

    if ( sobj->id() == selectedid_ )
	return;

    deSelect( selectedid_ );
    for ( int idx=0; idx<objects_.size(); idx++ )
    {
	if ( objects_[idx]->id()==sobj->id() )
	{
	    selectedid_ = sobj->id();
	    break;
	}
    }
}


void Vw2DDataManager::deSelect( int id )
{
    Vw2DDataObject* dataobj = getObject( id );
    if( dataobj )
	dataobj->triggerDeSel();

    if ( selectedid_ == id )
	selectedid_ = -1;
}


void Vw2DDataManager::getObjects( ObjectSet<Vw2DDataObject>& objs ) const
{
    objs.copy( objects_ );
}


const Vw2DDataObject* Vw2DDataManager::getObject( int id ) const
{ return const_cast<Vw2DDataManager*>(this)->getObject(id); }


void Vw2DDataManager::fillPar( IOPar& par ) const
{
    for ( int idx=0; idx<objects_.size(); idx++ )
    {
	IOPar dataobjpar;
	const Vw2DDataObject& dataobj = *objects_[idx];
	dataobj.fillPar( dataobjpar );
	par.mergeComp( dataobjpar, toString( dataobj.id() ) );
    }
    par.set( sKeyNrObjects(), objects_.size() );
}


void Vw2DDataManager::usePar( const IOPar& iop, uiFlatViewWin* win, 
			    const ObjectSet<uiFlatViewAuxDataEditor>& eds )
{
    int curnrobects = objects_.size();
    int nrobjects;
    if ( !iop.get( sKeyNrObjects(), nrobjects ))
	return;

    for ( int idx=curnrobects; idx<nrobjects; idx++ )
    {
	PtrMan<IOPar> objpar = iop.subselect( toString(idx) );
	if ( !objpar || !objpar->size() )
	{
	    if ( !idx ) continue;
	    break;
	}
	const char* type = objpar->find( sKey::Type() );
	RefMan<Vw2DDataObject> obj = factory().create(type, -1 ,win,eds);
	if ( obj && obj->usePar( *objpar ) )
	{
	    addObject( obj );
	}
    }
}

