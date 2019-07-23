/*+
___________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Satyaki Maitra
 Date:		September 2016
___________________________________________________________________

-*/

#include "atomic.h"
#include "probe.h"
#include "probeimpl.h"
#include "probetr.h"
#include "iopar.h"
#include "keystrs.h"
#include "survinfo.h"
#include "survgeom2d.h"
#include "zdomain.h"


const char* ProbeLayer::sKeyLayerType() { return "Probe Layer Type"; }
const char* ProbeLayer::sKeyLayer()	{ return "Probe Layer"; }

ProbeLayer::ID ProbeLayer::getNewID()
{
    static Threads::Atomic<ProbeLayer::IDType> layid_;
    return ProbeLayer::ID( layid_++ );
}


ProbeLayer::ProbeLayer()
    : id_(getNewID())
    , probe_(0)
{
}


ProbeLayer::ProbeLayer( const ProbeLayer& oth )
    : SharedObject(oth)
    , id_(oth.id_)
    , probe_(0)
{
    copyClassData( oth );
}


mImplMonitorableAssignment( ProbeLayer, SharedObject )


void ProbeLayer::copyClassData( const ProbeLayer& oth )
{
    probe_ = oth.probe_;
    const_cast<ID&>(id_) = getNewID();
}


Monitorable::ChangeType ProbeLayer::compareClassData(
					const ProbeLayer& oth ) const
{
    mDeliverYesNoMonitorableCompare( (!probe_ && !oth.probe_)
	    || (probe_ && oth.probe_ && *probe_ == *oth.probe_) );
}


const Probe* ProbeLayer::getProbe() const
{
    mLock4Read();
    return probe_.ptr();
}


void ProbeLayer::setProbe( const Probe* newparent )
{
    mLock4Write();
    probe_ = newparent;
    mSendEntireObjChgNotif();
}


ProbeLayer::ID ProbeLayer::getID() const
{
    mLock4Read();
    return id_;
}


void ProbeLayer::fillPar( IOPar& par ) const
{
    mLock4Read();
    par.set( sKeyLayerType(), layerType() );
}


void ProbeLayer::usePar( const IOPar& )
{
}


ProbeLayerFactory& PrLayFac()
{
    mDefineStaticLocalObject(ProbeLayerFactory,probelayfac_,);
    return probelayfac_;
}


void ProbeLayerFactory::addCreateFunc( CreateFunc crfn, const char* key )
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


ProbeLayer* ProbeLayerFactory::create( const IOPar& par )
{
    BufferString keystr;
    if ( !par.get(ProbeLayer::sKeyLayerType(),keystr) )
	return 0;

    const int keyidx = keys_.indexOf( keystr );
    if ( keyidx < 0 )
	return 0;

    return (*createfuncs_[keyidx])( par );
}



ProbeFactory& ProbeFac()
{
    mDefineStaticLocalObject(ProbeFactory,probefac_,);
    return probefac_;
}


const char* Probe::sProbeType()		{ return "ProbeType"; }

mDefineInstanceCreatedNotifierAccess( Probe )
static Threads::Atomic<int> curprobeid_( 0 );

Probe::Probe()
    : SharedObject()
{
    name_ = BufferString( "Probe ", ++curprobeid_ );
    mTriggerInstanceCreatedNotifier();
}


Probe::Probe( const Probe& oth )
    : SharedObject(oth)
{
    name_ = BufferString( "Probe ", ++curprobeid_ );
    mTriggerInstanceCreatedNotifier();
}


Probe::~Probe()
{
    sendDelNotif();
}


mImplMonitorableAssignment( Probe, SharedObject )

void Probe::copyClassData( const Probe& oth )
{
    probepos_ = oth.probepos_;

    deepUnRef( layers_ );
    for ( int idx=0; idx<oth.layers_.size(); idx++ )
	layers_ += oth.layers_[idx]->clone();
}


Monitorable::ChangeType Probe::compareClassData( const Probe& oth ) const
{
    if ( probepos_ != oth.probepos_ || layers_.size() != oth.layers_.size() )
	return cEntireObjectChange();

    for ( int idx=0; idx<layers_.size(); idx++ )
	if ( *layers_[idx] != *oth.layers_[idx] )
	    return cEntireObjectChange();

    return cNoChange();
}


