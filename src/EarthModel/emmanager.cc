/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : K. Tingdahl
 * DATE     : Apr 2002
-*/


#include "emmanager.h"

#include "ctxtioobj.h"
#include "embodytr.h"
#include "emobject.h"
#include "emhorizon.h"
#include "emsurfacegeometry.h"
#include "emsurfaceiodata.h"
#include "emsurfacetr.h"
#include "emioobjinfo.h"
#include "emobjectio.h"
#include "emundo.h"
#include "executor.h"
#include "filepath.h"
#include "iopar.h"
#include "dbman.h"
#include "iostrm.h"
#include "ptrman.h"
#include "selector.h"
#include "stratlevel.h"
#include "keystrs.h"
#include "od_iostream.h"


namespace EM
{
mImplFactory( EMObject, EMOF );
}

#define mDefineEMMan(typeprefix,translgrp) \
EM::ObjectManager& EM::typeprefix##Man() \
{ \
    mDefineStaticLocalObject( PtrMan<ObjectManager>, emm, \
			      (new ObjectManager(mIOObjContext(translgrp))) ); \
    return *emm; \
}

mDefineEMMan(Hor3D,EMHorizon3D)
mDefineEMMan(Hor2D,EMHorizon2D)
mDefineEMMan(FSS,EMFaultStickSet)
mDefineEMMan(Flt3D,EMFault3D)
mDefineEMMan(Body,EMBody)

EM::Manager& EM::MGR()
{
    mDefineStaticLocalObject( PtrMan<Manager>, emm, (new Manager) );
    return *emm;
}


bool EM::canOverwrite( const ObjectManager::ObjID& objid )
{
    const IOObj* ioobj = DBM().get( objid );
    if ( !ioobj )
	return true;

    mDynamicCastGet(const IOStream*,iostream,ioobj)
    return iostream;
}


const char* EM::ObjectManager::displayparameterstr()
{
    return "Display Parameters";
}


EM::ObjectManager::ObjectManager( const IOObjContext& ctxt )
    : SaveableManager(ctxt,true)
    , addRemove( this )
{
    mAttachCB( Strat::eLVLS().objectChanged(), ObjectManager::levelSetChgCB );
}


EM::ObjectManager::~ObjectManager()
{
    detachAllNotifiers();

    setEmpty();
}


void EM::ObjectManager::setEmpty()
{
    savers_.setEmpty();
    addRemove.trigger();
}


void EM::ObjectManager::eraseUndoList()
{
    deepErase( undolist_ );
}


BufferString EM::ObjectManager::objectName( const ObjID& id ) const
{
    return DBM().nameOf( id );
}


bool EM::ObjectManager::is2D( const ObjID& id ) const
{
    //TODO crappy impl, what about FSS's? should look for obj in loaded first
    PtrMan<IOObj> ioobj = DBM().get( id );
    if ( !ioobj )
	return false;

    return FixedString(ioobj->group())
	== EMHorizon2DTranslatorGroup::sGroupName();
}


const char* EM::ObjectManager::objectType( const ObjID& id ) const
{
    PtrMan<IOObj> ioobj = DBM().get( id );
    return ioobj ? ioobj->group() : OD::String::empty();
}


EM::EMObject* EM::ObjectManager::createObject( const char* type,
						const char* nm )
{
    EMObject* object = EMOF().create( type, *this );
    if ( !object )
	{ pErrMsg(BufferString("Unknown type: ",type)); return 0; }

    CtxtIOObj ctio( object->getIOObjContext() );
    ctio.ctxt_.forread_ = false;
    ctio.ioobj_ = 0;
    ctio.setName( nm );
    if ( ctio.fillObj() )
    {
	object->setDBKey( ctio.ioobj_->key() );
	delete ctio.ioobj_;
    }

    object->setFullyLoaded( true );
    return object;
}


EM::ObjectManager::ObjID EM::ObjectManager::objID( int idx ) const
{
    return savers_.validIdx(idx) ? savers_[idx]->key() : ObjID();
}


