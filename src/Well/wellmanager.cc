/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : Bert
 * DATE     : Mar 2001 / Mar 2016
-*/


#include "wellmanager.h"
#include "wellreader.h"
#include "wellwriter.h"
#include "welltransl.h"
#include "dbman.h"
#include "ioobj.h"


#define mToWD(cnsttyp,reftyp,var) static_cast<cnsttyp Well::Data reftyp>(var)


mDefineSaveableManagerInstance(Well::Manager);


Well::Manager::Manager()
    : SaveableManager(mIOObjContext(Well),true)
{
    mTriggerInstanceCreatedNotifier();
}


Well::Manager::~Manager()
{
    sendDelNotif();
}


template <class RefManType>
RefManType Well::Manager::doFetch( const ObjID& id, uiRetVal& uirv ) const
{
    mLock4Read();
    Data* wd = const_cast<Data*>( gtData(id) );
    if ( wd )
	return RefManType( wd );		// already loaded

    wd = new Data;
    Reader rdr( id, *wd );
    if ( !rdr.get() )
    {
	uirv = rdr.errMsg();
	return RefManType( 0 );
    }

    mUnlockAllAccess();
    add( *wd, id, 0, true );
    return RefManType( wd );
}


ConstRefMan<Well::Data> Well::Manager::fetch( const ObjID& id ) const
{
    uiRetVal uirv = uiRetVal::OK();
    return doFetch< ConstRefMan<Data> >( id, uirv );
}


ConstRefMan<Well::Data> Well::Manager::fetch( const ObjID& id,
				    uiRetVal& uirv ) const
{
    return doFetch< ConstRefMan<Data> >( id, uirv );
}


RefMan<Well::Data> Well::Manager::fetchForEdit( const ObjID& id )
{
    uiRetVal uirv = uiRetVal::OK();
    return doFetch< RefMan<Data> >( id, uirv );
}


RefMan<Well::Data> Well::Manager::fetchForEdit( const ObjID& id,
				    uiRetVal& uirv )
{
    return doFetch< RefMan<Data> >( id, uirv );
}


Well::Manager::ObjID Well::Manager::getID( const Data& wd ) const
{
    return SaveableManager::getID( wd );
}


uiRetVal Well::Manager::store( const Data& newset,
				  const IOPar* ioobjpars ) const
{
    return SaveableManager::store( newset, ioobjpars );
}


uiRetVal Well::Manager::store( const Data& newset, const ObjID& id,
			      const IOPar* ioobjpars ) const
{
    return SaveableManager::store( newset, id, ioobjpars );
}


uiRetVal Well::Manager::save( const ObjID& id ) const
{
    return SaveableManager::save( id );
}


uiRetVal Well::Manager::save( const Data& set ) const
{
    return SaveableManager::save( set );
}


bool Well::Manager::needsSave( const ObjID& id ) const
{
    return SaveableManager::needsSave( id );
}


bool Well::Manager::needsSave( const Data& wd ) const
{
    return SaveableManager::needsSave( wd );
}


ConstRefMan<Well::Data> Well::Manager::get( IdxType idx ) const
{
    const SharedObject* shobj = gtObj( idx );
    return ConstRefMan<Data>( mToWD(const,*,shobj) );
}


RefMan<Well::Data> Well::Manager::getForEdit( IdxType idx )
{
    SharedObject* shobj = gtObj( idx );
    return RefMan<Data>( mToWD(,*,shobj) );
}


Well::Data* Well::Manager::gtData( const ObjID& id ) const
{
    const IdxType idxof = gtIdx( id );
    if ( idxof < 0 )
	return 0;
    SharedObject* obj = const_cast<SharedObject*>( savers_[idxof]->object() );
    return mToWD(,*,obj);
}


Saveable* Well::Manager::getSaver( const SharedObject& obj ) const
{
    return new Saver( mToWD(const,&,obj) );
}


mDefineInstanceCreatedNotifierAccess(Well::Saver)


Well::Saver::Saver( const Data& wd )
    : Saveable(wd)
{
    mTriggerInstanceCreatedNotifier();
}


Well::Saver::Saver( const Well::Saver& oth )
    : Saveable(oth)
{
    copyClassData( oth );
    mTriggerInstanceCreatedNotifier();
}


Well::Saver::~Saver()
{
    sendDelNotif();
}


mImplMonitorableAssignment(Well::Saver,Saveable)

void Well::Saver::copyClassData( const Well::Saver& oth )
{
}


ConstRefMan<Well::Data> Well::Saver::wellData() const
{
    return ConstRefMan<Data>( static_cast<const Data*>( object() ) );
}


void Well::Saver::setWellData( const Data& wd )
{
    setObject( wd );
}



bool Well::Saver::doStore( const IOObj& ioobj ) const
{
    ConstRefMan<Data> wd = wellData();
    if ( !wd )
	return true;

    Writer wrr( ioobj, *wd );
    if ( !wrr.put() )
    {
	errmsg_ = wrr.errMsg();
	return false;
    }

    return true;
}