void Probe::fillPar( IOPar& par ) const
{
    mLock4Read();

    par.set( sProbeType(), type() );
    probepos_.fillPar( par );

    for ( int idx=0; idx<layers_.size(); idx++ )
    {
	IOPar layerpar;
	layers_[idx]->fillPar( layerpar );
	par.mergeComp( layerpar, IOPar::compKey(ProbeLayer::sKeyLayer(),idx) );
    }
}


bool Probe::usePar( const IOPar& par )
{
    mLock4Write();
    probepos_.usePar( par );
    mSendEntireObjChgNotif();
    return true;
}


int Probe::nrLayers() const
{
    mLock4Read();
    return layers_.size();
}


void Probe::addLayer( ProbeLayer* layer )
{
    mLock4Read();

    if ( layers_.isPresent(layer) )
	return;

    if ( !mLock2Write() )
    {
	if ( layers_.isPresent(layer) )
	    return;
    }

    layer->setProbe( this );
    layers_ += layer;
    mSendChgNotif( cLayerAdd(), layers_.size()-1 );
}



ProbeLayer* Probe::removeLayer( ProbeLayer::ID id )
{
    mLock4Read();

    ProbeLayer* lay = getLayer( id );
    if ( !lay )
	return 0;

    if ( !mLock2Write() )
    {
	lay = getLayer( id );
	if ( !lay )
	    return 0;
    }

    int probeidx = layers_.indexOf( lay );
    if ( probeidx<0 )
	return 0;

    mSendChgNotif( cLayerToBeRemoved(), probeidx );
    mReLock();

    probeidx = layers_.indexOf( lay );
    if ( probeidx<0 )
	return 0;

    return layers_.removeSingle( probeidx );
}


const ProbeLayer* Probe::getLayerByIdx( int idx ) const
{
    mLock4Read();
    return layers_.validIdx(idx) ? layers_[idx] : 0;
}


ProbeLayer* Probe::getLayerByIdx( int idx )
{
    mLock4Read();
    return layers_.validIdx(idx) ? layers_[idx] : 0;
}


const ProbeLayer* Probe::getLayer( ProbeLayer::ID id ) const
{
    return const_cast<Probe*>(this)->getLayer( id );
}


ProbeLayer* Probe::getLayer( ProbeLayer::ID id )
{
    mLock4Read();

    for ( int idx=0; idx<layers_.size(); idx++ )
    {
	if ( layers_[idx]->getID() == id )
	    return layers_[idx];
    }

    return 0;
}


void Probe::setPos( const TrcKeyZSampling& newpos )
{
    mLock4Read();

    if ( probepos_ == newpos )
	return;

    if ( !mLock2Write() && probepos_ == newpos )
	return;

    probepos_ = newpos;

    for ( int idx=0; idx<layers_.size(); idx++ )
	layers_[idx]->invalidateData();

    mSendChgNotif( cPositionChange(), cEntireObjectChgID() );
}


uiWord Probe::mkDispNm( const uiWord& add ) const
{
    return usrType().constructWordWith( toUiString("[%1]").arg(add), false );
}


void ProbeFactory::addCreateFunc( CreateFunc crfn, const char* key )
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


Probe* ProbeFactory::create( const IOPar& par )
{
    BufferString keystr;
    if ( !par.get(Probe::sProbeType(),keystr) )
	return 0;

    const int keyidx = keys_.indexOf( keystr );
    if ( keyidx < 0 )
	return 0;

    return (*createfuncs_[keyidx])( par );
}


ProbeSaver::ProbeSaver( const Probe& probe )
    : Saveable(probe)
{
    mTriggerInstanceCreatedNotifier();
}


ProbeSaver::ProbeSaver( const ProbeSaver& oth )
    : Saveable(oth)
{
    copyClassData( oth );
    mTriggerInstanceCreatedNotifier();
}


ProbeSaver::~ProbeSaver()
{
    sendDelNotif();
}

mImplMonitorableAssignmentWithNoMembers(ProbeSaver,Saveable)


uiRetVal ProbeSaver::doStore( const IOObj& ioobj,
			      const TaskRunnerProvider& ) const
{
    uiRetVal uirv;
    mDynamicCastGet(const Probe*,probe,object());
    if ( probe )
    {
	uiString errmsg;
	RefMan<Probe> probecopy = probe->clone();
	if ( !ProbeTranslator::store(*probecopy,&ioobj,errmsg) )
	    uirv.add( errmsg );
    }
    return uirv;
}