EM::EMObject* EM::ObjectManager::getObject( const ObjID& id )
{
   mLock4Read();
   return gtObject( id );
}


EM::EMObject* EM::ObjectManager::gtObject( const ObjID& objid )
{
    for ( int idx=0; idx<savers_.size(); idx++ )
    {
	if ( savers_[idx]->key() == objid )
	    return mCast(EMObject*,savers_[idx]->object());
    }

    return 0;
}


RefObjectSet<EM::EMObject> EM::ObjectManager::loadObjects( const char* typ,
					    const ObjIDSet& dbkeys,
					const SurfaceIODataSelection* sel,
					TaskRunner* tskr )
{
    RefObjectSet<EMObject> loadedpbjs;
    PtrMan<EM::ObjectLoader> emloader =
		EM::ObjectLoader::factory().create( typ, dbkeys, sel );
    if ( !emloader )
	return loadedpbjs;

    if ( emloader->load(tskr) )
	loadedpbjs = emloader->getLoadedEMObjects();

    return loadedpbjs;
}


ConstRefMan<EM::EMObject> EM::ObjectManager::fetch( const ObjID& objid,
				TaskRunner* trunnr, bool forcereload ) const
{
    mLock4Read();
    EMObject* ret = const_cast<ObjectManager*>(this)->gtObject( objid );
    if ( !forcereload && ret && ret->isFullyLoaded() )
	return ret;

    PtrMan<Executor> exec = EM::Hor3DMan().objectLoader( objid );
    mUnlockAllAccess();
    if ( !exec || !TaskRunner::execute(trunnr,*exec) )
	return 0;

    mReLock();
    return const_cast<ObjectManager*>(this)->gtObject( objid );
}


RefMan<EM::EMObject> EM::ObjectManager::fetchForEdit( const ObjID& objid,
				TaskRunner* trunnr, bool forcereload )
{
    mLock4Read();
    EMObject* ret = gtObject( objid );
    if ( !forcereload && ret && ret->isFullyLoaded() )
	return ret;

    PtrMan<Executor> exec = EM::Hor3DMan().objectLoader( objid );
    mUnlockAllAccess();
    if ( !exec || !TaskRunner::execute(trunnr,*exec) )
	return 0;

    mReLock();
    return gtObject( objid );
}


uiRetVal EM::ObjectManager::store( const EMObject& emobj,
				  const IOPar* ioobjpars ) const
{
    return SaveableManager::store( emobj, ioobjpars );
}


uiRetVal EM::ObjectManager::store( const EMObject& emobj, const ObjID& id,
			      const IOPar* ioobjpars ) const
{
    return SaveableManager::store( emobj, id, ioobjpars );
}


bool EM::ObjectManager::objectExists( const EMObject* obj ) const
{
    return isPresent( *obj );
}


void EM::ObjectManager::addObject( EMObject* obj )
{
    if ( !obj )
    { pErrMsg("No object provided!"); return; }

    if ( isPresent(*obj) )
    { pErrMsg("Adding object twice"); return; }

    addNew( *obj, obj->dbKey(), 0, true );
    addRemove.trigger();
}


EM::EMObject* EM::ObjectManager::createTempObject( const char* type )
{
    return EMOF().create( type, *this );
}


