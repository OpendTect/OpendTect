/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : Satyaki Maitra
 * DATE     : August 2016
-*/


#include "odpresentationmgr.h"
#include "keystrs.h"
#include "iopar.h"

static ODPresentationManager* prman_ = 0;
ODPresentationManager& ODPrMan()
{
    if ( !prman_ )
	prman_ = new ODPresentationManager;
    return *prman_;
}


ODPresentationManager::ODPresentationManager()
{
    syncAllViewerTypes();
}


ODVwrTypePresentationMgr* ODPresentationManager::getViewerTypeMgr(int vwrtypeid)
{
    const int idx = syncInfoIdx( vwrtypeid );
    if ( idx<0 )
	return 0;

    return vwrtypemanagers_[idx];
}


void ODPresentationManager::request( int curdomainid, RequestType req,
				     const IOPar& disppar )
{
    for ( int idx=0; idx<vwrtypemanagers_.size(); idx++ )
    {
	ODVwrTypePresentationMgr* domainmgr = vwrtypemanagers_[idx];
	const SyncInfo& syninfo = vwrtypesyncinfos_[idx];
	const int vwrtypeid = syninfo.vwrtypeid_;
	if ( vwrtypeid==curdomainid ||
	     !areViewerTypesSynced(curdomainid,vwrtypeid) )
	    continue;

	switch ( req )
	{
	    case Add:
		domainmgr->ObjAdded.trigger( disppar );
		break;
	    case Vanish:
		domainmgr->VanishRequested.trigger( disppar );
		break;
	    case Show:
		domainmgr->ShowRequested.trigger( disppar );
		break;
	    case Hide:
		domainmgr->HideRequested.trigger( disppar );
		break;
	}
    }
}


int ODPresentationManager::syncInfoIdx( int vwrtypeid ) const
{
    for ( int idx=0; idx<vwrtypesyncinfos_.size(); idx++ )
    {
	if ( vwrtypesyncinfos_[idx].vwrtypeid_ == vwrtypeid )
	    return idx;
    }

    return -1;
}


bool ODPresentationManager::areViewerTypesSynced( int domain1id,
					      int domain2id ) const
{
    const int domain1idx = syncInfoIdx( domain1id );
    const int domain2idx = syncInfoIdx( domain2id );
    if ( domain1idx<0 || domain2idx<0 )
	return false;

    return vwrtypesyncinfos_[domain1idx].issynced_ &&
	   vwrtypesyncinfos_[domain2idx].issynced_;
}


void ODPresentationManager::syncAllViewerTypes()
{
    for ( int idx=0; idx<vwrtypesyncinfos_.size(); idx++ )
	vwrtypesyncinfos_[idx].issynced_ = true;
}


void ODPresentationManager::addViewerTypeManager( ODVwrTypePresentationMgr* vtm)
{
    vwrtypemanagers_ += vtm;
    vwrtypesyncinfos_ += SyncInfo( vtm->viewerTypeID(), true );
}


ODVwrTypePresentationMgr::ODVwrTypePresentationMgr()
    : ObjAdded(this)
    , ObjOrphaned(this)
    , UnsavedObjLastCall(this)
    , ShowRequested(this)
    , HideRequested(this)
    , VanishRequested(this)
{
}


void ObjPresentationInfo::fillPar( IOPar& par ) const
{
    par.set( sKey::Type(), objtypekey_ );
    par.set( IOPar::compKey(sKey::Stored(),sKey::ID()), storedid_ );
}


bool ObjPresentationInfo::usePar( const IOPar& par )
{
    if ( !par.get(sKey::Type(),objtypekey_) )
	return false;

    return par.get( IOPar::compKey(sKey::Stored(),sKey::ID()), storedid_ );
}


static ObjPresentationInfoFactory* dispinfofac_ = 0;

ObjPresentationInfoFactory& ODIFac()
{
    if ( !dispinfofac_ )
	dispinfofac_ = new ObjPresentationInfoFactory;
    return *dispinfofac_;
}


void ObjPresentationInfoFactory::addCreateFunc( CreateFunc crfn,
						const char* key )
{
    const int keyidx = keys_.indexOf( key );
    if ( keyidx >= 0 )
    {
	createfuncs_[keyidx] = crfn;
	return;
    }

    createfuncs_ += crfn;
    keys_.add( key );
}


ObjPresentationInfo* ObjPresentationInfoFactory::create( const IOPar& par )
{
    BufferString keystr;
    if ( !par.get(sKey::Type(),keystr) )
	return 0;

    const int keyidx = keys_.indexOf( keystr );
    if ( keyidx < 0 )
	return 0;

    return (*createfuncs_[keyidx])( par );
}
