/*+
 * COPYRIGHT: (C) de Groot-Bril Earth Sciences B.V.
 * AUTHOR   : K. Tingdahl
 * DATE     : Apr 2002
-*/

static const char* rcsID = "$Id: emmanager.cc,v 1.21 2003-09-09 16:06:12 kristofer Exp $";

#include "emmanager.h"

#include "ctxtioobj.h"
#include "emfaulttransl.h"
#include "emhistory.h"
#include "emhorizontransl.h"
#include "emsticksettransl.h"
#include "emobject.h"
#include "emsurfaceiodata.h"
#include "errh.h"
#include "executor.h"
#include "iodir.h"
#include "ioman.h"
#include "ptrman.h"
#include "uidobjset.h"


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
    delete &history_;
}


const EM::History& EM::EMManager::history() const
{ return history_; }


EM::History& EM::EMManager::history()
{ return history_; }


void EM::EMManager::init()
{ } 


MultiID EM::EMManager::add( EM::EMManager::Type type, const char* name )
{
    CtxtIOObj* ctio = 0;
    if ( type==EM::EMManager::Hor )
	ctio = new CtxtIOObj(EMHorizonTranslator::ioContext());
    else if ( type==EM::EMManager::Fault )
	ctio = new CtxtIOObj(EMFaultTranslator::ioContext());
    else if ( type==EMManager::StickSet )
	ctio = new CtxtIOObj(EMStickSetTranslator::ioContext());
    else
	return -1;

    ctio->ctxt.forread = false;
    ctio->ioobj = 0;
    ctio->setName( name );
    ctio->fillObj();
    if ( !ctio->ioobj ) return -1;

    EMObject* obj = EM::EMObject::create( *ctio->ioobj, *this );
    if ( !obj )
	return -1;

    objects += obj;
    refcounts += 0;

    PtrMan<Executor> saver = obj->saver();
    if ( saver )
	saver->execute();

    return obj->id();
} 


EM::EMObject* EM::EMManager::getObject( const MultiID& id )
{
    for ( int idx=0; idx<objects.size(); idx++ )
    {
	if ( objects[idx]->id()==id )
	    return objects[idx];
    }

    return 0;
}


const EM::EMObject* EM::EMManager::getObject( const MultiID& id ) const
{
    for ( int idx=0; idx<objects.size(); idx++ )
    {
	if ( objects[idx]->id()==id )
	    return objects[idx];
    }

    return 0;
}


const EM::EMObject* EM::EMManager::getEMObject( int idx ) const
{
    if ( objects.size() )
	return objects[idx];

    return 0;
}


EM::EMObject* EM::EMManager::getEMObject( int idx )
{
    if ( objects.size() )
	return objects[idx];

    return 0;
}


void EM::EMManager::removeObject( const MultiID& id )
{
    for ( int idx=0; idx<objects.size(); idx++ )
    {
	if ( objects[idx]->id() == id )
	{
	    delete objects[idx];
	    objects.remove( idx );
	    refcounts.remove( idx );
	    return;
	}
    }
}


void EM::EMManager::ref( const MultiID& id )
{
    for ( int idx=0; idx<objects.size(); idx++ )
    {
	if ( objects[idx]->id() == id )
	{
	    refcounts[idx]++;
	    return;
	}
    }

    pErrMsg("Reference of id does not exist");
}


void EM::EMManager::unRef( const MultiID& id )
{
    for ( int idx=0; idx<objects.size(); idx++ )
    {
	if ( objects[idx]->id() == id )
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


void EM::EMManager::unRefNoDel( const MultiID& id )
{
    for ( int idx=0; idx<objects.size(); idx++ )
    {
	if ( objects[idx]->id() == id )
	{
	    if ( !refcounts[idx] )
		pErrMsg("Un-refing object that is not reffed");

	    refcounts[idx]--;
	    return;
	}
    }

    pErrMsg("Reference of id does not exist");
}


void EM::EMManager::addObject( EM::EMObject* obj )
{
    if ( !obj ) return;
    objects += obj;
    refcounts += 0;
}


EM::EMObject* EM::EMManager::getTempObj( EM::EMManager::Type type )
{
    EMObject* res = 0;
    if ( type==EM::EMManager::Hor )
	res = new EM::Horizon( *this, -1 );
    else if ( type==EM::EMManager::Fault )
	res = new EM::Fault( *this, -1 );

    return res;
}


EM::EMObject* EM::EMManager::createObject( const MultiID& id, bool addtoman )
{
    EMObject* obj = getObject( id );
    if ( obj && addtoman ) return obj;

    PtrMan<IOObj> ioobj = IOM().get( id );
    if ( !ioobj ) return false;

    EMObject* newobj = EM::EMObject::create( *ioobj, *this );
    if ( newobj && addtoman )
    {
	objects += newobj;
	refcounts += 0;
	return newobj;
    }

    return newobj;
}


Executor* EM::EMManager::load( const MultiID& id )
{
    EMObject* obj = getObject( id );
   
    if ( !obj )
    {
	PtrMan<IOObj> ioobj = IOM().get( id );
	if ( !ioobj ) return 0;

	obj = EM::EMObject::create( *ioobj, *this );
	if ( obj )
	{
	    objects += obj;
	    refcounts += 0;
	}
    }

    mDynamicCastGet(EM::Surface*,surface,obj)
    if ( surface )
	return surface->loader();
    else if ( obj )
	return obj->loader();

    return 0;
}


bool EM::EMManager::isLoaded( const MultiID& id ) const
{
    const EMObject* obj = getObject( id );
    return obj ? obj->isLoaded() : false;
}


void EM::EMManager::getSurfaceData( const MultiID& id, EM::SurfaceIOData& sd )
{
    PtrMan<IOObj> ioobj = IOM().get( id );
    if ( !ioobj ) return;

    const char* grpname = ioobj->group();
    if ( !strcmp( grpname, EMHorizonTranslator::keyword ))
    {
	dgbEMHorizonTranslator tr;
	if ( !tr.startRead( *ioobj ) )
	    return;

	const EM::SurfaceIOData& newsd = tr.selections().sd;
	sd.rg = newsd.rg;
	deepCopy( sd.patches, newsd.patches );
	deepCopy( sd.valnames, newsd.valnames );
    }
}
