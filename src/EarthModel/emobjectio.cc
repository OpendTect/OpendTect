/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : R. Sen
 * DATE     : Jan 2017
-*/


#include "emobjectio.h"

#include "dbman.h"
#include "emmanager.h"
#include "emfaultstickset.h"
#include "emfault3d.h"
#include "executor.h"
#include "ioobj.h"
#include "iopar.h"
#include "keystrs.h"
#include "task.h"
#include "uistrings.h"

mDefineInstanceCreatedNotifierAccess(EM::ObjectSaver)
mImplFactory1Param(EM::ObjectLoader,const DBKeySet&,EM::ObjectLoader::factory)
mImplFactory1Param(EM::ObjectSaver,const SharedObject&,EM::ObjectSaver::factory)

namespace EM
{

ObjectLoader::ObjectLoader( const DBKeySet& keys )
    : dbkeys_(keys)
{
}


class FSSLoaderExec : public ExecutorGroup
{ mODTextTranslationClass(FSSLoaderExec)
public:

FSSLoaderExec( FaultStickSetLoader& ldr )
    : ExecutorGroup("FaultStickSet Loader")
    , loader_(ldr)
    , curidx_(0)
    , totnr_(-1)
    , nrdone_(0)
{
    loader_.notloadedkeys_.setEmpty();
    init();
}

protected:

void init()
{
    for ( int idx=0; idx<loader_.tobeLodedKeys().size(); idx++ )
    {
	const DBKey key = loader_.tobeLodedKeys()[idx];
	PtrMan<IOObj> ioobj = DBM().get( key );
	if ( !ioobj )
	{
	    pErrMsg( "Required ID not in IOM. Probably not OK" );
	    continue;
	}

	BufferString typenm = ioobj->pars().find( sKey::Type() );
	if ( typenm.isEmpty() )
	    typenm = ioobj->group();

	FaultStickSet* fss = new FaultStickSet( ioobj->name() );
	fss->ref();
	fss->setDBKey( key );
	add( fss->loader() );
	fltstcksets_.add( fss );
    }

}

int nextStep()
{
    const int ret = ExecutorGroup::nextStep();
    if (  ret == ErrorOccurred() )
    {
	loader_.notloadedkeys_ += fltstcksets_[currentexec_]->dbKey();
	nrdone_++;
	if( goToNextExecutor() )
	    return MoreToDo();
    }
    else if ( ret == Finished() )
    {
	finishWork();
	return Finished();
    }
   
    nrdone_++;
    return ret;
}

void finishWork()
{
    for ( int idx=0; idx<fltstcksets_.size(); idx++ )
    {
	FaultStickSet* fss = fltstcksets_[idx];
	const DBKey& dbkey = fss->dbKey();
	if ( loader_.notloadedkeys_.isPresent(dbkey) )
	    continue;

	FSSMan().addObject( fss );
	loader_.addObject( fss );
    }
}

public:

od_int64 totalNr() const
{
    return executors_.size() ? ( totnr_ + ExecutorGroup::totalNr() ) : totnr_;
}


od_int64 nrDone() const
{
    return executors_.size() ? ( nrdone_ + ExecutorGroup::nrDone() ) : nrdone_;
}

virtual uiString message() const
{
    return uiStrings::phrLoading( uiStrings::sFaultStickSet() );
}

virtual uiString nrDoneText() const
{
    return tr("FaultStickSets loaded");
}

protected:

    FaultStickSetLoader&	loader_;
    int				curidx_;
    ObjectSet<FaultStickSet>	fltstcksets_;
    od_int64			nrdone_;
    od_int64			totnr_;
};


FaultStickSetLoader::FaultStickSetLoader( const DBKeySet& keys )
    : ObjectLoader(keys)
{
}


bool FaultStickSetLoader::load( TaskRunner* tskr )
{
    PtrMan<Executor> exec = getLoader();
    TaskRunner::execute( tskr, *exec );
    return allOK();
}


Executor* FaultStickSetLoader::getLoader() const
{
    FaultStickSetLoader* _this = const_cast<FaultStickSetLoader*>( this );
    return new FSSLoaderExec( *_this );
}


class FLT3DLoaderExec : public Executor
{ mODTextTranslationClass(FLT3DLoaderExec)
public:

FLT3DLoaderExec( Fault3DLoader& ldr )
    : Executor("FaultStickSet Loader")
    , loader_(ldr)
    , curidx_(-1)
{
    loader_.loadedkeys_.setEmpty();
}

virtual od_int64 nrDone() const
{
    return curidx_;
}

virtual od_int64 totalNr() const
{
    return loader_.tobeLodedKeys().size();
}

virtual uiString message() const
{
    return uiStrings::phrLoading( uiStrings::sPickSet() );
}

virtual uiString nrDoneText() const
{
    return tr("FaultStickSets loaded");
}

