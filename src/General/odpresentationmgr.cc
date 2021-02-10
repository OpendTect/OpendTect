/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : Satyaki Maitra
 * DATE     : August 2016
-*/


#include "odpresentationmgr.h"
#include "keystrs.h"
#include "iopar.h"
#include "survinfo.h"

const char* Presentation::sKeyObj()	{ return "Presentation Obj"; }

Presentation::Manager& OD::PrMan()
{
    mDefineStaticLocalObject( PtrMan<Presentation::Manager>, prman_,
	    = new Presentation::Manager);
    return *prman_;
}


Presentation::ObjInfoFactory& OD::PrIFac()
{
    mDefineStaticLocalObject( PtrMan<Presentation::ObjInfoFactory>, dispinfofac_,
	= new Presentation::ObjInfoFactory );
    return *dispinfofac_;
}


Presentation::Manager::Manager()
{
    syncAllViewerTypes();
}


Presentation::VwrTypeMgr*
    Presentation::Manager::getViewerTypeMgr( ViewerTypeID vwrtypeid )
{
    const int idx = syncInfoIdx( vwrtypeid );
    if ( idx<0 )
	return 0;

    return vwrtypemanagers_[idx];
}


const Presentation::VwrTypeMgr*
    Presentation::Manager::getViewerTypeMgr( ViewerTypeID vwrtypid ) const
{
    const int idx = syncInfoIdx( vwrtypid );
    if ( idx<0 )
	return 0;

    return vwrtypemanagers_[idx];
}


Presentation::ManagedViewer*
    Presentation::Manager::getViewer( ViewerID vwrid )
{
    Presentation::VwrTypeMgr* vwrtypemgr =
	getViewerTypeMgr( vwrid.viewerTypeID() );
    return vwrtypemgr ? vwrtypemgr->getViewer( vwrid.viewerObjID() ) : 0;
}


const Presentation::ManagedViewer*
    Presentation::Manager::getViewer( ViewerID vwrid ) const
{
    const Presentation::VwrTypeMgr* vwrtypemgr =
	getViewerTypeMgr( vwrid.viewerTypeID() );
    return vwrtypemgr ? vwrtypemgr->getViewer( vwrid.viewerObjID() ) : 0;
}


void Presentation::Manager::handleRequest( ViewerID originvwrid,
					   RequestType req,
					   const IOPar& prinfopar )
{
    for ( int idx=0; idx<vwrtypemanagers_.size(); idx++ )
    {
	Presentation::VwrTypeMgr* vwrtypemgr = vwrtypemanagers_[idx];
	const SyncInfo& syninfo = vwrtypesyncinfos_[idx];
	const ViewerTypeID vwrtypeid = syninfo.vwrtypeid_;
	if ( originvwrid.isValid() &&
	     !areViewerTypesSynced(originvwrid.viewerTypeID(),vwrtypeid) )
	    continue;

	vwrtypemgr->handleRequest( originvwrid, req, prinfopar );
    }
}


int Presentation::Manager::syncInfoIdx( ViewerTypeID vwrtypeid ) const
{
    for ( int idx=0; idx<vwrtypesyncinfos_.size(); idx++ )
    {
	if ( vwrtypesyncinfos_[idx].vwrtypeid_ == vwrtypeid )
	    return idx;
    }

    return -1;
}


bool Presentation::Manager::canViewerBeSynced(
	ViewerID vwr1id, ViewerID vwr2id ) const
{
    const Presentation::ManagedViewer* vwr1 = getViewer( vwr1id );
    const Presentation::ManagedViewer* vwr2 = getViewer( vwr2id );
    if ( !vwr1 || !vwr2 )
	return false;

    return vwr1->zDomain().isCompatibleWith( vwr2->zDomain() );
}


bool Presentation::Manager::areViewerTypesSynced(
	ViewerTypeID vwr1typeid, ViewerTypeID vwr2typeid ) const
{
    const int vwr1typeidx = syncInfoIdx( vwr1typeid );
    const int vwr2typeidx = syncInfoIdx( vwr2typeid );
    if ( vwr1typeidx<0 || vwr2typeidx<0 )
	return false;

    return vwrtypesyncinfos_[vwr1typeidx].issynced_ &&
	   vwrtypesyncinfos_[vwr2typeidx].issynced_;
}


void Presentation::Manager::syncAllViewerTypes()
{
    for ( int idx=0; idx<vwrtypesyncinfos_.size(); idx++ )
	vwrtypesyncinfos_[idx].issynced_ = true;
}


void Presentation::Manager::addViewerTypeManager(
	Presentation::VwrTypeMgr* vtm )
{
    vwrtypemanagers_ += vtm;
    vwrtypesyncinfos_ += SyncInfo( vtm->viewerTypeID(), true );
}


Presentation::ManagedViewer::ManagedViewer()
    : ObjAdded(this)
    , ObjOrphaned(this)
    , UnsavedObjLastCall(this)
    , ShowRequested(this)
    , HideRequested(this)
    , VanishRequested(this)
    , datatransform_(0)
    , zdomaininfo_( new ZDomain::Info(SI().zDomain()) )
{
}