mDefineInstanceCreatedNotifierAccess( InlineProbe );


InlineProbe::InlineProbe( const TrcKeyZSampling& pos )
    : Probe()
{
    setPos( pos );
    mTriggerInstanceCreatedNotifier();
}


InlineProbe::InlineProbe()
    : Probe()
{
    const int centerpos = SI().inlRange( OD::UsrWork ).center();
    probepos_.hsamp_.setLineRange( Interval<int>(centerpos,centerpos) );
    mTriggerInstanceCreatedNotifier();
}


InlineProbe::InlineProbe( const InlineProbe& oth )
    : Probe(oth)
{
    copyClassData( oth );
    mTriggerInstanceCreatedNotifier();
}


InlineProbe::~InlineProbe()
{
    sendDelNotif();
}


const char* InlineProbe::sFactoryKey()
{
    return sKey::Inline();
}


uiWord InlineProbe::usrType() const
{
    return uiStrings::sInline();
}


uiWord InlineProbe::displayName() const
{
    return mkDispNm( probepos_.hsamp_.inlRange().start );
}


mImplMonitorableAssignmentWithNoMembers( InlineProbe, Probe )


Probe* InlineProbe::createFrom( const IOPar& par )
{
    InlineProbe* inlprobe = new InlineProbe();
    if ( !inlprobe->usePar(par) )
    {
	delete inlprobe;
	return 0;
    }

    return inlprobe;
}

void InlineProbe::initClass()
{
    ProbeFac().addCreateFunc( createFrom, sFactoryKey() );
}


bool InlineProbe::usePar( const IOPar& par )
{
    mLock4Write();

    return Probe::usePar( par );
}


mDefineInstanceCreatedNotifierAccess( CrosslineProbe );


CrosslineProbe::CrosslineProbe( const TrcKeyZSampling& pos )
    : Probe()
{
    setPos( pos );
    mTriggerInstanceCreatedNotifier();
}


CrosslineProbe::CrosslineProbe()
    : Probe()
{
    const int centerpos = SI().crlRange( OD::UsrWork ).center();
    probepos_.hsamp_.setTrcRange( Interval<int>(centerpos,centerpos) );
    mTriggerInstanceCreatedNotifier();
}


CrosslineProbe::CrosslineProbe( const CrosslineProbe& oth )
    : Probe(oth)
{
    copyClassData( oth );
    mTriggerInstanceCreatedNotifier();
}


CrosslineProbe::~CrosslineProbe()
{
    sendDelNotif();
}


const char* CrosslineProbe::sFactoryKey()
{
    return sKey::Crossline();
}


uiWord CrosslineProbe::usrType() const
{
    return uiStrings::sCrossline();
}


uiWord CrosslineProbe::displayName() const
{
    return mkDispNm( probepos_.hsamp_.crlRange().start );
}


mImplMonitorableAssignmentWithNoMembers( CrosslineProbe, Probe )


Probe* CrosslineProbe::createFrom( const IOPar& par )
{
    CrosslineProbe* inlprobe = new CrosslineProbe();
    if ( !inlprobe->usePar(par) )
    {
	delete inlprobe;
	return 0;
    }

    return inlprobe;
}

void CrosslineProbe::initClass()
{
    ProbeFac().addCreateFunc( createFrom, sFactoryKey() );
}


bool CrosslineProbe::usePar( const IOPar& par )
{
    mLock4Write();

    return Probe::usePar( par );
}


mDefineInstanceCreatedNotifierAccess( ZSliceProbe );

ZSliceProbe::ZSliceProbe( const TrcKeyZSampling& pos )
    : Probe()
{
    setPos( pos );
    mTriggerInstanceCreatedNotifier();
}


ZSliceProbe::ZSliceProbe()
    : Probe()
{
    const float centerpos = SI().zRange( OD::UsrWork ).center();
    probepos_.zsamp_.set( centerpos, centerpos, SI().zStep() );
    mTriggerInstanceCreatedNotifier();
}


ZSliceProbe::ZSliceProbe( const ZSliceProbe& oth )
    : Probe(oth)
{
    copyClassData( oth );
    mTriggerInstanceCreatedNotifier();
}


ZSliceProbe::~ZSliceProbe()
{
    sendDelNotif();
}


