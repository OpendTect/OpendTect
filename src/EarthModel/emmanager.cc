/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : K. Tingdahl
 * DATE     : Apr 2002
-*/


#include "emmanager.h"

#include "ctxtioobj.h"
#include "embodytr.h"
#include "emobject.h"
#include "emfaultset3d.h"
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
#include "iostrm.h"
#include "ptrman.h"
#include "selector.h"
#include "stratlevel.h"
#include "keystrs.h"
#include "od_iostream.h"


namespace EM
{
mImplFactory( Object, EMOF );
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
    PtrMan<IOObj> ioobj = objid.getIOObj();
    return ioobj && ioobj->isStream();
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
    return id.name();
}


bool EM::ObjectManager::is2D( const ObjID& id ) const
{
    //TODO crappy impl, what about FSS's? should look for obj in loaded first
    PtrMan<IOObj> ioobj = id.getIOObj();
    if ( !ioobj )
	return false;

    return ioobj->group() == EMHorizon2DTranslatorGroup::sGroupName();
}


BufferString EM::ObjectManager::objectType( const ObjID& id ) const
{
    PtrMan<IOObj> ioobj = id.getIOObj();
    return ioobj ? ioobj->group() : OD::String::empty();
}


EM::Object* EM::ObjectManager::createObject( const char* type,
						const char* nm )
{
    Object* object = EMOF().create( type, *this );
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


EM::Object* EM::ObjectManager::getObject( const ObjID& id )
{
   mLock4Read();
   return gtObject( id );
}


EM::Object* EM::ObjectManager::gtObject( const ObjID& objid )
{
    for ( int idx=0; idx<savers_.size(); idx++ )
    {
	if ( savers_[idx]->key() == objid )
	{
	    mDynamicCastGet(const Object*,emobj,savers_[idx]->object())
	    return const_cast<Object*>(emobj);
	}
    }

    return 0;
}


RefObjectSet<EM::Object> EM::ObjectManager::loadObjects(const ObjIDSet& dbkeys,
					const TaskRunnerProvider& trprov,
					const SurfaceIODataSelection* sel )
{
    RefObjectSet<Object> loadedpbjs;
    PtrMan<EM::ObjectLoader> emloader = objectLoader( dbkeys, sel );
    if ( !emloader )
	return loadedpbjs;

    if ( emloader->load(trprov) )
	loadedpbjs = emloader->getLoadedEMObjects();

    return loadedpbjs;
}


ConstRefMan<EM::Object> EM::ObjectManager::fetch( const ObjID& objid,
					    const TaskRunnerProvider& trprov,
					    const SurfaceIODataSelection* sel,
					    bool forcereload ) const
{
    mLock4Read();
    Object* ret = const_cast<ObjectManager*>(this)->gtObject( objid );
    if ( !forcereload && ret && ret->isFullyLoaded() )
	return ret;

    PtrMan<EM::ObjectLoader> loader = EM::Hor3DMan().objectLoader( objid, sel );
    mUnlockAllAccess();
    if ( !loader || !loader->load(trprov) )
	return 0;

    mReLock();
    return const_cast<ObjectManager*>(this)->gtObject( objid );
}


RefMan<EM::Object> EM::ObjectManager::fetchForEdit( const ObjID& objid,
					const TaskRunnerProvider& trprov,
					const SurfaceIODataSelection* sel,
					bool forcereload )
{
    mLock4Read();
    Object* ret = gtObject( objid );
    if ( !forcereload && ret && ret->isFullyLoaded() )
	return ret;

    PtrMan<EM::ObjectLoader> loader = EM::Hor3DMan().objectLoader( objid, sel );
    mUnlockAllAccess();
    if ( !loader || !loader->load(trprov) )
	return 0;

    mReLock();
    return gtObject( objid );
}


uiRetVal EM::ObjectManager::store( const Object& emobj,
				   const TaskRunnerProvider& trprov,
				   const IOPar* ioobjpars ) const
{
    return SaveableManager::store( emobj, trprov, ioobjpars );
}


uiRetVal EM::ObjectManager::store( const Object& emobj, const ObjID& id,
				   const TaskRunnerProvider& trprov,
				   const IOPar* ioobjpars ) const
{
    return SaveableManager::store( emobj, id, trprov, ioobjpars );
}


bool EM::ObjectManager::objectExists( const Object* obj ) const
{
    return isPresent( *obj );
}


void EM::ObjectManager::addObject( Object* obj )
{
    if ( !obj )
	{ pErrMsg("No object provided!"); return; }

    if ( isPresent(*obj) )
	{ pErrMsg("Adding object twice"); return; }

    PtrMan<IOObj> ioobj = getIOObj( obj->dbKey() );
    if ( ioobj )
	addNew( *obj, ioobj->key(), &ioobj->pars(), true );
    else
	addNew( *obj, obj->dbKey(), 0, true );
    addRemove.trigger();
}


EM::Object* EM::ObjectManager::createTempObject( const char* type )
{
    return EMOF().create( type, *this );
}


EM::ObjectLoader* EM::ObjectManager::objectLoader( const ObjIDSet& objids,
				   const SurfaceIODataSelection* iosel,
				   ObjIDSet* includedids )
{
    BufferString objtype;
    for ( int idx=0; idx<objids.size(); idx++ )
    {
	if ( !idx )
	    objtype = objectType( objids[idx] );
	else if ( objtype != objectType(objids[idx]) )
	{ pErrMsg("Should be used t load objects of the same type only"); }
    }

    if ( objtype.isEmpty() ) // Huh?
	return 0;

    return EM::ObjectLoader::factory().create( objtype, objids, iosel );
}


EM::ObjectLoader* EM::ObjectManager::objectLoader( const ObjID& objid,
				   const SurfaceIODataSelection* iosel )
{
    DBKeySet objids;
    objids.add( objid );
    return objectLoader( objids, iosel );
}


EM::Object* EM::ObjectManager::loadIfNotFullyLoaded( const ObjID& objid,
					   const TaskRunnerProvider& trprov )
{
    RefMan<EM::Object> emobj = getObject( objid );

    mLock4Read();
    if ( !emobj || !emobj->isFullyLoaded() )
    {
	PtrMan<EM::ObjectLoader> loader = EM::Hor3DMan().objectLoader( objid );
	mUnlockAllAccess();
	if ( !loader || !loader->load(trprov) )
	    return 0;

	emobj = getObject( objid );
    }

    if ( !emobj || !emobj->isFullyLoaded() )
	return 0;

    EM::Object* tmpobj = emobj;
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
	mDynamicCastGet(EM::Object*,emobj,shobj);
	if ( emobj )
	    emobj->setBurstAlert( yn );
    }
}


