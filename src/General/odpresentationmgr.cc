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


ODVwrTypePresentationMgr* ODPresentationManager::getViewerTypeMgr(
	ViewerSubID vwrtypeid )
{
    const int idx = syncInfoIdx( vwrtypeid );
    if ( idx<0 )
	return 0;

    return vwrtypemanagers_[idx];
}


void ODPresentationManager::request(
	ODViewerID vwrid,OD::PresentationRequestType req,const IOPar& prinfopar)
{
    for ( int idx=0; idx<vwrtypemanagers_.size(); idx++ )
    {
	ODVwrTypePresentationMgr* vwrtypemgr = vwrtypemanagers_[idx];
	const SyncInfo& syninfo = vwrtypesyncinfos_[idx];
	const ViewerSubID vwrtypeid = syninfo.vwrtypeid_;
	if ( !areViewerTypesSynced(vwrid.viewerTypeID(),vwrtypeid) )
	    continue;

	vwrtypemgr->request(
		req, prinfopar, vwrtypeid==vwrid.viewerTypeID()
				? vwrid.viewerID() : ViewerSubID::get(-1) );
    }
}


int ODPresentationManager::syncInfoIdx( ViewerSubID vwrtypeid ) const
{
    for ( int idx=0; idx<vwrtypesyncinfos_.size(); idx++ )
    {
	if ( vwrtypesyncinfos_[idx].vwrtypeid_ == vwrtypeid )
	    return idx;
    }

    return -1;
}


bool ODPresentationManager::areViewerTypesSynced( ViewerSubID vwr1typeid,
						  ViewerSubID vwr2typeid ) const
{
    const int vwr1typeidx = syncInfoIdx( vwr1typeid );
    const int vwr2typeidx = syncInfoIdx( vwr2typeid );
    if ( vwr1typeidx<0 || vwr2typeidx<0 )
	return false;

    return vwrtypesyncinfos_[vwr1typeidx].issynced_ &&
	   vwrtypesyncinfos_[vwr2typeidx].issynced_;
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


PresentationManagedViewer::PresentationManagedViewer()
    : ObjAdded(this)
    , ObjOrphaned(this)
    , UnsavedObjLastCall(this)
    , ShowRequested(this)
    , HideRequested(this)
    , VanishRequested(this)
    , viewerid_(ViewerSubID::get(-1))
{
}


PresentationManagedViewer::~PresentationManagedViewer()
{
    detachAllNotifiers();
}


void ODVwrTypePresentationMgr::request(
	OD::PresentationRequestType req, const IOPar& prinfopar,
	ViewerSubID skipvwrid )
{
    for ( int idx=0; idx<viewers_.size(); idx++ )
    {
	PresentationManagedViewer* vwr = viewers_[idx];
	if ( vwr->viewerID()==skipvwrid )
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
