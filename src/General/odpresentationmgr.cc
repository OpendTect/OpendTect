/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : Satyaki Maitra
 * DATE     : August 2016
-*/


#include "odpresentationmgr.h"
#include "keystrs.h"
#include "iopar.h"

static OD::PresentationManager* prman_ = 0;
OD::PresentationManager& OD::PrMan()
{
    if ( !prman_ )
	prman_ = new OD::PresentationManager;
    return *prman_;
}


OD::PresentationManager::PresentationManager()
{
    syncAllViewerTypes();
}


OD::VwrTypePresentationMgr* OD::PresentationManager::getViewerTypeMgr(
	OD::ViewerTypeID vwrtypeid )
{
    const int idx = syncInfoIdx( vwrtypeid );
    if ( idx<0 )
	return 0;

    return vwrtypemanagers_[idx];
}


void OD::PresentationManager::request( OD::ViewerID vwrid,
				     OD::PresentationRequestType req,
				     const IOPar& prinfopar )
{
    for ( int idx=0; idx<vwrtypemanagers_.size(); idx++ )
    {
	OD::VwrTypePresentationMgr* vwrtypemgr = vwrtypemanagers_[idx];
	const SyncInfo& syninfo = vwrtypesyncinfos_[idx];
	const OD::ViewerTypeID vwrtypeid = syninfo.vwrtypeid_;
	if ( !areViewerTypesSynced(vwrid.viewerTypeID(),vwrtypeid) )
	    continue;

	vwrtypemgr->request(
		req, prinfopar, vwrtypeid==vwrid.viewerTypeID()
				    ? vwrid.viewerObjID()
				    : OD::ViewerObjID::get(-1) );
    }
}


int OD::PresentationManager::syncInfoIdx( OD::ViewerTypeID vwrtypeid ) const
{
    for ( int idx=0; idx<vwrtypesyncinfos_.size(); idx++ )
    {
	if ( vwrtypesyncinfos_[idx].vwrtypeid_ == vwrtypeid )
	    return idx;
    }

    return -1;
}


bool OD::PresentationManager::areViewerTypesSynced(
	OD::ViewerTypeID vwr1typeid, OD::ViewerTypeID vwr2typeid ) const
{
    const int vwr1typeidx = syncInfoIdx( vwr1typeid );
    const int vwr2typeidx = syncInfoIdx( vwr2typeid );
    if ( vwr1typeidx<0 || vwr2typeidx<0 )
	return false;

    return vwrtypesyncinfos_[vwr1typeidx].issynced_ &&
	   vwrtypesyncinfos_[vwr2typeidx].issynced_;
}


void OD::PresentationManager::syncAllViewerTypes()
{
    for ( int idx=0; idx<vwrtypesyncinfos_.size(); idx++ )
	vwrtypesyncinfos_[idx].issynced_ = true;
}


void OD::PresentationManager::addViewerTypeManager(
	OD::VwrTypePresentationMgr* vtm )
{
    vwrtypemanagers_ += vtm;
    vwrtypesyncinfos_ += SyncInfo( vtm->viewerTypeID(), true );
}


OD::PresentationManagedViewer::PresentationManagedViewer()
    : ObjAdded(this)
    , ObjOrphaned(this)
    , UnsavedObjLastCall(this)
    , ShowRequested(this)
    , HideRequested(this)
    , VanishRequested(this)
    , viewerobjid_(OD::ViewerObjID::get(-1))
{
}


OD::PresentationManagedViewer::~PresentationManagedViewer()
{
    detachAllNotifiers();
}


void OD::VwrTypePresentationMgr::request( OD::PresentationRequestType req,
					const IOPar& prinfopar,
					OD::ViewerObjID skipvwrid )
{
    for ( int idx=0; idx<viewers_.size(); idx++ )
    {
	OD::PresentationManagedViewer* vwr = viewers_[idx];
	if ( vwr->viewerObjID()==skipvwrid )
	    continue;

	switch ( req )
	{
	    case OD::Add:
		vwr->ObjAdded.trigger( prinfopar );
		break;
	    case OD::Vanish:
		vwr->VanishRequested.trigger( prinfopar );
		break;
	    case OD::Show:
		vwr->ShowRequested.trigger( prinfopar );
		break;
	    case OD::Hide:
		vwr->HideRequested.trigger( prinfopar );
		break;
	}
    }
}


void OD::ObjPresentationInfo::fillPar( IOPar& par ) const
{
    par.set( sKey::Type(), objtypekey_ );
    par.set( IOPar::compKey(sKey::Stored(),sKey::ID()), storedid_ );
}


bool OD::ObjPresentationInfo::usePar( const IOPar& par )
{
    if ( !par.get(sKey::Type(),objtypekey_) )
	return false;

    return par.get( IOPar::compKey(sKey::Stored(),sKey::ID()), storedid_ );
}


static OD::ObjPresentationInfoFactory* dispinfofac_ = 0;

OD::ObjPresentationInfoFactory& OD::PRIFac()
{
    if ( !dispinfofac_ )
	dispinfofac_ = new OD::ObjPresentationInfoFactory;
    return *dispinfofac_;
}


void OD::ObjPresentationInfoFactory::addCreateFunc( CreateFunc crfn,
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


OD::ObjPresentationInfo* OD::ObjPresentationInfoFactory::create(
	const IOPar& par )
{
    BufferString keystr;
    if ( !par.get(sKey::Type(),keystr) )
	return 0;

    const int keyidx = keys_.indexOf( keystr );
    if ( keyidx < 0 )
	return 0;

    return (*createfuncs_[keyidx])( par );
}
