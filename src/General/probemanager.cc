/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : Satyaki Maitra
 * DATE     : October 2016
-*/


#include "probemanager.h"
#include "probetr.h"
#include "ioman.h"
#include "ioobj.h"


#define mToProbe(cnsttyp,reftyp,var) static_cast<cnsttyp Probe reftyp>(var)


mDefineSaveableManagerInstance(ProbeManager);

ProbeManager::ProbeManager()
    : SaveableManager(mIOObjContext(Probe),false,true)
{
    mTriggerInstanceCreatedNotifier();
}


ProbeManager::~ProbeManager()
{
    sendDelNotif();
}


template <class RefManType>
RefManType ProbeManager::doFetch( const ObjID& id, uiRetVal& uirv ) const
{
    mLock4Read();
    Probe* oldprobe = const_cast<Probe*>( gtProbe(id) );
    if ( oldprobe )
	return RefManType( oldprobe );		// already loaded

    PtrMan<IOObj> ioobj = DBM().get( id );
    if ( !ioobj )
    {
	pErrMsg( "Required ID not in IOM. Probably not OK" );
	return RefManType( 0 );
    }

    uiString errmsg;
    Probe* newprobe = ProbeTranslator::retrieve( ioobj, errmsg );
    if ( !newprobe )
    {
	uirv = errmsg;
	return RefManType( 0 );
    }

    add( *newprobe, id, mAccessLocker(), &ioobj->pars(), true );
    return RefManType( gtProbe(id) );
}



ConstRefMan<Probe> ProbeManager::fetch( const ObjID& id ) const
{
    uiRetVal uirv = uiRetVal::OK();
    return doFetch< ConstRefMan<Probe> >( id, uirv );
}


ConstRefMan<Probe> ProbeManager::fetch( const ObjID& id,
					      uiRetVal& uirv ) const
{
    return doFetch< ConstRefMan<Probe> >( id, uirv );
}


RefMan<Probe> ProbeManager::fetchForEdit( const ObjID& id )
{
    uiRetVal uirv = uiRetVal::OK();
    return doFetch< RefMan<Probe> >( id, uirv );
}


RefMan<Probe> ProbeManager::fetchForEdit( const ObjID& id,
						uiRetVal& uirv )
{
    return doFetch< RefMan<Probe> >( id, uirv );
}


ProbeManager::ObjID ProbeManager::getID( const Probe& ps ) const
{
    return SaveableManager::getID( ps );
}


uiRetVal ProbeManager::store( const Probe& newset,
				  const IOPar* ioobjpars ) const
{
    return SaveableManager::store( newset, ioobjpars );
}


uiRetVal ProbeManager::store( const Probe& newset, const ObjID& id,
			      const IOPar* ioobjpars ) const
{
    return SaveableManager::store( newset, id, ioobjpars );
}


uiRetVal ProbeManager::save( const ObjID& id ) const
{
    return SaveableManager::save( id );
}


uiRetVal ProbeManager::save( const Probe& set ) const
{
    return SaveableManager::save( set );
}


bool ProbeManager::needsSave( const ObjID& id ) const
{
    return SaveableManager::needsSave( id );
}


bool ProbeManager::needsSave( const Probe& ps ) const
{
    return SaveableManager::needsSave( ps );
}


ConstRefMan<Probe> ProbeManager::get( IdxType idx ) const
{
    const SharedObject* shobj = gtObj( idx );
    return ConstRefMan<Probe>( mToProbe(const,*,shobj) );
}


RefMan<Probe> ProbeManager::getForEdit( IdxType idx )
{
    SharedObject* shobj = gtObj( idx );
    return RefMan<Probe>( mToProbe(,*,shobj) );
}


Probe* ProbeManager::gtProbe( const ObjID& id ) const
{
    const IdxType idxof = gtIdx( id );
    if ( idxof < 0 )
	return 0;
    SharedObject* obj = const_cast<SharedObject*>( savers_[idxof]->object() );
    return mToProbe(,*,obj);
}


Saveable* ProbeManager::getSaver( const SharedObject& obj ) const
{
    return new ProbeSaver( mToProbe(const,&,obj) );
}
