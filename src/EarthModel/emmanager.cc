/*+
 * COPYRIGHT: (C) dGB Beheer B.V.
 * AUTHOR   : K. Tingdahl
 * DATE     : Apr 2002
-*/

static const char* rcsID = "$Id: emmanager.cc,v 1.36 2005-01-06 09:39:57 kristofer Exp $";

#include "emmanager.h"

#include "ctxtioobj.h"
#include "emhistory.h"
#include "emsurfacetr.h"
#include "emsticksettransl.h"
#include "emobject.h"
#include "emsurfaceiodata.h"
#include "errh.h"
#include "executor.h"
#include "iodir.h"
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
{
    init();
}


EM::EMManager::~EMManager()
{
    deepErase( objects );
    deepErase( objectfactories );
    delete &history_;
}


const EM::History& EM::EMManager::history() const
{ return history_; }


EM::History& EM::EMManager::history()
{ return history_; }


BufferString EM::EMManager::objectName(const EM::ObjectID& oid) const
{
    if ( getObject(oid) ) return getObject(oid)->name();
    MultiID mid = IOObjContext::getStdDirData(IOObjContext::Surf)->id;
    mid.add(oid);

    PtrMan<IOObj> ioobj = IOM().get( mid );
    BufferString res;
    if ( ioobj ) res = ioobj->name();
    return res;
}


EM::ObjectID EM::EMManager::findObject( const char* type,
					const char* name ) const
{
    const IOObjContext* context = getContext(type);
    if ( IOM().to(IOObjContext::getStdDirData(context->stdseltype)->id) )
    {
	PtrMan<IOObj> ioobj = IOM().getLocal( name );
	IOM().back();
	if ( ioobj ) return multiID2ObjectID(ioobj->key());
    }

    return -1;
}


const char* EM::EMManager::objectType(const EM::ObjectID& oid) const
{
    if ( getObject(oid) )
	return getObject(oid)->getTypeStr();

    MultiID mid = IOObjContext::getStdDirData(IOObjContext::Surf)->id;
    mid.add(oid);

    PtrMan<IOObj> ioobj = IOM().get( mid );
    if ( !ioobj ) 
	return 0;

    return ioobj->group();
}


void EM::EMManager::init()
{ } 


EM::ObjectID EM::EMManager::createObject( const char* type, const char* name )
{
    EM::EMObject* object = 0;
    for ( int idx=0; idx<objectfactories.size(); idx++ )
    {
	if ( !strcmp(type,objectfactories[idx]->typeStr()) )
	{
	    object = objectfactories[idx]->create( name, false, *this );
	    break;
	}
    }

    if ( !object )
	return -1;

    objects += object;
    refcounts += 0;

    return object->id();
} 


EM::EMObject* EM::EMManager::getObject( const EM::ObjectID& id )
{
    const EM::ObjectID objid = multiID2ObjectID(id);
    for ( int idx=0; idx<objects.size(); idx++ )
    {
	if ( objects[idx]->id()==objid )
	    return objects[idx];
    }

    return 0;
}


const EM::EMObject* EM::EMManager::getObject( const EM::ObjectID& id ) const
{ return const_cast<EM::EMManager*>(this)->getObject(id); }


EM::ObjectID EM::EMManager::multiID2ObjectID( const MultiID& id )
{ return id.leafID(); }


void EM::EMManager::removeObject( const EM::ObjectID& id )
{
    const EM::ObjectID objid = multiID2ObjectID(id);
    for ( int idx=0; idx<objects.size(); idx++ )
    {
	if ( objects[idx]->id() == objid )
	{
	    EMObjectCallbackData cbdata;
	    cbdata.event = EMObjectCallbackData::Removal;
	    objects[idx]->notifier.trigger(cbdata);
	    delete objects[idx];
	    objects.remove( idx );
	    refcounts.remove( idx );
	    return;
	}
    }
}


void EM::EMManager::ref( const EM::ObjectID& id )
{
    const EM::ObjectID objid = multiID2ObjectID(id);
    for ( int idx=0; idx<objects.size(); idx++ )
    {
	if ( objects[idx]->id() == objid )
	{
	    refcounts[idx]++;
	    return;
	}
    }

    pErrMsg("Reference of id does not exist");
}


void EM::EMManager::unRef( const EM::ObjectID& id )
{
    const EM::ObjectID objid = multiID2ObjectID(id);
    for ( int idx=0; idx<objects.size(); idx++ )
    {
	if ( objects[idx]->id() == objid )
	{
	    if ( !refcounts[idx] )
		pErrMsg("Un-refing object that is not reffed");

	    refcounts[idx]--;
	    if ( !refcounts[idx] )
		removeObject( id );

	    return;
	}
    }

    pErrMsg("Reference of id does not exist");
}


void EM::EMManager::unRefNoDel( const EM::ObjectID& id )
{
    const EM::ObjectID objid = multiID2ObjectID(id);
    for ( int idx=0; idx<objects.size(); idx++ )
    {
	if ( objects[idx]->id() == objid )
	{
	    if ( !refcounts[idx] )
		pErrMsg("Un-refing object that is not reffed");

	    refcounts[idx]--;
	    return;
	}
    }

    pErrMsg("Reference of id does not exist");
}

/*
void EM::EMManager::addObject( EM::EMObject* obj )
{
    if ( !obj ) return;
    objects += obj;
    refcounts += 0;
}
*/


EM::EMObject* EM::EMManager::createTempObject( const char* type )
{
    for ( int idx=0; idx<objectfactories.size(); idx++ )
    {
	if ( !strcmp(type,objectfactories[idx]->typeStr()) )
	    return objectfactories[idx]->create( 0, true, *this );
    }

    return 0;
}


EM::ObjectID EM::EMManager::objectID(int idx) const
{ return idx>=0 && idx<objects.size() ? objects[idx]->id() : -1; }


Executor* EM::EMManager::loadObject( const MultiID& mid,
				     const SurfaceIODataSelection* iosel )
{
    EM::ObjectID id = multiID2ObjectID(mid);
    EMObject* obj = getObject( id );
   
    if ( !obj )
    {
	PtrMan<IOObj> ioobj = IOM().get( mid );
	if ( !ioobj ) return 0;

	obj = createTempObject( ioobj->group() );
	if ( obj )
	{
	    objects += obj;
	    refcounts += 0;
	}
    }

    mDynamicCastGet(EM::Surface*,surface,obj)
    if ( surface )
	return surface->geometry.loader(iosel);
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
    for ( int idx=0; idx<objectfactories.size(); idx++ )
    {
	if ( !strcmp(fact->typeStr(),objectfactories[idx]->typeStr()) )
	{
	    delete fact;
	    return;
	}
    }

    objectfactories += fact;

}


const IOObjContext* EM::EMManager::getContext( const char* type ) const
{
    for ( int idx=0; idx<objectfactories.size(); idx++ )
    {
	if ( !strcmp(type,objectfactories[idx]->typeStr()) )
	    return &objectfactories[idx]->ioContext();
    }

    return 0;
}