Executor* EM::ObjectManager::objectLoader( const ObjIDSet& objids,
				   const SurfaceIODataSelection* iosel,
				   ObjIDSet* includedids )
{
    ExecutorGroup* execgrp = objids.size()>1
			   ? new ExecutorGroup( "Reading" ) : 0;
    for ( int idx=0; idx<objids.size(); idx++ )
    {
	const EMObject* obj = getObject( objids[idx] );
	Executor* loader = obj && obj->isFullyLoaded()
			 ? 0 : objectLoader( objids[idx], iosel );
	if ( includedids && loader )
	    *includedids += objids[idx];

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


Executor* EM::ObjectManager::objectLoader( const ObjID& objid,
				   const SurfaceIODataSelection* iosel )
{
    EMObject* obj = getObject( objid );

    if ( !obj )
    {
	PtrMan<IOObj> ioobj = DBM().get( objid );
	if ( !ioobj )
	    return 0;

	BufferString typenm = ioobj->pars().find( sKey::Type() );
	if ( typenm.isEmpty() )
	    typenm = ioobj->group();

	obj = EMOF().create( typenm, *this );
	if ( !obj )
	    return 0;
	obj->setDBKey( objid );
    }

    mDynamicCastGet(Surface*,surface,obj)
    if ( surface )
    {
	mDynamicCastGet(RowColSurfaceGeometry*,geom,&surface->geometry())
	if ( geom && iosel )
	{
	    TrcKeySampling hs;
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


EM::EMObject* EM::ObjectManager::loadIfNotFullyLoaded( const ObjID& objid,
					   const TaskRunnerProvider& trprov )
{
    RefMan<EM::EMObject> emobj = getObject( objid );

    if ( !emobj || !emobj->isFullyLoaded() )
    {
	PtrMan<Executor> exec = objectLoader( objid );
	if ( !exec )
	    return 0;

	if ( !trprov.execute(*exec) )
	    return 0;

	emobj = getObject( objid );
    }

    if ( !emobj || !emobj->isFullyLoaded() )
	return 0;

    EM::EMObject* tmpobj = emobj;
    tmpobj->ref();
    emobj = 0; //unrefs
    tmpobj->unRefNoDelete();

    return tmpobj;
}


void EM::ObjectManager::burstAlertToAll( bool yn )
{
    for ( int idx=nrLoadedObjects()-1; idx>=0; idx-- )
    {
	SharedObject* shobj = const_cast<SharedObject*>(savers_[idx]->object());
	mDynamicCastGet(EM::EMObject*,emobj,shobj);
	if ( emobj )
	    emobj->setBurstAlert( yn );
    }
}


void EM::ObjectManager::removeSelected( const ObjID& id,
				const Selector<Coord3>& selector,
				const TaskRunnerProvider& trprov )
{
    EM::EMObject* emobj = getObject( id );
    if ( !emobj )
	return;

    emobj->ref();
    emobj->removeSelected( selector, trprov );
    emobj->unRef();
}


bool EM::ObjectManager::readDisplayPars( const ObjID& objid,
					 IOPar& outpar ) const
{
    if( !readParsFromDisplayInfoFile(objid,outpar) )
	return readParsFromGeometryInfoFile( objid, outpar );

    return true;

}


bool EM::ObjectManager::readParsFromDisplayInfoFile( const ObjID& objid,
					     IOPar& outpar ) const
{
    outpar.setEmpty();

    IOObjInfo ioobjinfo( objid );
    if( !ioobjinfo.isOK() )
	return false;

    const BufferString filenm = Surface::getParFileName( *ioobjinfo.ioObj() );
    File::Path fp( filenm );
    fp.setExtension( sParFileExtension() );
    od_istream strm( fp );

    if( !strm.isOK() )
	return false;

    return outpar.read( strm, displayparameterstr() );
}


bool EM::ObjectManager::readParsFromGeometryInfoFile( const ObjID& objid,
					      IOPar& outpar ) const
{
    outpar.setEmpty();
    IOPar* par = IOObjInfo(objid).getPars();
    if( !par )
	return false;

    Color col;
    if( par->get(sKey::Color(),col) )
	outpar.set( sKey::Color(), col );

    BufferString lnststr;
    if( par->get(sKey::LineStyle(),lnststr) )
	outpar.set( sKey::LineStyle(), lnststr );

    BufferString mkststr;
    if( par->get(sKey::MarkerStyle(),mkststr) )
	outpar.set( sKey::MarkerStyle(), mkststr );

    int lvlid;
    if( par->get(sKey::StratRef(),lvlid) )
	outpar.set( sKey::StratRef(), lvlid );

    delete par;
    return true;

}


bool EM::ObjectManager::writeDisplayPars( const ObjID& objid,
					  const IOPar& inpar ) const
{
    IOObjInfo ioobjinfo( objid );
    if( !ioobjinfo.isOK() )
	return false;

    IOPar displaypar;
    readDisplayPars( objid, displaypar );
    displaypar.merge( inpar );

    const BufferString filenm = Surface::getParFileName( *ioobjinfo.ioObj() );
    return displaypar.write( filenm.buf(), displayparameterstr() );

}


bool EM::ObjectManager::getSurfaceData( const ObjID& objid, SurfaceIOData& sd,
				uiString& errmsg ) const
{
    EM::IOObjInfo oi( objid );
    return oi.getSurfaceData( sd, errmsg );
}


void EM::ObjectManager::levelSetChgCB( CallBacker* cb )
{
    mGetMonitoredChgData( cb, chgdata );
    if ( chgdata.changeType() != Strat::LevelSet::cLevelToBeRemoved() )
	return;

    mGetIDFromChgData( Strat::Level::ID, lvlid, chgdata );
    for ( int idx=0; idx<savers_.size(); idx++ )
    {
	SharedObject* shobj = const_cast<SharedObject*>(savers_[idx]->object());
	mDynamicCastGet( EM::Horizon*, hor, shobj )
	if ( hor && hor->stratLevelID() == lvlid )
	    hor->setStratLevelID( Strat::Level::ID::getInvalid() );
    }
}


Saveable* EM::ObjectManager::getSaver( const SharedObject& shobj ) const
{
    mDynamicCastGet(const EMObject*,emobj,&shobj);
    if ( !emobj )
	return 0;

    ObjectSaver* objsaver = ObjectSaver::factory().create( emobj->getTypeStr(),
							   shobj );
    return objsaver;
}


EM::Undo& EM::ObjectManager::undo( const ObjID& id )
{
    const int idx = undoIndexOf( id );
    if ( undolist_.validIdx(idx) )
	return undolist_[idx]->undo_;

    ObjUndo* newemobjundo = new ObjUndo( id );
    undolist_ += newemobjundo;
    return newemobjundo->undo_;
}


int EM::ObjectManager::undoIndexOf( const ObjID& id )
{
    for ( int idx=0; idx<undolist_.size(); idx++ )
    {
	if ( undolist_[idx]->id_ == id )
	    return idx;
    }

    return -1;
}


EM::ObjectManager& EM::getMgr( const ObjID& id )
{
    PtrMan<IOObj> ioobj = DBM().get( id );
    return ioobj ? getMgr( ioobj->group().str() ) : Hor3DMan();
}


EM::ObjectManager& EM::getMgr( const char* trgrp )
{
    if ( mTranslGroupName(EMHorizon3D)==trgrp ) return Hor3DMan();
    if ( mTranslGroupName(EMHorizon2D)==trgrp ) return Hor2DMan();
    if ( mTranslGroupName(EMFaultStickSet)==trgrp ) return FSSMan();
    if ( mTranslGroupName(EMFault3D)==trgrp ) return Flt3DMan();
    if ( mTranslGroupName(EMBody)==trgrp ) return BodyMan();

    return Hor3DMan();
}


EM::Manager::Manager()
    : ObjectManager(mIOObjContext(EMHorizon3D)) // any ctxt will do, not used
{
}


void EM::Manager::addObject( EMObject* obj )
{
    if ( !obj )
	{ pErrMsg("No object provided!"); return; }

    if ( isPresent(*obj) )
	{ pErrMsg("Adding object twice"); return; }

    getMgr( obj->dbKey() ).addObject( obj );
}


EM::EMObject* EM::Manager::createTempObject( const char* type )
{
    FixedString trgrp( type );
    return getMgr( trgrp ).createTempObject( type );
}