Presentation::ManagedViewer::~ManagedViewer()
{
    detachAllNotifiers();
}


void Presentation::ManagedViewer::setZAxisTransform( ZAxisTransform* zat )
{
    datatransform_ = zat;
    if ( zat )
    {
	delete zdomaininfo_;
	zdomaininfo_ = new ZDomain::Info( zat->toZDomainInfo() );
    }
}


Presentation::ManagedViewer*
    Presentation::VwrTypeMgr::getViewer( ViewerObjID vwrid )
{
    for ( int idx=0; idx<viewers_.size(); idx++ )
    {
	Presentation::ManagedViewer* vwr = viewers_[idx];
	if ( vwr->viewerObjID() == vwrid )
	    return vwr;
    }

    return 0;
}


const Presentation::ManagedViewer*
    Presentation::VwrTypeMgr::getViewer( ViewerObjID vwrid ) const
{
    for ( int idx=0; idx<viewers_.size(); idx++ )
    {
	const Presentation::ManagedViewer* vwr = viewers_[idx];
	if ( vwr->viewerObjID() == vwrid )
	    return vwr;
    }

    return 0;
}


void Presentation::VwrTypeMgr::handleRequest( ViewerID originvwrid,
					      RequestType req,
					      const IOPar& prinfopar )
{
    for ( int idx=0; idx<viewers_.size(); idx++ )
    {
	Presentation::ManagedViewer* vwr = viewers_[idx];
	if ( originvwrid.isValid() &&
	     (vwr->viewerID()==originvwrid ||
	     !OD::PrMan().canViewerBeSynced(vwr->viewerID(),originvwrid)) )
	    continue;

	switch ( req )
	{
	    case Add:
		vwr->ObjAdded.trigger( prinfopar );
		break;
	    case Vanish:
		vwr->VanishRequested.trigger( prinfopar );
		break;
	    case Show:
		vwr->ShowRequested.trigger( prinfopar );
		break;
	    case Hide:
		vwr->HideRequested.trigger( prinfopar );
		break;
	}
    }
}


void Presentation::ObjInfo::fillPar( IOPar& par ) const
{
    par.set( IOPar::compKey(Presentation::sKeyObj(),sKey::Type()),
	     objtypekey_ );
    par.set( IOPar::compKey(sKey::Stored(),sKey::ID()), storedid_ );
}


bool Presentation::ObjInfo::usePar( const IOPar& par )
{
    if ( !par.get(IOPar::compKey(Presentation::sKeyObj(),sKey::Type()),
		  objtypekey_) )
	return false;

    return par.get( IOPar::compKey(sKey::Stored(),sKey::ID()), storedid_ );
}


bool Presentation::ObjInfo::isSameObj(
	const  Presentation::ObjInfo& prinfo ) const
{
    return objtypekey_ == prinfo.objTypeKey() &&
	   storedid_.isInvalid() ? true : storedid_==prinfo.storedID();
}


Presentation::ObjInfo* Presentation::ObjInfo::clone() const
{
    IOPar prinfopar;
    fillPar( prinfopar );
    return OD::PrIFac().create( prinfopar );
}


uiString Presentation::ObjInfo::getName() const
{
    return toUiString( storedid_.name() );
}


bool Presentation::ObjInfoSet::isPresent(
	const Presentation::ObjInfo& prinfo ) const
{
    for ( int idx=0; idx<prinfoset_.size(); idx++ )
    {
	if ( prinfoset_[idx]->isSameObj(prinfo) )
	    return true;
    }

    return false;
}


bool Presentation::ObjInfoSet::add( Presentation::ObjInfo* prinfo )
{
    if ( !prinfo || isPresent(*prinfo) )
	return false;

    prinfoset_ += prinfo;
    return true;
}


Presentation::ObjInfo* Presentation::ObjInfoSet::get( int idx )
{
    if ( !prinfoset_.validIdx(idx) )
	return 0;

    return prinfoset_[idx];
}


const Presentation::ObjInfo* Presentation::ObjInfoSet::get( int idx ) const
{
    return const_cast<Presentation::ObjInfoSet*>(this)->get( idx );
}


Presentation::ObjInfo* Presentation::ObjInfoSet::remove( int idx )
{
    if ( !prinfoset_.validIdx(idx) )
	return 0;

    return prinfoset_.removeSingle( idx );
}


void Presentation::ObjInfoFactory::addCreateFunc( CreateFunc crfn,
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


Presentation::ObjInfo* Presentation::ObjInfoFactory::create(
						    const IOPar& par )
{
    BufferString keystr;
    if ( !par.get(IOPar::compKey(Presentation::sKeyObj(),sKey::Type()),
		  keystr) )
	return 0;

    const int keyidx = keys_.indexOf( keystr );
    if ( keyidx < 0 )
	return 0;

    return (*createfuncs_[keyidx])( par );
}
