/*+
 * COPYRIGHT: (C) dGB Beheer B.V.
 * AUTHOR   : K. Tingdahl
 * DATE     : Apr 2002
-*/

static const char* rcsID = "$Id: emmanager.cc,v 1.66 2007-11-14 20:31:41 cvskris Exp $";

#include "emmanager.h"

#include "ctxtioobj.h"
#include "emfault.h"
#include "emhorizon3d.h"
#include "emhorizon2d.h"
#include "emmarchingcubessurface.h"
#include "emhorizonztransform.h"
#include "emobject.h"
#include "emsurfaceiodata.h"
#include "emsurfacetr.h"
#include "errh.h"
#include "executor.h"
#include "iodir.h"
#include "iopar.h"
#include "ioman.h"
#include "ptrman.h"
#include "undo.h"


EM::EMManager& EM::EMM()
{
    static PtrMan<EMManager> emm = 0;

    if ( !emm )
    {
	EM::EMManager::initClasses();
	emm = new EM::EMManager;
    }

    return *emm;
}


namespace EM
{


mImplFactory1Param( EMObject, EMManager&, EMOF );

EMManager::EMManager()
    : undo_( *new Undo() )
    , addRemove( this )
    , syncGeomReq( this )
{}


EMManager::~EMManager()
{
    empty();
    delete &undo_;
}


void EMManager::empty()
{   
    for ( int idx=0; idx<objects_.size(); idx++ )
    {
	EMObjectCallbackData cbdata;
	cbdata.event = EMObjectCallbackData::Removal;

	const int oldsize = objects_.size();
	objects_[idx]->change.trigger(cbdata);
	if ( oldsize!=objects_.size() ) idx--;
    }

    deepRef( objects_ );		//Removes all non-reffed 
    deepUnRef( objects_ );

    if ( objects_.size() )
	pErrMsg( "All objects are not unreffed" );

    addRemove.trigger();

    undo_.removeAll();
}


const Undo& EMManager::undo() const	{ return undo_; }
Undo& EMManager::undo()			{ return undo_; }


BufferString EMManager::objectName( const MultiID& mid ) const
{
    if ( getObject(getObjectID(mid)) )
	return getObject(getObjectID(mid))->name();

    PtrMan<IOObj> ioobj = IOM().get( mid );
    BufferString res;
    if ( ioobj ) res = ioobj->name();
    return res;
}


const char* EMManager::objectType( const MultiID& mid ) const
{
    if ( getObject(getObjectID(mid)) )
	return getObject(getObjectID(mid))->getTypeStr();

    PtrMan<IOObj> ioobj = IOM().get( mid );
    if ( !ioobj ) 
	return 0;

    const int idx = EMOF().getNames().indexOf( ioobj->group() );
    if ( idx<0 )
	return 0;

    return EMOF().getNames()[idx]->buf();
}


void EMManager::initClasses()
{
    Horizon3D::initClass();
    Fault::initClass();
    EM::MarchingCubesSurface::initClass();
    Horizon2D::initClass();
    HorizonZTransform::initClass();
} 


ObjectID EMManager::createObject( const char* type, const char* name )
{
    EMObject* object = EMOF().create( type, *this );
    if ( !object ) return -1;

    CtxtIOObj ctio( object->getIOObjContext() );
    ctio.ctxt.forread = false;
    ctio.ioobj = 0;
    ctio.setName( name );
    if ( ctio.fillObj() ) 
    	object->setMultiID( ctio.ioobj->key() );
    
    object->setFullyLoaded( true );
    return object->id();
}

/*
MultiID EMManager::findObject( const char* type, const char* name ) const
{
    const IOObjContext* context = getContext(type);
    if ( IOM().to(context->getSelKey()) )
    {
	PtrMan<IOObj> ioobj = IOM().getLocal( name );
	IOM().back();
	if ( ioobj && !strcmp(ioobj->group(),type) )
	    return ioobj->key();
    }

    return -1;
}
*/


EMObject* EMManager::getObject( const ObjectID& id )
{
    for ( int idx=0; idx<objects_.size(); idx++ )
    {
	if ( objects_[idx]->id()==id )
	    return objects_[idx];
    }

    return 0;
}


const EMObject* EMManager::getObject( const ObjectID& id ) const
{ return const_cast<EMManager*>(this)->getObject(id); }


ObjectID EMManager::getObjectID( const MultiID& mid ) const
{
    ObjectID res = -1;
    for ( int idx=0; idx<objects_.size(); idx++ )
    {
	if ( objects_[idx]->multiID()==mid )
	{
	    if ( objects_[idx]->isFullyLoaded() )
		return objects_[idx]->id();

	    if ( res==-1 )
		res = objects_[idx]->id(); //Better to return this than nothing
	}
    }

    return res;
}


MultiID EMManager::getMultiID( const ObjectID& oid ) const
{
    const EMObject* emobj = getObject(oid);
    return emobj ? emobj->multiID() : MultiID(-1);
}


void EMManager::addObject( EMObject* obj )
{
    if ( !obj )
    { pErrMsg("No object provided!");
	return;
    }

    if ( objects_.indexOf( obj )!=-1 )
    {
	pErrMsg("Adding object twice");
	return;
    }

    objects_ += obj;
    addRemove.trigger();
}


void EMManager::removeObject( const EMObject* obj )
{
    const int idx = objects_.indexOf( obj );
    if ( idx<0 ) return;
    objects_.remove( idx );
    addRemove.trigger();
}


EMObject* EMManager::createTempObject( const char* type )
{
    return EMOF().create( type, *this );
}


ObjectID EMManager::objectID( int idx ) const
{ return idx>=0 && idx<objects_.size() ? objects_[idx]->id() : -1; }


Executor* EMManager::objectLoader( const TypeSet<MultiID>& mids,
				   const SurfaceIODataSelection* iosel )
{
    ExecutorGroup* execgrp = new ExecutorGroup( "Reading" );
    execgrp->setNrDoneText( "Nr done" );
    for ( int idx=0; idx<mids.size(); idx++ )
    {
	const ObjectID objid = getObjectID( mids[idx] );
	Executor* loader = objid<0 ? objectLoader( mids[idx], iosel ) : 0;
	if ( loader ) execgrp->add( loader );
    }

    return execgrp;
}


Executor* EMManager::objectLoader( const MultiID& mid,
				   const SurfaceIODataSelection* iosel )
{
    const ObjectID id = getObjectID( mid );
    EMObject* obj = getObject( id );
   
    if ( !obj )
    {
	PtrMan<IOObj> ioobj = IOM().get( mid );
	if ( !ioobj ) return 0;

	obj = EMOF().create( ioobj->group(), *this );
	if ( !obj ) return 0;
	obj->setMultiID( mid );
    }

    mDynamicCastGet(Surface*,surface,obj)
    if ( surface )
	return surface->geometry().loader(iosel);
    else if ( obj )
	return obj->loader();

    return 0;
}



EMObject* EMManager::loadIfNotFullyLoaded( const MultiID& mid,
					   TaskRunner* taskrunner )
{
    EM::ObjectID emid = EM::EMM().getObjectID( mid );
    RefMan<EM::EMObject> emobj = EM::EMM().getObject( emid );

    if ( !emobj || !emobj->isFullyLoaded() )
    {
	PtrMan<Executor> exec = EM::EMM().objectLoader( mid );
	if ( !exec )
	    return 0;

	if ( !(taskrunner ? taskrunner->execute(*exec) : exec->execute()) )
	    return 0;

	emid = EM::EMM().getObjectID( mid );
	emobj = EM::EMM().getObject( emid );
    }

    if ( !emobj || !emobj->isFullyLoaded() )
	return 0;

    EM::EMObject* tmpobj = emobj;
    tmpobj->ref();
    emobj = 0; //unrefs
    tmpobj->unRefNoDelete();

    return tmpobj;
}


const char* EMManager::getSurfaceData( const MultiID& id, SurfaceIOData& sd )
{
    PtrMan<IOObj> ioobj = IOM().get( id );
    if ( !ioobj )
	return id.isEmpty() ? 0 : "Object Manager cannot find surface";

    const char* grpname = ioobj->group();
    if ( !strcmp(grpname,EMHorizon2DTranslatorGroup::keyword) ||
	 !strcmp(grpname,EMHorizon3DTranslatorGroup::keyword) ||
	 !strcmp(grpname,EMFaultTranslatorGroup::keyword) )
    {
	PtrMan<EMSurfaceTranslator> tr = 
	    		(EMSurfaceTranslator*)ioobj->getTranslator();
	if ( !tr )
	{ return "Cannot create translator"; }

	if ( !tr->startRead( *ioobj ) )
	{
	    static BufferString msg;
	    msg = tr->errMsg();
	    if ( msg.isEmpty() )
		{ msg = "Cannot read '"; msg += ioobj->name(); msg += "'"; }

	    return msg.buf();
	}

	const SurfaceIOData& newsd = tr->selections().sd;
	sd.rg = newsd.rg;
	deepCopy( sd.sections, newsd.sections );
	deepCopy( sd.valnames, newsd.valnames );
	return 0;
    }

    pErrMsg("(read surface): unknown tr group");
    return 0;
}


void EMManager::syncGeometry( const ObjectID& id )
{
    syncGeomReq.trigger( id );
}


} // namespace EM
