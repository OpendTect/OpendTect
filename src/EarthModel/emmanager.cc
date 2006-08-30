/*+
 * COPYRIGHT: (C) dGB Beheer B.V.
 * AUTHOR   : K. Tingdahl
 * DATE     : Apr 2002
-*/

static const char* rcsID = "$Id: emmanager.cc,v 1.49 2006-08-30 16:03:27 cvsbert Exp $";

#include "emmanager.h"

#include "ctxtioobj.h"
#include "emhistory.h"
#include "emsurfacetr.h"
#include "emhorizontaltube.h"
#include "emsticksettransl.h"
#include "emobject.h"
#include "emsurfaceiodata.h"
#include "errh.h"
#include "executor.h"
#include "iodir.h"
#include "iopar.h"
#include "ioman.h"
#include "ptrman.h"


EM::EMManager& EM::EMM()
{
    static PtrMan<EMManager> emm = 0;

    if ( !emm ) emm = new EM::EMManager;
    return *emm;
}


EM::EMManager::EMManager()
    : history_( *new EM::History(*this) )
    , freeid( 0 )
{
    init();
}


EM::EMManager::~EMManager()
{
    empty();
    delete &history_;
}


void EM::EMManager::empty()
{   
    for ( int idx=0; idx<objects.size(); idx++ )
    {
	EMObjectCallbackData cbdata;
	cbdata.event = EMObjectCallbackData::Removal;

	const int oldsize = objects.size();
	objects[idx]->notifier.trigger(cbdata);
	if ( oldsize!=objects.size() ) idx--;
    }

    deepRef( objects );		//Removes all non-reffed 
    deepUnRef( objects );

    if ( objects.size() )
	pErrMsg( "All objects are not unreffed" );

    deepErase( objectfactories );

    history_.empty();
}


const EM::History& EM::EMManager::history() const
{ return history_; }


EM::History& EM::EMManager::history()
{ return history_; }


BufferString EM::EMManager::objectName(const MultiID& mid) const
{
    if ( getObject(getObjectID(mid)) )
	return getObject(getObjectID(mid))->name();

    PtrMan<IOObj> ioobj = IOM().get( mid );
    BufferString res;
    if ( ioobj ) res = ioobj->name();
    return res;
}


const char* EM::EMManager::objectType( const MultiID& mid ) const
{
    if ( getObject(getObjectID(mid)) )
	return getObject(getObjectID(mid))->getTypeStr();

    PtrMan<IOObj> ioobj = IOM().get( mid );
    if ( !ioobj ) 
	return 0;

    const ObjectFactory* fact = getFactory( ioobj->group() );
    return fact->typeStr();
}


void EM::EMManager::init()
{
    Horizon::initClass(*this);
    Fault::initClass(*this);
    HorizontalTube::initClass(*this);
    StickSet::initClass(*this);
    Horizon2D::initClass(*this);
} 


EM::ObjectID EM::EMManager::createObject( const char* type, const char* name )
{
    const ObjectFactory* fact = getFactory( type );
    EM::EMObject* object = fact ? fact->createObject( name, false ) : 0;

    if ( !object )
	return -1;

    object->setFullyLoaded( true );
    return object->id();
} 


MultiID EM::EMManager::findObject( const char* type,
					const char* name ) const
{
    const IOObjContext* context = getContext(type);
    if ( IOM().to(context->getSelKey()) )
    {
	PtrMan<IOObj> ioobj = IOM().getLocal( name );
	IOM().back();
	if ( ioobj ) return ioobj->key();
    }

    return -1;
}



EM::EMObject* EM::EMManager::getObject( const EM::ObjectID& id )
{
    for ( int idx=0; idx<objects.size(); idx++ )
    {
	if ( objects[idx]->id()==id )
	    return objects[idx];
    }

    return 0;
}


const EM::EMObject* EM::EMManager::getObject( const EM::ObjectID& id ) const
{ return const_cast<EM::EMManager*>(this)->getObject(id); }


EM::ObjectID EM::EMManager::getObjectID( const MultiID& mid ) const
{
    for ( int idx=0; idx<objects.size(); idx++ )
    {
	if ( objects[idx]->multiID()==mid )
	    return objects[idx]->id();
    }

    return -1;
}


MultiID EM::EMManager::getMultiID( const EM::ObjectID& oid ) const
{
    const EMObject* emobj = getObject(oid);
    return emobj ? emobj->multiID() : MultiID(-1);
}


EM::ObjectID EM::EMManager::addObject( EM::EMObject* obj )
{
    if ( !obj )
    {
	pErrMsg("No object provided!");
	return -1;
    }

    objects += obj;
    return freeid++;
}


void EM::EMManager::removeObject( EM::EMObject* obj )
{
    objects -= obj;
}


EM::EMObject* EM::EMManager::createTempObject( const char* type )
{
    const ObjectFactory* fact = getFactory( type );
    if ( !fact ) return 0;

    return fact->createObject( 0, true );
}


EM::ObjectID EM::EMManager::objectID(int idx) const
{ return idx>=0 && idx<objects.size() ? objects[idx]->id() : -1; }


Executor* EM::EMManager::objectLoader( const TypeSet<MultiID>& mids,
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


Executor* EM::EMManager::objectLoader( const MultiID& mid,
				       const SurfaceIODataSelection* iosel )
{
    ObjectID id = getObjectID( mid );
    EMObject* obj = getObject( id );
   
    if ( !obj )
    {
	PtrMan<IOObj> ioobj = IOM().get( mid );
	if ( !ioobj ) return 0;

	const ObjectFactory* fact = getFactory( ioobj->group() );
	obj = fact ? fact->loadObject( mid ) : 0;
    }

    mDynamicCastGet(EM::Surface*,surface,obj)
    if ( surface )
	return surface->geometry().loader(iosel);
    else if ( obj )
	return obj->loader();

    return 0;
}


const char* EM::EMManager::getSurfaceData( const MultiID& id,
					   EM::SurfaceIOData& sd )
{
    PtrMan<IOObj> ioobj = IOM().get( id );
    if ( !ioobj )
	return id == "" ? 0 : "Object Manager cannot find surface";

    const char* grpname = ioobj->group();
    if ( !strcmp(grpname,EMHorizonTranslatorGroup::keyword) ||
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
	    if ( msg == "" )
		{ msg = "Cannot read '"; msg += ioobj->name(); msg += "'"; }

	    return msg.buf();
	}

	const EM::SurfaceIOData& newsd = tr->selections().sd;
	sd.rg = newsd.rg;
	deepCopy( sd.sections, newsd.sections );
	deepCopy( sd.valnames, newsd.valnames );
	return 0;
    }

    pErrMsg("(read surface): unknown tr group");
    return 0;
}


void EM::EMManager::addFactory( ObjectFactory* fact )
{
    const ObjectFactory* existingfact = getFactory( fact->typeStr() );
    if ( existingfact )
    {
	delete fact;
	return;
    }

    objectfactories += fact;
}


const IOObjContext* EM::EMManager::getContext( const char* type ) const
{
    const ObjectFactory* fact = getFactory( type );
    return fact ? &fact->ioContext() : 0;
}


const EM::ObjectFactory* EM::EMManager::getFactory( const char* type ) const
{
    for ( int idx=0; idx<objectfactories.size(); idx++ )
    {
	if ( !strcmp(type,objectfactories[idx]->typeStr()) )
	{
	    return objectfactories[idx];
	}
    }

    return 0;
}
