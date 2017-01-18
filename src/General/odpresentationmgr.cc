/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : Satyaki Maitra
 * DATE     : August 2016
-*/


#include "odpresentationmgr.h"
#include "keystrs.h"
#include "iopar.h"
#include "dbman.h"
#include "survinfo.h"

const char* OD::sKeyPresentationObj()	{ return "Presentation Obj"; }

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


OD::VwrTypePresentationMgr*
    OD::PresentationManager::getViewerTypeMgr( OD::ViewerTypeID vwrtypeid )
{
    const int idx = syncInfoIdx( vwrtypeid );
    if ( idx<0 )
	return 0;

    return vwrtypemanagers_[idx];
}


const OD::VwrTypePresentationMgr*
    OD::PresentationManager::getViewerTypeMgr( OD::ViewerTypeID vwrtypid ) const
{
    const int idx = syncInfoIdx( vwrtypid );
    if ( idx<0 )
	return 0;

    return vwrtypemanagers_[idx];
}


OD::PresentationManagedViewer*
    OD::PresentationManager::getViewer( OD::ViewerID vwrid )
{
    OD::VwrTypePresentationMgr* vwrtypemgr =
	getViewerTypeMgr( vwrid.viewerTypeID() );
    return vwrtypemgr ? vwrtypemgr->getViewer( vwrid.viewerObjID() ) : 0;
}


const OD::PresentationManagedViewer*
    OD::PresentationManager::getViewer( OD::ViewerID vwrid ) const
{
    const OD::VwrTypePresentationMgr* vwrtypemgr =
	getViewerTypeMgr( vwrid.viewerTypeID() );
    return vwrtypemgr ? vwrtypemgr->getViewer( vwrid.viewerObjID() ) : 0;
}


void OD::PresentationManager::request( OD::ViewerID originvwrid,
				     OD::PresentationRequestType req,
				     const IOPar& prinfopar )
{
    for ( int idx=0; idx<vwrtypemanagers_.size(); idx++ )
    {
	OD::VwrTypePresentationMgr* vwrtypemgr = vwrtypemanagers_[idx];
	const SyncInfo& syninfo = vwrtypesyncinfos_[idx];
	const OD::ViewerTypeID vwrtypeid = syninfo.vwrtypeid_;
	if ( originvwrid.isValid() &&
	     !areViewerTypesSynced(originvwrid.viewerTypeID(),vwrtypeid) )
	    continue;

	vwrtypemgr->request( originvwrid, req, prinfopar);
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


bool OD::PresentationManager::canViewerBeSynced(
	OD::ViewerID vwr1id, OD::ViewerID vwr2id ) const
{
    const PresentationManagedViewer* vwr1 = getViewer( vwr1id );
    const PresentationManagedViewer* vwr2 = getViewer( vwr2id );
    if ( !vwr1 || !vwr2 )
	return false;

    return vwr1->zDomain().isCompatibleWith( vwr2->zDomain() );
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
    , datatransform_(0)
    , zdomaininfo_( new ZDomain::Info(SI().zDomain()) )
{
}


OD::PresentationManagedViewer::~PresentationManagedViewer()
{
    detachAllNotifiers();
}


void OD::PresentationManagedViewer::setZAxisTransform( ZAxisTransform* zat )
{
    datatransform_ = zat;
    if ( zat )
    {
	delete zdomaininfo_;
	zdomaininfo_ = new ZDomain::Info( zat->toZDomainInfo() );
    }
}


OD::PresentationManagedViewer*
    OD::VwrTypePresentationMgr::getViewer( OD::ViewerObjID vwrid )
{
    for ( int idx=0; idx<viewers_.size(); idx++ )
    {
	OD::PresentationManagedViewer* vwr = viewers_[idx];
	if ( vwr->viewerObjID() == vwrid )
	    return vwr;
    }

    return 0;
}


const OD::PresentationManagedViewer*
    OD::VwrTypePresentationMgr::getViewer( OD::ViewerObjID vwrid ) const
{
    for ( int idx=0; idx<viewers_.size(); idx++ )
    {
	const OD::PresentationManagedViewer* vwr = viewers_[idx];
	if ( vwr->viewerObjID() == vwrid )
	    return vwr;
    }

    return 0;
}


void OD::VwrTypePresentationMgr::request( OD::ViewerID originvwrid,
					OD::PresentationRequestType req,
					const IOPar& prinfopar )
{
    for ( int idx=0; idx<viewers_.size(); idx++ )
    {
	OD::PresentationManagedViewer* vwr = viewers_[idx];
	if ( originvwrid.isValid() &&
	     (vwr->viewerID()==originvwrid ||
	     !OD::PrMan().canViewerBeSynced(vwr->viewerID(),originvwrid)) )
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
    par.set( IOPar::compKey(OD::sKeyPresentationObj(),sKey::Type()),
	     objtypekey_ );
    par.set( IOPar::compKey(sKey::Stored(),sKey::ID()), storedid_ );
}


bool OD::ObjPresentationInfo::usePar( const IOPar& par )
{
    if ( !par.get(IOPar::compKey(OD::sKeyPresentationObj(),sKey::Type()),
		  objtypekey_) )
	return false;

    return par.get( IOPar::compKey(sKey::Stored(),sKey::ID()), storedid_ );
}


bool OD::ObjPresentationInfo::isSameObj(
	const  OD::ObjPresentationInfo& prinfo ) const
{
    return objtypekey_ == prinfo.objTypeKey() &&
	   storedid_.isInvalid() ? true : storedid_==prinfo.storedID();
}


OD::ObjPresentationInfo* OD::ObjPresentationInfo::clone() const
{
    IOPar prinfopar;
    fillPar( prinfopar );
    return OD::PRIFac().create( prinfopar );
}


uiString OD::ObjPresentationInfo::getName() const
{
    return toUiString( DBM().nameOf(storedid_) );
}


bool OD::ObjPresentationInfoSet::isPresent(
	const OD::ObjPresentationInfo& prinfo ) const
{
    for ( int idx=0; idx<prinfoset_.size(); idx++ )
    {
	if ( prinfoset_[idx]->isSameObj(prinfo) )
	    return true;
    }

    return false;
}


bool OD::ObjPresentationInfoSet::add( OD::ObjPresentationInfo* prinfo )
{
    if ( !prinfo || isPresent(*prinfo) )
	return false;

    prinfoset_ += prinfo;
    return true;
}


OD::ObjPresentationInfo* OD::ObjPresentationInfoSet::get( int idx )
{
    if ( !prinfoset_.validIdx(idx) )
	return 0;

    return prinfoset_[idx];
}


const OD::ObjPresentationInfo* OD::ObjPresentationInfoSet::get( int idx ) const
{
    return const_cast<OD::ObjPresentationInfoSet*>(this)->get( idx );
}


OD::ObjPresentationInfo* OD::ObjPresentationInfoSet::remove( int idx )
{
    if ( !prinfoset_.validIdx(idx) )
	return 0;

    return prinfoset_.removeSingle( idx );
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
    if ( !par.get(IOPar::compKey(OD::sKeyPresentationObj(),sKey::Type()),
		  keystr) )
	return 0;

    const int keyidx = keys_.indexOf( keystr );
    if ( keyidx < 0 )
	return 0;

    return (*createfuncs_[keyidx])( par );
}
