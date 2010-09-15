/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Umesh Sinha
 Date:		Apr 2010
 RCS:		$Id: visvw2ddataman.cc,v 1.3 2010-09-15 08:30:21 cvsbruno Exp $
________________________________________________________________________

-*/

#include "visvw2ddataman.h"

#include "visvw2ddata.h"


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
    if ( objects_.indexOf(obj)!=-1) return;

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
    if ( objects_.indexOf(dobj) == -1 ) return;
    
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
