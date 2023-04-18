/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "emmanager.h"

#include "ctxtioobj.h"
#include "emobject.h"
#include "emhorizon.h"
#include "emsurfacegeometry.h"
#include "emsurfaceiodata.h"
#include "emsurfacetr.h"
#include "emioobjinfo.h"
#include "executor.h"
#include "filepath.h"
#include "iopar.h"
#include "ioman.h"
#include "iostrm.h"
#include "ptrman.h"
#include "selector.h"
#include "stratlevel.h"
#include "keystrs.h"
#include "od_iostream.h"

EM::EMManager& EM::EMM()
{
    mDefineStaticLocalObject( PtrMan<EM::EMManager>, emm, (new EM::EMManager) );
    return *emm;
}

bool EM::canOverwrite( const MultiID& mid )
{
    const PtrMan<IOObj> ioobj = IOM().get( mid );
    if ( !ioobj )
	return true;

    mDynamicCastGet(const IOStream*,iostream,ioobj.ptr())
    const bool res = bool(iostream);
    return res;
}


namespace EM
{
const char* EMManager::displayparameterstr() { return "Display Parameters"; }



mImplFactory1Param( EMObject, EMManager&, EMOF );

EMManager::EMManager()
    : undo_( *new EMUndo() )
    , addRemove( this )
{
    Strat::LevelSet& lvlset = Strat::eLVLS();
    mAttachCB( lvlset.levelToBeRemoved, EMManager::levelToBeRemoved );
}


EMManager::~EMManager()
{
    detachAllNotifiers();
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
	{ pErrMsg( "Not all objects are unreffed" ); }

    addRemove.trigger();
    eraseUndoList();

    undo_.removeAll();
}


void EMManager::eraseUndoList()
{
    deepErase( undolist_ );
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
    BufferString typenm = ioobj.pars().find( sKey::Type() );
    if ( typenm.isEmpty() )
	typenm = ioobj.group();

    const int idx = EMOF().getNames().indexOf( typenm );
    if ( idx<0 )
	return 0;

    return EMOF().getNames()[idx]->buf();
}


ObjectID EMManager::createObject( const char* type, const char* name )
{
    EMObject* object = EMOF().create( type, *this );
    if ( !object )
	return ObjectID::udf();

    CtxtIOObj ctio( object->getIOObjContext() );
    ctio.ctxt_.forread_ = false;
    ctio.ioobj_ = 0;
    ctio.setName( name );
    if ( ctio.fillObj() )
    {
	object->setMultiID( ctio.ioobj_->key() );
	delete ctio.ioobj_;
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


EMObject* EMManager::getObject( const MultiID& mid )
{
    ObjectID id = getObjectID( mid );
    return id.isValid() ? getObject(id) : nullptr;
}


const EMObject* EMManager::getObject( const MultiID& mid ) const
{
    ObjectID id = getObjectID( mid );
    return id.isValid() ? getObject(id) : nullptr;
}


ObjectID EMManager::getObjectID( const MultiID& mid ) const
{
    ObjectID res;
    for ( int idx=0; idx<objects_.size(); idx++ )
    {
	if ( objects_[idx]->multiID()==mid )
	{
	    if ( objects_[idx]->isFullyLoaded() )
		return objects_[idx]->id();

	    if ( !res.isValid() )
		res = objects_[idx]->id(); //Better to return this than nothing
	}
    }

    return res;
}


MultiID EMManager::getMultiID( const ObjectID& oid ) const
{
    const EMObject* emobj = getObject(oid);
    return emobj ? emobj->multiID() : MultiID::udf();
}


bool EMManager::objectExists( const EMObject* obj ) const
{
    return objects_.isPresent(obj);
}


void EMManager::addObject( EMObject* obj )
{
    if ( !obj )
	{ pErrMsg("No object provided!"); return; }

    if ( objects_.isPresent(obj) )
	{ pErrMsg("Adding object twice"); return; }

    objects_ += obj;
    addRemove.trigger();
}


void EMManager::removeObject( const EMObject* obj )
{
   const int idy = undoIndexOf( obj->id() );
   if ( idy>=0 )
	delete undolist_.removeSingle( idy );

    const int idx = objects_.indexOf(obj);
    if ( idx<0 ) return;
    objects_.removeSingle( idx );
    addRemove.trigger();
}


EMObject* EMManager::createTempObject( const char* type )
{
    return EMOF().create( type, *this );
}


ObjectID EMManager::objectID( int idx ) const
{
    return objects_.validIdx(idx) ? objects_[idx]->id() : ObjectID::udf();
}


Executor* EMManager::objectLoader( const TypeSet<MultiID>& mids,
				   const SurfaceIODataSelection* iosel,
				   TypeSet<MultiID>* idstobeloaded,
				   const ZAxisTransform* zatf )
{
    ExecutorGroup* execgrp = mids.size()>1 ? new ExecutorGroup( "Reading" ) :
								nullptr;
    for ( int idx=0; idx<mids.size(); idx++ )
    {
	const ObjectID objid = getObjectID( mids[idx] );
	const EMObject* obj = getObject( objid );
	Executor* loader =
	    obj && obj->isFullyLoaded() ? nullptr :
				    objectLoader( mids[idx], iosel, zatf );
	if ( idstobeloaded && loader )
	    *idstobeloaded += mids[idx];

	if ( execgrp )
	{
	    if ( loader )
	    {
		if ( !execgrp->nrExecutors() )
		    execgrp->setNrDoneText( loader->uiNrDoneText() );
		execgrp->add( loader );
	    }
	}
	else
	{
	    return loader;
	}
    }

    if ( execgrp && !execgrp->nrExecutors() )
	deleteAndNullPtr( execgrp );

    return execgrp;

}


Executor* EMManager::objectLoader( const MultiID& mid,
				   const SurfaceIODataSelection* iosel,
				   const ZAxisTransform* zatf )
{
    const ObjectID id = getObjectID( mid );
    EMObject* obj = getObject( id );

    if ( !obj )
    {
	PtrMan<IOObj> ioobj = IOM().get( mid );
	if ( !ioobj )
	    return nullptr;

	BufferString typenm = ioobj->pars().find( sKey::Type() );
	if ( typenm.isEmpty() )
	    typenm = ioobj->group();

	obj = EMOF().create( typenm, *this );
	if ( !obj )
	    return nullptr;

	obj->setMultiID( mid );
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
		return geom->loader( iosel, zatf );

	    SurfaceIODataSelection newsel( *iosel );
	    newsel.rg.include( hs );
	    return geom->loader( &newsel, zatf );
	}

	return surface->geometry().loader( iosel, zatf );
    }
    else if ( obj )
	return obj->loader();

    return nullptr;
}


EMObject* EMManager::loadIfNotFullyLoaded( const MultiID& mid,
			TaskRunner* taskrunner, const ZAxisTransform* zatf )
{
    EM::ObjectID emid = EM::EMM().getObjectID( mid );
    RefMan<EM::EMObject> emobj = EM::EMM().getObject( emid );

    if ( !emobj || !emobj->isFullyLoaded() )
    {
	PtrMan<Executor> exec = EM::EMM().objectLoader( mid, nullptr, zatf );
	if ( !exec )
	    return nullptr;

	if ( !TaskRunner::execute( taskrunner, *exec ) )
	    return nullptr;

	emid = EM::EMM().getObjectID( mid );
	emobj = EM::EMM().getObject( emid );
    }

    if ( !emobj || !emobj->isFullyLoaded() )
	return nullptr;

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


bool EMManager::readDisplayPars( const MultiID& mid, IOPar& outpar ) const
{
    if( !readParsFromDisplayInfoFile(mid,outpar) )
	return readParsFromGeometryInfoFile( mid, outpar );

    return true;

}


bool EMManager::readParsFromDisplayInfoFile( const MultiID& mid,
					     IOPar& outpar ) const
{
    outpar.setEmpty();

    IOObjInfo ioobjinfo( mid );
    if( !ioobjinfo.isOK() )
	return false;

    const BufferString filenm = Surface::getParFileName( *ioobjinfo.ioObj() );
    FilePath fp( filenm );
    fp.setExtension( "par" );
    od_istream strm( fp );

    if( !strm.isOK() )
	return false;

    return outpar.read( strm, displayparameterstr() );
}


bool EMManager::readParsFromGeometryInfoFile( const MultiID& mid,
					      IOPar& outpar ) const
{
    outpar.setEmpty();
    IOPar* par = IOObjInfo(mid).getPars();
    if( !par )
	return false;

    OD::Color col;
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


bool EMManager::writeDisplayPars( const MultiID& mid,const IOPar& inpar ) const
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


bool EMManager::getSurfaceData( const MultiID& mid, SurfaceIOData& sd,
				uiString& errmsg ) const
{
    EM::IOObjInfo oi( mid );
    return oi.getSurfaceData( sd, errmsg );
}


void EMManager::levelToBeRemoved( CallBacker* cb )
{
    if ( !cb )
	return;

    mCBCapsuleUnpack(Strat::LevelID,lvlid,cb);
    for ( auto* emobj : objects_ )
    {
	mDynamicCastGet(EM::Horizon*,hor,emobj)
	if ( hor && hor->stratLevelID() == lvlid )
	    hor->setNoLevelID();
    }
}


Undo& EMManager::undo( const EM::ObjectID& id )
{
    const int idx = undoIndexOf( id );
    if ( undolist_.validIdx(idx) )
	return undolist_[idx]->undo_;

    EMObjUndo* newemobjundo = new EMObjUndo( id );
    undolist_ += newemobjundo;

    return newemobjundo->undo_;
}


int EMManager::undoIndexOf( const EM::ObjectID& id )
{
    for ( int idx=0; idx<undolist_.size(); idx++ )
    {
	if ( undolist_[idx]->id_ == id )
	    return idx;
    }

    return -1;
}

} // namespace EM