const char* ZSliceProbe::sFactoryKey()
{
    return sKey::ZSlice();
}


uiWord ZSliceProbe::usrType() const
{
    return uiStrings::sZSlice();
}


uiWord ZSliceProbe::displayName() const
{
    const float pos = probepos_.zsamp_.start * SI().showZ2UserFactor();
    return mkDispNm( pos );
}


mImplMonitorableAssignmentWithNoMembers( ZSliceProbe, Probe )


Probe* ZSliceProbe::createFrom( const IOPar& par )
{
    ZSliceProbe* inlprobe = new ZSliceProbe();
    if ( !inlprobe->usePar(par) )
    {
	delete inlprobe;
	return 0;
    }

    return inlprobe;
}


void ZSliceProbe::initClass()
{
    ProbeFac().addCreateFunc( createFrom, sFactoryKey() );
}


bool ZSliceProbe::usePar( const IOPar& par )
{
    mLock4Write();

    return Probe::usePar( par );
}


mDefineInstanceCreatedNotifierAccess( Line2DProbe );

Line2DProbe::Line2DProbe( Pos::GeomID geomid )
    : Probe()
{
    setGeomID( geomid );
    mTriggerInstanceCreatedNotifier();
}


Line2DProbe::Line2DProbe()
    : Probe()
{
    mTriggerInstanceCreatedNotifier();
}


Line2DProbe::Line2DProbe( const Line2DProbe& oth )
    : Probe(oth)
{
    copyClassData( oth );
    mTriggerInstanceCreatedNotifier();
}


Line2DProbe::~Line2DProbe()
{
    sendDelNotif();
}


void Line2DProbe::setGeomID( Pos::GeomID geomid )
{
    mLock4Read();

    if ( geomid_ == geomid )
	return;

    if ( !mLock2Write() && geomid_ == geomid )
	return;

    geomid_ = geomid;
    const auto& geom2d = SurvGeom::get2D( geomid_ );
    if ( geom2d.isEmpty() )
	{ pErrMsg( "Geometry not found" ); return; }

    geom2d.getSampling( probepos_ );

    for ( int idx=0; idx<layers_.size(); idx++ )
	layers_[idx]->invalidateData();

    mSendChgNotif( cPositionChange(), cEntireObjectChgID() );
}


const char* Line2DProbe::sFactoryKey()
{
    return IOPar::compKey( sKey::TwoD(), sKey::Line() );
}


uiWord Line2DProbe::usrType() const
{
    return uiStrings::sLine();
}


uiWord Line2DProbe::displayName() const
{
    return toUiString( geomid_.name() );
}


void Line2DProbe::fillPar( IOPar& par ) const
{
    mLock4Read();

    Probe::fillPar( par );
    par.set( sKey::GeomID(), geomid_ );
}

bool Line2DProbe::usePar( const IOPar& par )
{
    mLock4Write();

    if ( !Probe::usePar(par) )
	return false;

    return par.get( sKey::GeomID(), geomid_ );
}


mImplMonitorableAssignment( Line2DProbe, Probe )


void Line2DProbe::copyClassData( const Line2DProbe& oth )
{
    geomid_ = oth.geomID();
}


Monitorable::ChangeType Line2DProbe::compareClassData(
					const Line2DProbe& oth ) const
{
    mDeliverYesNoMonitorableCompare( geomid_ == oth.geomid_ );
}


Probe* Line2DProbe::createFrom( const IOPar& par )
{
    Line2DProbe* probe = new Line2DProbe();
    if ( !probe->usePar(par) )
    {
	delete probe;
	return 0;
    }

    return probe;
}

void Line2DProbe::initClass()
{
    ProbeFac().addCreateFunc( createFrom, sFactoryKey() );
}


mDefineInstanceCreatedNotifierAccess( VolumeProbe );

VolumeProbe::VolumeProbe( const TrcKeyZSampling& pos )
    : Probe()
    , zdominfo_( new ZDomain::Info(SI().zDomain()) )
{
    setPos( pos );
    mTriggerInstanceCreatedNotifier();
}


