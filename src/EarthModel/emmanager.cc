/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : K. Tingdahl
 * DATE     : Apr 2002
-*/

static const char* rcsID mUsedVar = "$Id$";

#include "emmanager.h"

#include "ctxtioobj.h"
#include "emobject.h"
#include "emhorizon.h"
#include "emsurfacegeometry.h"
#include "emsurfaceiodata.h"
#include "emioobjinfo.h"
#include "errh.h"
#include "executor.h"
#include "filepath.h"
#include "iopar.h"
#include "ioman.h"
#include "ptrman.h"
#include "undo.h"
#include "selector.h"
#include "stratlevel.h"
#include "keystrs.h"


EM::EMManager& EM::EMM()
{
    static EMManager* emm = 0;
    if ( !emm )
	emm = new EM::EMManager;
    return *emm;
}


namespace EM
{


mImplFactory1Param( EMObject, EMManager&, EMOF );

EMManager::EMManager()
    : undo_( *new Undo() )
    , addRemove( this )
{
    Strat::LevelSet& lvlset = Strat::eLVLS();
    lvlset.levelToBeRemoved.notify( mCB(this, EMManager, levelToBeRemoved ) );
}


EMManager::~EMManager()
{
    setEmpty();
    delete &undo_;
}


void EMManager::setEmpty()
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

    IOObjInfo ioobjinfo( mid );
    if ( !ioobjinfo.isOK() )
	return 0;

    const IOObj& ioobj = *ioobjinfo.ioObj();
    FixedString typenm = ioobj.pars().find( sKey::Type() );
    if ( !typenm )
	typenm = ioobj.group();

    const int idx = EMOF().getNames().indexOf( typenm );
    if ( idx<0 )
	return 0;

    return EMOF().getNames()[idx]->buf();
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
    {
    	object->setMultiID( ctio.ioobj->key() );
	delete ctio.ioobj;
    }

    object->setFullyLoaded( true );
    return object->id();
}


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


bool EMManager::objectExists( const EMObject* obj ) const
{
    return objects_.indexOf( obj ) != -1;
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
    objects_.removeSingle( idx );
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
    ExecutorGroup* execgrp = mids.size()>1 ? new ExecutorGroup( "Reading" ) : 0;
    for ( int idx=0; idx<mids.size(); idx++ )
    {
	const ObjectID objid = getObjectID( mids[idx] );
	const EMObject* obj = getObject( objid );
	Executor* loader =
	    obj && obj->isFullyLoaded() ? 0 : objectLoader( mids[idx], iosel );
	if ( execgrp )
	{
	    if ( loader )
	    {
		if ( !execgrp->nrExecutors() )
		    execgrp->setNrDoneText( loader->nrDoneText() );
		execgrp->add( loader );
	    }
	}
	else
	{
	    return loader;
	}
    }

    if ( execgrp && !execgrp->nrExecutors() )
    {
	delete execgrp;
	execgrp = 0;
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

	FixedString typenm = ioobj->pars().find( sKey::Type() );
	if ( !typenm )
	    typenm = ioobj->group();

	obj = EMOF().create( typenm, *this );
	if ( !obj ) return 0;
	obj->setMultiID( mid );
    }

    mDynamicCastGet(Surface*,surface,obj)
    if ( surface )
    {
	mDynamicCastGet(RowColSurfaceGeometry*,geom,&surface->geometry())
	if ( geom && iosel )
	{
	    HorSampling hs;
	    hs.setInlRange( geom->rowRange() );
	    hs.setCrlRange( geom->colRange() );
	    if ( hs.isEmpty() )
		return geom->loader( iosel );

	    SurfaceIODataSelection newsel( *iosel );
	    newsel.rg.include( hs );
	    return geom->loader( &newsel );
	}

	return surface->geometry().loader(iosel);
    }
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

	if ( !TaskRunner::execute( taskrunner, *exec ) )
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


void EMManager::burstAlertToAll( bool yn )
{
    for ( int idx=nrLoadedObjects()-1; idx>=0; idx-- )
    {
	const EM::ObjectID oid = objectID( idx );
	EM::EMObject* emobj = getObject( oid );
	emobj->setBurstAlert( yn );
    }
}


void EMManager::removeSelected( const ObjectID& id, 
				const Selector<Coord3>& selector,
       				TaskRunner* tr )
{
    EM::EMObject* emobj = getObject( id );
    if ( !emobj ) return;

    emobj->ref();
    emobj->removeSelected( selector, tr );
    emobj->unRef();
}


bool EMManager::readPars( const MultiID& mid, IOPar& outpar ) const
{
    outpar.setEmpty();
    IOPar* par = IOObjInfo(mid).getPars();
    if ( !par )
	return false;

    outpar = *par;
    delete par;
    return true;
}


bool EMManager::writePars( const MultiID& mid, const IOPar& inpar ) const
{
    IOObjInfo ioobjinfo( mid );
    if ( !ioobjinfo.isOK() || !ioobjinfo.isHorizon() )
	return false;

    IOPar* rdpar = ioobjinfo.getPars();
    IOPar wrpar( *rdpar ); delete rdpar;
    wrpar.merge( inpar );
    const BufferString filenm = Surface::getParFileName( *ioobjinfo.ioObj() );
    return wrpar.write( filenm.buf(), "Surface parameters" );
}


void EMManager::getSurfaceData( const MultiID& mid, SurfaceIOData& sd ) const
{
    EM::IOObjInfo oi( mid );
    oi.getSurfaceData( sd );
}


void EMManager::levelToBeRemoved( CallBacker* cb )
{
    mDynamicCastGet(Strat::LevelSet*,lvlset,cb)
    if ( !lvlset ) pErrMsg( "Can't find levelSet" );
    const int lvlidx = lvlset->notifLvlIdx();
    if ( !lvlset->levels().validIdx( lvlidx ) ) return;
    const Strat::Level& lvl = *lvlset->levels()[lvlidx];
    for ( int idx=0; idx<objects_.size(); idx++ )
    {
	mDynamicCastGet( EM::Horizon*, hor, objects_[idx] )
	if ( hor && hor->stratLevelID() == lvl.id() )
	    hor->setStratLevelID( -1 );
    }
}

} // namespace EM
