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
    mDefineStaticLocalObject( Threads::Atomic<ProbeLayer::IDType>,
			      lastprobelayerid_, );
    lastprobelayerid_++;
    return ProbeLayer::ID::get( lastprobelayerid_ );
}


ProbeLayer::ProbeLayer()
    : id_(ProbeLayer::getNewID())
    , probe_(0)
{
}


mImplMonitorableAssignment( ProbeLayer, NamedMonitorable )

void ProbeLayer::copyClassData( const ProbeLayer& oth )
{
    probe_ = oth.probe_;
    id_ = oth.id_;
}

const Probe* ProbeLayer::getProbe() const
{
    mLock4Read();

    return probe_;
}

void ProbeLayer::setProbe( const Probe* parent )
{
    mLock4Write();

    probe_ = parent;
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



ProbeLayer* ProbeLayer::clone() const
{
    mLock4Read();

    IOPar probelaypar;
    fillPar( probelaypar );
    return PrLayFac().create( probelaypar );
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

Probe::Probe()
    : SharedObject()
{
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

    deepErase( layers_ );
    for ( int idx=0; idx<oth.layers_.size(); idx++ )
	layers_ += oth.layers_[idx]->clone();
}


Probe* Probe::clone() const
{
    mLock4Read();

    IOPar probepar;
    fillPar( probepar );
    return ProbeFac().create( probepar );
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
    probepos_.usePar( par );
    updateName();
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

    const int probeidx = layers_.indexOf( lay );
    if ( probeidx<0 )
	return 0;

    mSendChgNotif( cLayerToRemove(), probeidx );
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
    mLock4Read();

    for ( int idx=0; idx<layers_.size(); idx++ )
    {
	if ( layers_[idx]->getID()==id )
	    return layers_[idx];
    }

    return 0;
}


ProbeLayer* Probe::getLayer( ProbeLayer::ID id )
{
    mLock4Read();

    for ( int idx=0; idx<layers_.size(); idx++ )
    {
	if ( layers_[idx]->getID()==id )
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
    updateName();

    for ( int idx=0; idx<layers_.size(); idx++ )
	layers_[idx]->invalidateData();

    mSendChgNotif( cPositionChange(), cEntireObjectChangeID() );
}


void Probe::updateName()
{
    name_ = createName();
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

mImplMonitorableAssignment(ProbeSaver,Saveable)

void ProbeSaver::copyClassData( const ProbeSaver& saver )
{
}


uiRetVal ProbeSaver::doStore( const IOObj& ioobj ) const
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
    const int centerpos = SI().inlRange( true ).center();
    probepos_.hsamp_.setLineRange( Interval<int>(centerpos,centerpos) );
    updateName();
    mTriggerInstanceCreatedNotifier();
}

const char* InlineProbe::sFactoryKey()
{ return sKey::Inline(); }


BufferString InlineProbe::createName() const
{
    return BufferString( type(), probepos_.hsamp_.inlRange().start );
}


mImplMonitorableAssignment( InlineProbe, Probe )

void InlineProbe::copyClassData( const InlineProbe& oth )
{
}


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
    const int centerpos = SI().crlRange( true ).center();
    probepos_.hsamp_.setTrcRange( Interval<int>(centerpos,centerpos) );
    updateName();
    mTriggerInstanceCreatedNotifier();
}

const char* CrosslineProbe::sFactoryKey()
{ return sKey::Crossline(); }


BufferString CrosslineProbe::createName() const
{
    return BufferString( type(), probepos_.hsamp_.crlRange().start );
}

mImplMonitorableAssignment( CrosslineProbe, Probe )

void CrosslineProbe::copyClassData( const CrosslineProbe& oth )
{
}


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
    const float centerpos = SI().zRange( true ).center();
    probepos_.zsamp_.set( centerpos, centerpos, SI().zStep() );
    updateName();
    mTriggerInstanceCreatedNotifier();
}

const char* ZSliceProbe::sFactoryKey()
{ return sKey::ZSlice(); }



BufferString ZSliceProbe::createName() const
{
    return BufferString( type(),probepos_.zsamp_.start*SI().showZ2UserFactor());
}


mImplMonitorableAssignment( ZSliceProbe, Probe )

void ZSliceProbe::copyClassData( const ZSliceProbe& oth )
{
}


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
    , geomid_(Survey::GeometryManager::cUndefGeomID())
{
    updateName();
    mTriggerInstanceCreatedNotifier();
}

void Line2DProbe::setGeomID( Pos::GeomID geomid )
{
    mLock4Read();

    if ( geomid_ == geomid )
	return;

    if ( !mLock2Write() && geomid_ == geomid )
	return;

    geomid_ = geomid;
    const Survey::Geometry* geom = Survey::GM().getGeometry( geomid_ );
    if ( !geom )
    { pErrMsg( "Geometry not found" ); return; }

    const Survey::Geometry2D* geom2d = geom->as2D();
    if ( !geom2d )
    { pErrMsg( "2D Geometry not found" ); return; }

    probepos_ = geom2d->sampling();
    updateName();

    for ( int idx=0; idx<layers_.size(); idx++ )
	layers_[idx]->invalidateData();

    mSendChgNotif( cPositionChange(), cEntireObjectChangeID() );
}


const char* Line2DProbe::sFactoryKey()
{ return IOPar::compKey(sKey::TwoD(),sKey::Line()); }


BufferString Line2DProbe::createName() const
{
    return BufferString( Survey::GM().getName(geomid_) );
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
    , zdomain_( new ZDomain::Info(SI().zDomain()) )
{
    setPos( pos );
    mTriggerInstanceCreatedNotifier();
}


VolumeProbe::VolumeProbe()
    : Probe()
    , zdomain_( new ZDomain::Info(SI().zDomain()) )
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
    SI().snap( probepos_.hsamp_.start_, BinID(0,0) );
    SI().snap( probepos_.hsamp_.stop_, BinID(0,0) );
    probepos_.zsamp_.start = probepos_.zsamp_.snap( probepos_.zsamp_.start );
    probepos_.zsamp_.stop = probepos_.zsamp_.snap( probepos_.zsamp_.stop );

    updateName();
    mTriggerInstanceCreatedNotifier();
}

const char* VolumeProbe::sFactoryKey()
{ return sKey::Volume(); }



void VolumeProbe::setZDomain( const ZDomain::Info& zdom )
{
    mLock4Write();

    delete zdomain_;
    zdomain_ = new ZDomain::Info( zdom );
    updateName();
}


BufferString VolumeProbe::createName() const
{
    BufferString name;
    const int zuserfac = zdomain_->userFactor();
    name.add(probepos_.hsamp_.start_.inl()).add("-")
	.add(probepos_.hsamp_.stop_.inl()).add("/")
	.add(probepos_.hsamp_.start_.crl()).add( "-")
	.add(probepos_.hsamp_.stop_.crl()).add("/")
	.add(mNINT32(probepos_.zsamp_.start*zuserfac)).add("-")
	.add(mNINT32(probepos_.zsamp_.stop*zuserfac));

    return name;
}


mImplMonitorableAssignment( VolumeProbe, Probe )

void VolumeProbe::copyClassData( const VolumeProbe& oth )
{
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
    : OD::ObjPresentationInfo(prbkey)
{
    objtypekey_ = sFactoryKey();
}


ProbePresentationInfo::ProbePresentationInfo()
    : OD::ObjPresentationInfo()
{
    objtypekey_ = sFactoryKey();
}


OD::ObjPresentationInfo* ProbePresentationInfo::createFrom(
	const IOPar& par )
{
    ProbePresentationInfo* probeprinfo = new ProbePresentationInfo;
    if ( !probeprinfo->usePar(par) )
    {
	delete probeprinfo;
	return 0;
    }

    return probeprinfo;
}


const char* ProbePresentationInfo::sFactoryKey()
{
    return sKey::Probe();
}


void ProbePresentationInfo::initClass()
{
    OD::PRIFac().addCreateFunc( createFrom, sFactoryKey() );
}