    virtual int		nextStep();

    Fault3DLoader&	loader_;
    int			curidx_;

};


int FLT3DLoaderExec::nextStep()
{
    curidx_++;
    if ( curidx_ >= loader_.tobeLodedKeys().size() )
	return Finished();
   
    const DBKey key = loader_.tobeLodedKeys()[curidx_];
    PtrMan<IOObj> ioobj = DBM().get( key );
    if ( !ioobj )
    {
	pErrMsg( "Required ID not in IOM. Probably not OK" );
	return MoreToDo();
    }

    BufferString typenm = ioobj->pars().find( sKey::Type() );
    if ( typenm.isEmpty() )
	typenm = ioobj->group();

    Fault3D* flt = new Fault3D( ioobj->name() ); flt->setDBKey( key );
    if ( !flt->loader()->execute() )
	return MoreToDo();

    loader_.loadedkeys_ += key;
    Flt3DMan().addObject( flt );
    loader_.addObject( flt );
    return MoreToDo();
}


Fault3DLoader::Fault3DLoader( const DBKeySet& keys )
    : ObjectLoader(keys)
{
}


bool Fault3DLoader::load( TaskRunner* tskr )
{
    PtrMan<Executor> exec = getLoader();
    TaskRunner::execute( tskr, *exec );
    return allOK();
}


Executor* Fault3DLoader::getLoader() const
{
    Fault3DLoader* _this = const_cast<Fault3DLoader*>( this );
    return new FLT3DLoaderExec( *_this );
}


//Saver
ObjectSaver::ObjectSaver( const SharedObject& emobj )
    : Saveable(emobj)
{
    mTriggerInstanceCreatedNotifier();
}


ObjectSaver::ObjectSaver( const ObjectSaver& oth )
    : Saveable(oth)
{
    copyClassData( oth );
    mTriggerInstanceCreatedNotifier();
}


ObjectSaver::~ObjectSaver()
{
    sendDelNotif();
}


mImplMonitorableAssignment(ObjectSaver,Saveable)

void ObjectSaver::copyClassData( const ObjectSaver& oth )
{
}


ConstRefMan<EMObject> ObjectSaver::emObject() const
{
    return ConstRefMan<EMObject>( static_cast<const EMObject*>( object() ) );
}


void ObjectSaver::setEMObject( const EMObject& obj )
{
    setObject( obj );
}


uiRetVal ObjectSaver::doStore( const IOObj& ioobj ) const
{
    return uiRetVal::OK();
}


FaultStickSetSaver::FaultStickSetSaver( const SharedObject& emobj )
    : ObjectSaver(emobj)
{}


FaultStickSetSaver::~FaultStickSetSaver()
{}


uiRetVal FaultStickSetSaver::doStore( const IOObj& ioobj ) const
{
    uiRetVal uirv;
    ConstRefMan<EMObject> emobj = emObject();
    if ( !emobj )
	return uiRetVal::OK();

    SharedObject* copiedemobj = emobj->clone();
    mDynamicCastGet(FaultStickSet*,fss,copiedemobj)
    if ( !fss )
	return uiRetVal::OK();
    const DBKey key = ioobj.key();
    Executor* exec = fss->geometry().saver( 0, &key );
    if ( exec && !exec->execute() )
	return exec->errorWithDetails();
    
    if ( isSave(ioobj) )
    {
	emobj.getNonConstPtr()->setName( ioobj.name() );
	emobj.getNonConstPtr()->setDBKey( key );
	fss->saveDisplayPars();
    }
    
    return uiRetVal::OK();
}


//Fault3D
Fault3DSaver::Fault3DSaver( const SharedObject& emobj )
    : ObjectSaver(emobj)
{}


Fault3DSaver::~Fault3DSaver()
{}


uiRetVal Fault3DSaver::doStore( const IOObj& ioobj ) const
{
    uiRetVal uirv;
    ConstRefMan<EMObject> emobj = emObject();
    if ( !emobj )
	return uiRetVal::OK();

    SharedObject* copiedemobj = emobj->clone();
    mDynamicCastGet(Fault3D*,flt3d,copiedemobj)
    if ( !flt3d )
	return uiRetVal::OK();
    const DBKey key = ioobj.key();
    Executor* exec = flt3d->geometry().saver( 0, &key );
    if ( exec && !exec->execute() )
	return exec->errorWithDetails();
    
    flt3d->setDBKey( key );
    if ( isSave(ioobj) )
    {
	flt3d->setName( ioobj.name() );
	flt3d->saveDisplayPars();
    }
    
    return uiRetVal::OK();
}

} // namespace EM