VolumeProbe::VolumeProbe()
    : Probe()
    , zdominfo_( new ZDomain::Info(SI().zDomain()) )
{
    probepos_.hsamp_.start_.inl() =
	(5*probepos_.hsamp_.start_.inl()+3*probepos_.hsamp_.stop_.inl())/8;
    probepos_.hsamp_.start_.crl() =
	(5*probepos_.hsamp_.start_.crl()+3*probepos_.hsamp_.stop_.crl())/8;
    probepos_.hsamp_.stop_.inl() =
	(3*probepos_.hsamp_.start_.inl()+5*probepos_.hsamp_.stop_.inl())/8;
    probepos_.hsamp_.stop_.crl() =
	(3*probepos_.hsamp_.start_.crl()+5*probepos_.hsamp_.stop_.crl())/8;
    probepos_.zsamp_.start =
	( 5*probepos_.zsamp_.start + 3*probepos_.zsamp_.stop ) / 8.f;
    probepos_.zsamp_.stop =
	( 3*probepos_.zsamp_.start + 5*probepos_.zsamp_.stop ) / 8.f;
    SI().snap( probepos_.hsamp_.start_ );
    SI().snap( probepos_.hsamp_.stop_ );
    probepos_.zsamp_.start = probepos_.zsamp_.snap( probepos_.zsamp_.start );
    probepos_.zsamp_.stop = probepos_.zsamp_.snap( probepos_.zsamp_.stop );

    mTriggerInstanceCreatedNotifier();
}


VolumeProbe::VolumeProbe( const VolumeProbe& oth )
    : Probe(oth)
    , zdominfo_(0)
{
    copyClassData( oth );
    mTriggerInstanceCreatedNotifier();
}


VolumeProbe::~VolumeProbe()
{
    sendDelNotif();
}


const char* VolumeProbe::sFactoryKey()
{
    return sKey::Volume();
}


uiWord VolumeProbe::usrType() const
{
    return uiStrings::sVolume();
}


uiWord VolumeProbe::displayName() const
{
    uiWord ret( toUiString("[%1-%2|%3-%4|%5-%6]") );
    const int zuserfac = zdominfo_->userFactor();
    ret	.arg( probepos_.hsamp_.start_.inl() )
	.arg( probepos_.hsamp_.stop_.inl() )
	.arg( probepos_.hsamp_.start_.crl() )
	.arg( probepos_.hsamp_.stop_.crl() )
	.arg( mNINT32(probepos_.zsamp_.start*zuserfac) )
	.arg( mNINT32(probepos_.zsamp_.stop*zuserfac) );
    return ret;
}


void VolumeProbe::setZDomain( const ZDomain::Info& zdom )
{
    mLock4Write();

    delete zdominfo_;
    zdominfo_ = new ZDomain::Info( zdom );
}


mImplMonitorableAssignment( VolumeProbe, Probe )


void VolumeProbe::copyClassData( const VolumeProbe& oth )
{
    delete zdominfo_;
    zdominfo_ = new ZDomain::Info( *oth.zdominfo_ );
}


Monitorable::ChangeType VolumeProbe::compareClassData(
					const VolumeProbe& oth ) const
{
    mDeliverYesNoMonitorableCompare( zdominfo_->def_ == oth.zdominfo_->def_ );
}


Probe* VolumeProbe::createFrom( const IOPar& par )
{
    VolumeProbe* inlprobe = new VolumeProbe();
    if ( !inlprobe->usePar(par) )
    {
	delete inlprobe;
	return 0;
    }

    return inlprobe;
}


void VolumeProbe::initClass()
{
    ProbeFac().addCreateFunc( createFrom, sFactoryKey() );
}


bool VolumeProbe::usePar( const IOPar& par )
{
    mLock4Write();

    return Probe::usePar( par );
}



ProbePresentationInfo::ProbePresentationInfo( const DBKey& prbkey )
    : Presentation::ObjInfo(prbkey)
{
    objtypekey_ = sFactoryKey();
}


ProbePresentationInfo::ProbePresentationInfo()
{
    objtypekey_ = sFactoryKey();
}


Presentation::ObjInfo* ProbePresentationInfo::createFrom( const IOPar& par )
{
    ProbePresentationInfo* probeprinfo = new ProbePresentationInfo;
    if ( !probeprinfo->usePar(par) )
	{ delete probeprinfo; probeprinfo = 0; }

    return probeprinfo;
}


const char* ProbePresentationInfo::sFactoryKey()
{
    return sKey::Probe();
}


void ProbePresentationInfo::initClass()
{
    OD::PrIFac().addCreateFunc( createFrom, sFactoryKey() );
}
