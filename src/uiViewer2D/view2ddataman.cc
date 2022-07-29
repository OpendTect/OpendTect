/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Umesh Sinha
 Date:		Apr 2010
________________________________________________________________________

-*/

#include "view2ddataman.h"

#include "iopar.h"
#include "keystrs.h"
#include "view2ddata.h"
#include "view2dfaultss2d.h"
#include "view2dfaultss3d.h"
#include "view2dfault.h"
#include "view2dhorizon2d.h"
#include "view2dhorizon3d.h"
#include "view2dpickset.h"
#include "view2dseismic.h"

#include "uiflatviewwin.h"
#include "emposid.h"


mImplFactory2Param(Vw2DDataObject,uiFlatViewWin*,
	const ObjectSet<uiFlatViewAuxDataEditor>&,Vw2DDataManager::factory);

Vw2DDataManager::Vw2DDataManager()
    : selectedid_( -1 )
    , freeid_( 0 )
    , addRemove( this )
    , dataObjAdded( this )
    , dataObjToBeRemoved( this )
{}


Vw2DDataManager::~Vw2DDataManager()
{
    removeAll();
}


void Vw2DDataManager::addObject( Vw2DDataObject* obj )
{
    if ( objects_.isPresent(obj) ) return;

    objects_ += obj;
    freeid_++;
    obj->ref();

    if ( selectedid_ != -1 )
	deSelect( selectedid_ );

    selectedid_ = obj->id();
    dataObjAdded.trigger( obj->id() );
    addRemove.trigger();
}


void Vw2DDataManager::removeObject( Vw2DDataObject* dobj )
{
    if ( !objects_.isPresent(dobj) ) return;

    dataObjToBeRemoved.trigger( dobj->id() );
    objects_ -= dobj;

    if ( dobj->id() == selectedid_ )
	selectedid_ = -1;

    dobj->unRef();
    addRemove.trigger();
}


void Vw2DDataManager::removeAll()
{
    for ( int idx=0; idx<objects_.size(); idx++ )
	dataObjToBeRemoved.trigger( objects_[idx]->id() );

    deepUnRef( objects_ );
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


void Vw2DDataManager::getObjectIDs( TypeSet<int>& objids ) const
{
    ObjectSet<Vw2DDataObject> vw2dobjs;
    getObjects( vw2dobjs );
    objids.setSize( vw2dobjs.size(), -1 );
    for ( int idx=0; idx<vw2dobjs.size(); idx++ )
	objids[idx] = vw2dobjs[idx]->id();
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


void Vw2DDataManager::usePar( const IOPar& iop, uiFlatViewWin* vwwin,
			    const ObjectSet<uiFlatViewAuxDataEditor>& eds )
{
    int nrobjects;
    if ( !iop.get( sKeyNrObjects(), nrobjects ))
	return;

    for ( int idx=0; idx<nrobjects; idx++ )
    {
	PtrMan<IOPar> objpar = iop.subselect( toString(idx) );
	if ( !objpar || !objpar->size() )
	{
	    if ( !idx ) continue;
	    break;
	}
	const char* type = objpar->find( sKey::Type() );
	RefMan<Vw2DDataObject> obj = factory().create( type, vwwin, eds );
	if ( obj && obj->usePar(*objpar) && !similarObjectPresent(obj) )
	    addObject( obj );
    }
}


bool Vw2DDataManager::similarObjectPresent( const Vw2DDataObject* dobj ) const
{
    mDynamicCastGet(const VW2DSeis*,seisobj,dobj)
    mDynamicCastGet(const Vw2DHorizon3D*,hor3dobj,dobj)
    mDynamicCastGet(const Vw2DHorizon2D*,hor2dobj,dobj)
    mDynamicCastGet(const VW2DFaultSS2D*,fss2dobj,dobj)
    mDynamicCastGet(const VW2DFaultSS3D*,fss3dobj,dobj)
    mDynamicCastGet(const VW2DFault*,fltobj,dobj)
    mDynamicCastGet(const VW2DPickSet*,pickobj,dobj)

    for ( int idx=0; idx<objects_.size(); idx++ )
    {
	const Vw2DDataObject* vw2dobj = objects_[idx];
	if ( seisobj )
	{
	    mDynamicCastGet(const VW2DSeis*,vw2seisobj,vw2dobj)
	    if ( vw2seisobj )
		return true;
	}
	else if ( hor3dobj )
	{
	    mDynamicCastGet(const Vw2DHorizon3D*,vw2dhor3dobj,vw2dobj)
	    if ( vw2dhor3dobj && (hor3dobj==vw2dhor3dobj ||
		    hor3dobj->getEMObjectID()==vw2dhor3dobj->getEMObjectID()) )
		return true;
	}
	else if ( hor2dobj )
	{
	    mDynamicCastGet(const Vw2DHorizon2D*,vw2dhor2dobj,vw2dobj)
	    if ( vw2dhor2dobj && (hor2dobj==vw2dhor2dobj ||
		    hor2dobj->getEMObjectID()==vw2dhor2dobj->getEMObjectID()) )
		return true;
	}
	else if ( fss2dobj )
	{
	    mDynamicCastGet(const VW2DFaultSS2D*,vw2dfss2dobj,vw2dobj)
	    if ( vw2dfss2dobj && (fss2dobj==vw2dfss2dobj ||
		    fss2dobj->getEMObjectID()==vw2dfss2dobj->getEMObjectID()) )
		return true;
	}
	else if ( fss3dobj )
	{
	    mDynamicCastGet(const VW2DFaultSS3D*,vw2dfss3dobj,vw2dobj)
	    if ( vw2dfss3dobj && (fss3dobj==vw2dfss3dobj ||
		    fss3dobj->getEMObjectID()==vw2dfss3dobj->getEMObjectID()) )
		return true;
	}
	else if ( fltobj )
	{
	    mDynamicCastGet(const VW2DFault*,vw2dfltobj,vw2dobj)
	    if ( vw2dfltobj && (fltobj==vw2dfltobj ||
		    fltobj->getEMObjectID()==vw2dfltobj->getEMObjectID()) )
		return true;
	}
	else if ( pickobj )
	{
	    mDynamicCastGet(const VW2DPickSet*,vw2dpickobj,vw2dobj)
	    if ( vw2dpickobj &&
		 (pickobj==vw2dpickobj ||
		  pickobj->pickSetID()==vw2dpickobj->pickSetID()) )
		return true;
	}
    }

    return false;
}
