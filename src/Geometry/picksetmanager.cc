/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : Bert
 * DATE     : Mar 2001 / Mar 2016
-*/


#include "picksetmanager.h"
#include "picksetio.h"
#include "picksettr.h"
#include "picksetchangerecorder.h"
#include "autosaver.h"
#include "ioman.h"
#include "ioobj.h"


#define mToPS(cnsttyp,reftyp,var) static_cast<cnsttyp Pick::Set reftyp>(var)


mDefineSaveableManagerInstance(Pick::SetManager);


Pick::SetManager::SetManager()
    : SaveableManager(mIOObjContext(PickSet),true)
{
    mTriggerInstanceCreatedNotifier();
}


Pick::SetManager::~SetManager()
{
    sendDelNotif();
}


template <class RefManType,class SetType>
RefManType Pick::SetManager::doFetch( const ObjID& id, uiRetVal& uirv,
				      const char* cat ) const
{
    mLock4Read();
    SetType* ps = gtSet( id );
    if ( ps )
	return RefManType( ps );		// already loaded

    SetLoader loader( id );
    loader.setCategory( cat );
    mUnlockAllAccess();
    if ( loader.load() )
    {
	mReLock();
	return RefManType( gtSet(id) );		// now loaded
    }

    uirv = loader.errMsgs();
    return RefManType( 0 );
}


ConstRefMan<Pick::Set> Pick::SetManager::fetch( const ObjID& id ) const
{
    uiRetVal msg = uiRetVal::OK();
    return doFetch<ConstRefMan<Set>,const Set>( id, msg );
}


ConstRefMan<Pick::Set> Pick::SetManager::fetch( const ObjID& id,
				    uiRetVal& uirv, const char* cat ) const
{
    return doFetch<ConstRefMan<Set>,const Set>( id, uirv, cat );
}


RefMan<Pick::Set> Pick::SetManager::fetchForEdit( const ObjID& id )
{
    uiRetVal msg = uiRetVal::OK();
    return doFetch<RefMan<Set>,Set>( id, msg );
}


RefMan<Pick::Set> Pick::SetManager::fetchForEdit( const ObjID& id,
				    uiRetVal& uirv, const char* cat )
{
    return doFetch<RefMan<Set>,Set>( id, uirv, cat );
}


Pick::SetManager::ObjID Pick::SetManager::getID( const Set& ps ) const
{
    return SaveableManager::getID( ps );
}


uiRetVal Pick::SetManager::store( const Set& newset,
				  const IOPar* ioobjpars ) const
{
    return SaveableManager::store( newset, ioobjpars );
}


uiRetVal Pick::SetManager::store( const Set& newset, const ObjID& id,
			      const IOPar* ioobjpars ) const
{
    return SaveableManager::store( newset, id, ioobjpars );
}


uiRetVal Pick::SetManager::save( const ObjID& id ) const
{
    return SaveableManager::save( id );
}


uiRetVal Pick::SetManager::save( const Set& set ) const
{
    return SaveableManager::save( set );
}


bool Pick::SetManager::needsSave( const ObjID& id ) const
{
    return SaveableManager::needsSave( id );
}


bool Pick::SetManager::needsSave( const Set& ps ) const
{
    return SaveableManager::needsSave( ps );
}


bool Pick::SetManager::isPolygon( const ObjID& id ) const
{
    if ( id.isUdf() )
	return false;

    mLock4Read();
    const IdxType idx = gtIdx( id );
    if ( idx >= 0 )
    {
	const Pick::Set* ps = mToPS(const,*,savers_[idx]->object());
	if ( ps )
	    return ps->isPolygon();
    }
    mUnlockAllAccess();

    PtrMan<IOObj> ioobj = IOM().get( id );
    return ioobj ? PickSetTranslator::isPolygon( *ioobj ) : false;
}


bool Pick::SetManager::hasCategory( const ObjID& id, const char* cat ) const
{
    if ( id.isUdf() )
	return false;

    mLock4Read();
    const IdxType idx = gtIdx( id );
    if ( idx >= 0 )
    {
	const Pick::Set* ps = mToPS(const,*,savers_[idx]->object());
	if ( ps )
	    return FixedString(ps->category()) == cat;
    }
    mUnlockAllAccess();

    const bool defhascat = !cat || !*cat;
    PtrMan<IOObj> ioobj = IOM().get( id );
    return !ioobj ? defhascat : PickSetTranslator::getCategory(*ioobj) == cat;
}


ConstRefMan<Pick::Set> Pick::SetManager::get( IdxType idx ) const
{
    const SharedObject* shobj = gtObj( idx );
    return ConstRefMan<Pick::Set>( mToPS(const,*,shobj) );
}


RefMan<Pick::Set> Pick::SetManager::getForEdit( IdxType idx )
{
    SharedObject* shobj = gtObj( idx );
    return RefMan<Pick::Set>( mToPS(,*,shobj) );
}


Pick::Set* Pick::SetManager::gtSet( const ObjID& id ) const
{
    const IdxType idxof = gtIdx( id );
    if ( idxof < 0 )
	return 0;
    SharedObject* obj = const_cast<SharedObject*>( savers_[idxof]->object() );
    return mToPS(,*,obj);
}


Saveable* Pick::SetManager::getSaver( const SharedObject& obj ) const
{
    return new Pick::SetSaver( mToPS(const,&,obj) );
}


ChangeRecorder* Pick::SetManager::getChangeRecorder(
				    const SharedObject& obj ) const
{
    return new Pick::SetChangeRecorder( mToPS(const,&,obj) );
}
