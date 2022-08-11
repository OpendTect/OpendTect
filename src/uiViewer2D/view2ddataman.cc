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

namespace View2D
{

mImplFactory2Param(DataObject,uiFlatViewWin*,
	const ObjectSet<uiFlatViewAuxDataEditor>&,DataManager::factory);

DataManager::DataManager()
    : addRemove( this )
    , dataObjAdded( this )
    , dataObjToBeRemoved( this )
    , freeid_( 0 )
{}


DataManager::~DataManager()
{
    removeAll();
}


void DataManager::addObject( DataObject* obj )
{
    if ( objects_.isPresent(obj) ) return;

    objects_ += obj;
    freeid_++;
    obj->ref();

    if ( selectedid_.isValid() )
	deSelect( selectedid_ );

    selectedid_ = obj->id();
    dataObjAdded.trigger( obj->id() );
    addRemove.trigger();
}


void DataManager::removeObject( DataObject* dobj )
{
    if ( !objects_.isPresent(dobj) ) return;

    dataObjToBeRemoved.trigger( dobj->id() );
    objects_ -= dobj;

    if ( dobj->id() == selectedid_ )
	selectedid_.setUdf();

    dobj->unRef();
    addRemove.trigger();
}


void DataManager::removeAll()
{
    for ( int idx=0; idx<objects_.size(); idx++ )
	dataObjToBeRemoved.trigger( objects_[idx]->id() );

    deepUnRef( objects_ );
    selectedid_.setUdf();
    freeid_ = 0;

    addRemove.trigger();
}


DataObject* DataManager::getObject( Vis2DID id )
{
    for ( int idx=0; idx<objects_.size(); idx++ )
    {
	if ( objects_[idx]->id()==id ) return objects_[idx];
    }

    return 0;
}


const DataObject* DataManager::getObject( Vis2DID id ) const
{
    return const_cast<DataManager*>(this)->getObject(id);
}


void DataManager::setSelected( DataObject* sobj )
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


void DataManager::deSelect( Vis2DID id )
{
    DataObject* dataobj = getObject( id );
    if( dataobj )
	dataobj->triggerDeSel();

    if ( selectedid_ == id )
	selectedid_.setUdf();
}


void DataManager::getObjectIDs( TypeSet<Vis2DID>& objids ) const
{
    ObjectSet<DataObject> vw2dobjs;
    getObjects( vw2dobjs );
    objids.setSize( vw2dobjs.size(), Vis2DID::udf() );
    for ( int idx=0; idx<vw2dobjs.size(); idx++ )
	objids[idx] = vw2dobjs[idx]->id();
}


void DataManager::getObjects( ObjectSet<DataObject>& objs ) const
{
    objs.copy( objects_ );
}


void DataManager::fillPar( IOPar& par ) const
{
    for ( int idx=0; idx<objects_.size(); idx++ )
    {
	IOPar dataobjpar;
	const DataObject& dataobj = *objects_[idx];
	dataobj.fillPar( dataobjpar );
	par.mergeComp( dataobjpar, toString(dataobj.id().asInt()) );
    }

    par.set( sKeyNrObjects(), objects_.size() );
}


void DataManager::usePar( const IOPar& iop, uiFlatViewWin* vwwin,
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
	RefMan<DataObject> obj = factory().create( type, vwwin, eds );
	if ( obj && obj->usePar(*objpar) && !similarObjectPresent(obj) )
	    addObject( obj );
    }
}


bool DataManager::similarObjectPresent( const DataObject* dobj ) const
{
    mDynamicCastGet(const Seismic*,seisobj,dobj)
    mDynamicCastGet(const Horizon3D*,hor3dobj,dobj)
    mDynamicCastGet(const Horizon2D*,hor2dobj,dobj)
    mDynamicCastGet(const FaultSS2D*,fss2dobj,dobj)
    mDynamicCastGet(const FaultSS3D*,fss3dobj,dobj)
    mDynamicCastGet(const Fault*,fltobj,dobj)
    mDynamicCastGet(const PickSet*,pickobj,dobj)

    for ( int idx=0; idx<objects_.size(); idx++ )
    {
	const DataObject* vw2dobj = objects_[idx];
	if ( seisobj )
	{
	    mDynamicCastGet(const Seismic*,vw2seisobj,vw2dobj)
	    if ( vw2seisobj )
		return true;
	}
	else if ( hor3dobj )
	{
	    mDynamicCastGet(const Horizon3D*,vw2dhor3dobj,vw2dobj)
	    if ( vw2dhor3dobj && (hor3dobj==vw2dhor3dobj ||
		    hor3dobj->getEMObjectID()==vw2dhor3dobj->getEMObjectID()) )
		return true;
	}
	else if ( hor2dobj )
	{
	    mDynamicCastGet(const Horizon2D*,vw2dhor2dobj,vw2dobj)
	    if ( vw2dhor2dobj && (hor2dobj==vw2dhor2dobj ||
		    hor2dobj->getEMObjectID()==vw2dhor2dobj->getEMObjectID()) )
		return true;
	}
	else if ( fss2dobj )
	{
	    mDynamicCastGet(const FaultSS2D*,vw2dfss2dobj,vw2dobj)
	    if ( vw2dfss2dobj && (fss2dobj==vw2dfss2dobj ||
		    fss2dobj->getEMObjectID()==vw2dfss2dobj->getEMObjectID()) )
		return true;
	}
	else if ( fss3dobj )
	{
	    mDynamicCastGet(const FaultSS3D*,vw2dfss3dobj,vw2dobj)
	    if ( vw2dfss3dobj && (fss3dobj==vw2dfss3dobj ||
		    fss3dobj->getEMObjectID()==vw2dfss3dobj->getEMObjectID()) )
		return true;
	}
	else if ( fltobj )
	{
	    mDynamicCastGet(const Fault*,vw2dfltobj,vw2dobj)
	    if ( vw2dfltobj && (fltobj==vw2dfltobj ||
		    fltobj->getEMObjectID()==vw2dfltobj->getEMObjectID()) )
		return true;
	}
	else if ( pickobj )
	{
	    mDynamicCastGet(const PickSet*,vw2dpickobj,vw2dobj)
	    if ( vw2dpickobj &&
		 (pickobj==vw2dpickobj ||
		  pickobj->pickSetID()==vw2dpickobj->pickSetID()) )
		return true;
	}
    }

    return false;
}

} // namespace View2D