void EM::ObjectManager::removeSelected( const ObjID& id,
				const Selector<Coord3>& selector,
				const TaskRunnerProvider& trprov )
{
    EM::Object* emobj = getObject( id );
    if ( !emobj )
	return;

    emobj->ref();
    emobj->removeSelected( selector, trprov );
    emobj->unRef();
}


bool EM::ObjectManager::readDisplayPars( const ObjID& objid,
					 IOPar& outpar )
{
    if( !readParsFromDisplayInfoFile(objid,outpar) )
	return readParsFromGeometryInfoFile( objid, outpar );

    return true;

}


bool EM::ObjectManager::readParsFromDisplayInfoFile( const ObjID& objid,
					     IOPar& outpar )
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
					      IOPar& outpar )
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
					  const IOPar& inpar )
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
    mDynamicCastGet(const Object*,emobj,&shobj);
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
    PtrMan<IOObj> ioobj = id.getIOObj();
    return ioobj ? getMgr( ioobj->group().str() ) : Hor3DMan();
}


EM::ObjectManager& EM::getMgr( const char* trgrp )
{
    if ( mTranslGroupName(EMHorizon3D)==trgrp ) return Hor3DMan();
    if ( mTranslGroupName(EMHorizon2D)==trgrp ) return Hor2DMan();
    if ( mTranslGroupName(EMFaultStickSet)==trgrp ) return FSSMan();
    if ( mTranslGroupName(EMFault3D)==trgrp ) return Flt3DMan();
    if ( mTranslGroupName(EMFaultSet3D)==trgrp ) return FltSetMan();
    if ( mTranslGroupName(EMBody)==trgrp ) return BodyMan();

    return Hor3DMan();
}


EM::Manager::Manager()
    : ObjectManager(mIOObjContext(EMHorizon3D)) // any ctxt will do, not used
{
}


void EM::Manager::addObject( Object* obj )
{
    if ( !obj )
	{ pErrMsg("No object provided!"); return; }

    if ( isPresent(*obj) )
	{ pErrMsg("Adding object twice"); return; }

    getMgr( obj->dbKey() ).addObject( obj );
}


EM::Object* EM::Manager::createTempObject( const char* type )
{
    FixedString trgrp( type );
    return getMgr( trgrp ).createTempObject( type );
}


EM::FaultSetManager::FaultSetManager()
    : EM::ObjectManager(mIOObjContext(EMFaultSet3D))
{}


ConstRefMan<EM::Object> EM::FaultSetManager::fetch(const ObjID& dbkey,
					   const TaskRunnerProvider& tp,
					   const SurfaceIODataSelection*,
					   bool forcereload ) const
{
    PtrMan<IOObj> ioobj = dbkey.getIOObj();
    if ( !ioobj )
	return 0;

    PtrMan<EMFaultSet3DTranslator> transl =
			(EMFaultSet3DTranslator*)ioobj->createTranslator();
    if ( !transl )
	return 0;

    EM::FaultSetManager* self = const_cast<EM::FaultSetManager*>(this);
    mDynamicCastGet( EM::FaultSet3D*, fltset,
		     self->createTempObject(EM::FaultSet3D::typeStr()));
    PtrMan<Executor> loader = transl->reader( *fltset, *ioobj );
    if ( !tp.execute(*loader) )
	return 0;

    self->addObject( fltset );
    return fltset;
}


EM::FaultSetManager& EM::FltSetMan()
{
    static PtrMan<EM::FaultSetManager> man = new EM::FaultSetManager;
    return *man;
}


