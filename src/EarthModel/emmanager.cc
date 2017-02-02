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

#define mDefineEMMan(typeprefix,translgrp) \
EM::EMManager& EM::typeprefix##Man() \
{ \
    mDefineStaticLocalObject( PtrMan<EM::EMManager>, emm, \
			      (new EM::EMManager(mIOObjContext(translgrp))) ); \
    return *emm; \
}

mDefineEMMan(Hor3D,EMHorizon3D)
mDefineEMMan(Hor2D,EMHorizon2D)
mDefineEMMan(FSS,EMFaultStickSet)
mDefineEMMan(Flt3D,EMFault3D)
mDefineEMMan(Body,EMBody)

EM::GenEMManager& EM::EMM()
{
    mDefineStaticLocalObject( PtrMan<EM::GenEMManager>, emm,
			(new EM::GenEMManager(mIOObjContext(EMHorizon3D))) );
    return *emm;
}


bool EM::canOverwrite( const DBKey& mid )
{
    const IOObj* ioobj = DBM().get( mid );
    if ( !ioobj )
	return true;

    mDynamicCastGet(const IOStream*,iostream,ioobj)
    return iostream;
}

namespace EM
{
const char* EMManager::displayparameterstr() { return "Display Parameters"; }



mImplFactory1Param( EMObject, EMManager&, EMOF );

EMManager::EMManager( const IOObjContext& ctxt )
    : SaveableManager(ctxt,true)
    , undo_( *new EMUndo() )
    , addRemove( this )
{
    mAttachCB( Strat::eLVLS().objectChanged(), EMManager::levelSetChgCB );
}


EMManager::~EMManager()
{
    detachAllNotifiers();
    setEmpty();
    delete &undo_;
}


void EMManager::setEmpty()
{
    savers_.setEmpty();
    addRemove.trigger();
    undo_.removeAll();
}


const Undo& EMManager::undo() const	{ return undo_; }
Undo& EMManager::undo()			{ return undo_; }


BufferString EMManager::objectName( const DBKey& id ) const
{
    return DBM().nameOf( id );
}


const char* EMManager::objectType( const DBKey& id ) const
{
    PtrMan<IOObj> ioobj = DBM().get( id );
    return ioobj ? ioobj->group() : OD::String::empty();
}


EMObject* EMManager::createObject( const char* type, const char* nm )
{
    EMObject* object = EMOF().create( type, *this );
    if ( !object ) return 0;

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


DBKey EMManager::objID( int idx ) const
{
    if ( !savers_.validIdx(idx) )
	return DBKey::getInvalid();
    return savers_[idx]->key();
}


EMObject* EMManager::getObject( const DBKey& id )
{
   mLock4Read();
   return gtObject( id );
}


EMObject* EMManager::gtObject( const DBKey& mid )
{
    for ( int idx=0; idx<savers_.size(); idx++ )
    {
	if ( savers_[idx]->key() == mid )
	    return mCast(EMObject*,savers_[idx]->object());
    }

    return 0;
}


ConstRefMan<EMObject> EMManager::fetch( const DBKey& mid, TaskRunner* trunner,
				bool forcereload ) const
{
    mLock4Read();
    EMObject* ret = const_cast<EMManager*>(this)->gtObject( mid );
    if ( !forcereload && ret && ret->isFullyLoaded() )
	return ret;

    PtrMan<Executor> exec = EM::Hor3DMan().objectLoader( mid );
    mUnlockAllAccess();
    if ( !exec || !TaskRunner::execute(trunner,*exec) )
	return 0;

    mReLock();
    return const_cast<EMManager*>(this)->gtObject( mid );
}


RefMan<EMObject> EMManager::fetchForEdit( const DBKey& mid, TaskRunner* trunner,
				bool forcereload )
{
    mLock4Read();
    EMObject* ret = gtObject( mid );
    if ( !forcereload && ret && ret->isFullyLoaded() )
	return ret;

    PtrMan<Executor> exec = EM::Hor3DMan().objectLoader( mid );
    mUnlockAllAccess();
    if ( !exec || !TaskRunner::execute(trunner,*exec) )
	return 0;

    mReLock();
    return gtObject( mid );
}


uiRetVal EMManager::store( const EMObject& emobj,
				  const IOPar* ioobjpars ) const
{
    return SaveableManager::store( emobj, ioobjpars );
}


uiRetVal EMManager::store( const EMObject& emobj, const ObjID& id,
			      const IOPar* ioobjpars ) const
{
    return SaveableManager::store( emobj, id, ioobjpars );
}


bool EMManager::objectExists( const EMObject* obj ) const
{
    return isPresent( *obj );
}


void EMManager::addObject( EMObject* obj )
{
    if ( !obj )
	{ pErrMsg("No object provided!"); return; }

    if ( isPresent(*obj) )
	{ pErrMsg("Adding object twice"); return; }

    addNew( *obj, obj->dbKey(), 0, true ); 
    addRemove.trigger();
}


EMObject* EMManager::createTempObject( const char* type )
{
    return EMOF().create( type, *this );
}


Executor* EMManager::objectLoader( const DBKeySet& mids,
				   const SurfaceIODataSelection* iosel,
				   DBKeySet* idstobeloaded )
{
    ExecutorGroup* execgrp = mids.size()>1 ? new ExecutorGroup( "Reading" ) : 0;
    for ( int idx=0; idx<mids.size(); idx++ )
    {
	const EMObject* obj = getObject( mids[idx] );
	Executor* loader =
	    obj && obj->isFullyLoaded() ? 0 : objectLoader( mids[idx], iosel );
	if ( idstobeloaded && loader )
	    *idstobeloaded += mids[idx];

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


Executor* EMManager::objectLoader( const DBKey& mid,
				   const SurfaceIODataSelection* iosel )
{
    EMObject* obj = getObject( mid );

    if ( !obj )
    {
	PtrMan<IOObj> ioobj = DBM().get( mid );
	if ( !ioobj ) return 0;

	BufferString typenm = ioobj->pars().find( sKey::Type() );
	if ( typenm.isEmpty() )
	    typenm = ioobj->group();

	obj = EMOF().create( typenm, *this );
	if ( !obj ) return 0;
	obj->setDBKey( mid );
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


EMObject* EMManager::loadIfNotFullyLoaded( const DBKey& mid,
					   TaskRunner* taskrunner )
{
    RefMan<EM::EMObject> emobj = getObject( mid );

    if ( !emobj || !emobj->isFullyLoaded() )
    {
	PtrMan<Executor> exec = objectLoader( mid );
	if ( !exec )
	    return 0;

	if ( !TaskRunner::execute( taskrunner, *exec ) )
	    return 0;

	emobj = getObject( mid );
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
	SharedObject* shobj = const_cast<SharedObject*>(savers_[idx]->object());
	mDynamicCastGet(EM::EMObject*,emobj,shobj);
	if ( emobj )
	    emobj->setBurstAlert( yn );
    }
}


void EMManager::removeSelected( const DBKey& id,
				const Selector<Coord3>& selector,
				TaskRunner* tskr )
{
    EM::EMObject* emobj = getObject( id );
    if ( !emobj ) return;

    emobj->ref();
    emobj->removeSelected( selector, tskr );
    emobj->unRef();
}


bool EMManager::readDisplayPars( const DBKey& mid, IOPar& outpar ) const
{
    if( !readParsFromDisplayInfoFile(mid,outpar) )
	return readParsFromGeometryInfoFile( mid, outpar );

    return true;

}


bool EMManager::readParsFromDisplayInfoFile( const DBKey& mid,
					     IOPar& outpar ) const
{
    outpar.setEmpty();

    IOObjInfo ioobjinfo( mid );
    if( !ioobjinfo.isOK() )
	return false;

    const BufferString filenm = Surface::getParFileName( *ioobjinfo.ioObj() );
    File::Path fp( filenm );
    fp.setExtension( "par" );
    od_istream strm( fp );

    if( !strm.isOK() )
	return false;

    return outpar.read( strm, displayparameterstr() );
}


bool EMManager::readParsFromGeometryInfoFile( const DBKey& mid,
					      IOPar& outpar ) const
{
    outpar.setEmpty();
    IOPar* par = IOObjInfo(mid).getPars();
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


bool EMManager::writeDisplayPars( const DBKey& mid,const IOPar& inpar ) const
{
    IOObjInfo ioobjinfo( mid );
    if( !ioobjinfo.isOK() )
	return false;

    IOPar displaypar;
    readDisplayPars( mid, displaypar );
    displaypar.merge( inpar );

    const BufferString filenm = Surface::getParFileName( *ioobjinfo.ioObj() );
    return displaypar.write( filenm.buf(), displayparameterstr() );

}


bool EMManager::getSurfaceData( const DBKey& mid, SurfaceIOData& sd,
				uiString& errmsg ) const
{
    EM::IOObjInfo oi( mid );
    return oi.getSurfaceData( sd, errmsg );
}


void EMManager::levelSetChgCB( CallBacker* cb )
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


Saveable* EMManager::getSaver( const SharedObject& shobj ) const
{
    mDynamicCastGet(const EMObject*,emobj,&shobj);
    if ( !emobj )
	return 0;

    ObjectSaver* objsaver = ObjectSaver::factory().create( emobj->getTypeStr(),
							   shobj );
    return objsaver;
}


EMManager& getMgr( const DBKey& id )
{
    PtrMan<IOObj> ioobj = DBM().get( id );
    return ioobj ? getMgr( ioobj->group() ) : Hor3DMan();
}


EMManager& getMgr( const char* trgrp )
{
    if ( mTranslGroupName(EMHorizon3D)==trgrp ) return Hor3DMan();
    if ( mTranslGroupName(EMHorizon2D)==trgrp ) return Hor2DMan();
    if ( mTranslGroupName(EMFaultStickSet)==trgrp ) return FSSMan();
    if ( mTranslGroupName(EMFault3D)==trgrp ) return Flt3DMan();
    if ( mTranslGroupName(EMBody)==trgrp ) return BodyMan();

    return Hor3DMan();
}


GenEMManager::GenEMManager( const IOObjContext& ctxt )
    : EMManager(ctxt)
{}


EMObject* GenEMManager::createTempObject( const char* type )
{
    FixedString trgrp( type );
    return getMgr( trgrp ).createTempObject( type );
}

} // namespace EM
