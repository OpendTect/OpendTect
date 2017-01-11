/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : R. Sen
 * DATE     : Jan 2017
-*/


#include "emobjectio.h"

#include "dbman.h"
#include "emmanager.h"
#include "executor.h"
#include "ioobj.h"
#include "iopar.h"
#include "keystrs.h"
#include "task.h"
#include "uistrings.h"

mDefineInstanceCreatedNotifierAccess(EM::ObjectSaver)

namespace EM
{

ObjectLoader::ObjectLoader( const DBKey& key )
{
    dbkeys_ += key;
}


ObjectLoader::ObjectLoader( const DBKeySet& keys )
    : dbkeys_(keys)
{
}


class FSSLoaderExec : public Executor
{ mODTextTranslationClass(FSSLoaderExec)
public:

FSSLoaderExec( FaultStickSetLoader& ldr )
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

    FaultStickSetLoader&	loader_;
    int				curidx_;

};


int FSSLoaderExec::nextStep()
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

    FaultStickSet* fss = new FaultStickSet( EMM() ); fss->setDBKey( key );
    if ( !fss->loader()->execute() )
	return MoreToDo();

    loader_.loadedkeys_ += key;
    EMM().addObject( fss );
    loader_.addObject( fss );
    return MoreToDo();
}


FaultStickSetLoader::FaultStickSetLoader( const DBKey& key )
    : ObjectLoader(key)
{
}


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


//Saver
ObjectSaver::ObjectSaver( const EMObject& emobj )
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


FaultStickSetSaver::FaultStickSetSaver( const EMObject& emobj )
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
    
    fss->setDBKey( key );
    if ( isSave(ioobj) )
    {
	fss->setName( ioobj.name() );
	fss->saveDisplayPars();
    }
    
    return uiRetVal::OK();
}


} // namespace EM
