/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bruno
 Date:          July 2011
________________________________________________________________________

-*/
static const char* rcsID mUsedVar = "$Id$";


#include "stratsynth.h"

#include "datapackbase.h"
#include "elasticpropsel.h"
#include "flatposdata.h"
#include "ioman.h"
#include "prestackgather.h"
#include "propertyref.h"
#include "raytracerrunner.h"
#include "survinfo.h"
#include "seisbufadapters.h"
#include "seistrc.h"
#include "seistrcprop.h"
#include "stratlayermodel.h"
#include "stratlayersequence.h"
#include "synthseis.h"
#include "timeser.h"
#include "wavelet.h"



SynthGenParams::SynthGenParams()
    : isps_(false)
{
    const BufferStringSet& facnms = RayTracer1D::factory().getNames( false );
    if ( !facnms.isEmpty() )
	raypars_.set( sKey::Type(), facnms.get( facnms.size()-1 ) );

    RayTracer1D::setIOParsToZeroOffset( raypars_ );
    raypars_.setYN( RayTracer1D::sKeyVelBlock(), true );
    raypars_.set( RayTracer1D::sKeyVelBlockVal(), 20 );
}


const char* SynthGenParams::genName() const
{
    BufferString nm( wvltnm_ );
    TypeSet<float> offset; 
    raypars_.get( RayTracer1D::sKeyOffset(), offset );
    const int offsz = offset.size();
    if ( offsz )
    {
	nm += " ";
	nm += "Offset ";
	nm += toString( offset[0] );
	if ( offsz > 1 )
	    nm += "-"; nm += offset[offsz-1];
    }

    BufferString* newnm = new BufferString( nm );
    return newnm->buf();
}




StratSynth::StratSynth( const Strat::LayerModel& lm )
    : lm_(lm)
    , level_(0)  
    , tr_(0)
    , wvlt_(0)
    , lastsyntheticid_(0)
{}


StratSynth::~StratSynth()
{
    deepErase( synthetics_ );
    setLevel( 0 );
}


void StratSynth::setWavelet( const Wavelet* wvlt )
{
    if ( !wvlt ) 
	return;

    delete wvlt_; 
    wvlt_ = wvlt;
    genparams_.wvltnm_ = wvlt->name();
} 


void StratSynth::clearSynthetics()
{
    deepErase( synthetics_ );
}


#define mErrRet( msg, act )\
{\
    errmsg_ = "Can not generate synthetics:\n";\
    errmsg_ += msg;\
    act;\
}

SyntheticData* StratSynth::addSynthetic()
{
    SyntheticData* sd = generateSD( lm_,tr_ );
    if ( sd )
	synthetics_ += sd;
    return sd;
}


SyntheticData* StratSynth::replaceSynthetic( int id )
{
    SyntheticData* sd = getSynthetic( id );
    if ( !sd ) return 0;

    const int sdidx = synthetics_.indexOf( sd );
    sd = generateSD( lm_, tr_ );
    if ( sd )
	delete synthetics_.replace( sdidx, sd );

    return sd;
}


SyntheticData* StratSynth::addDefaultSynthetic()
{
    genparams_.name_ = genparams_.genName();
    SyntheticData* sd = addSynthetic();

    mDynamicCastGet(PostStackSyntheticData*,psd,sd);
    if ( psd )  generateOtherQuantities( *psd, lm_ );

    return sd;
}


SyntheticData* StratSynth::getSynthetic( const char* nm ) 
{
    for ( int idx=0; idx<synthetics().size(); idx ++ )
    {
	if ( !strcmp( synthetics_[idx]->name(), nm ) )
	    return synthetics_[idx]; 
    }
    return 0;
}


SyntheticData* StratSynth::getSynthetic( int id ) 
{
    for ( int idx=0; idx<synthetics().size(); idx ++ )
    {
	if ( synthetics_[idx]->id_ == id )
	    return synthetics_[ idx ];
    }
    return 0;
}


SyntheticData* StratSynth::getSyntheticByIdx( int idx ) 
{
    return synthetics_.validIdx( idx ) ?  synthetics_[idx] : 0;
}



SyntheticData* StratSynth::getSynthetic( const  PropertyRef& pr )
{
    for ( int idx=0; idx<synthetics_.size(); idx++ )
    {
	mDynamicCastGet(PropertyRefSyntheticData*,pssd,synthetics_[idx]);
	if ( !pssd ) continue;
	if ( pr == pssd->propRef() )
	    return pssd;
    }
    return 0;
}


int StratSynth::nrSynthetics() const 
{
    return synthetics_.size();
}


bool StratSynth::generate( const Strat::LayerModel& lm, SeisTrcBuf& trcbuf )
{
    SyntheticData* dummysd = generateSD( lm );
    if ( !dummysd ) 
	return false;

    for ( int idx=0; idx<lm.size(); idx ++ )
    {
	const SeisTrc* trc = dummysd->getTrace( idx );
	if ( trc )
	    trcbuf.add( new SeisTrc( *trc ) );
    }
    snapLevelTimes( trcbuf, dummysd->d2tmodels_ );

    delete dummysd;
    return !trcbuf.isEmpty();
}


SyntheticData* StratSynth::generateSD( const Strat::LayerModel& lm, 
				       TaskRunner* tr )
{
    errmsg_.setEmpty(); 

    if ( lm.isEmpty() ) 
	return 0;

    Seis::RaySynthGenerator synthgen;
    synthgen.setWavelet( wvlt_, OD::UsePtr );
    const IOPar& raypars = genparams_.raypars_;
    synthgen.usePar( raypars );

    const int nraimdls = lm.size();
    int maxsz = 0;
    for ( int idm=0; idm<nraimdls; idm++ )
    {
	ElasticModel aimod; 
	const Strat::LayerSequence& seq = lm.sequence( idm ); 
	const int sz = seq.size();
	if ( sz < 1 )
	    continue;

	if ( !fillElasticModel( lm, aimod, idm ) )
	{
	    BufferString msg( errmsg_ );
	    mErrRet( msg.buf(), return 0;) 
	}
	maxsz = mMAX( aimod.size(), maxsz );
	synthgen.addModel( aimod );
    }
    if ( maxsz == 0 )
	return 0;

    if ( maxsz == 1 )
	mErrRet( "Model has only one layer, please add an other layer.", 
		return 0; );

    if ( (tr && !tr->execute( synthgen ) ) || !synthgen.execute() )
    {
	const char* errmsg = synthgen.errMsg();
	mErrRet( errmsg ? errmsg : "", return 0 ) ;
    }

    const int crlstep = SI().crlStep();
    const BinID bid0( SI().inlRange(false).stop + SI().inlStep(),
	    	      SI().crlRange(false).stop + crlstep );

    ObjectSet<SeisTrcBuf> tbufs;
    for ( int imdl=0; imdl<nraimdls; imdl++ )
    {
	Seis::RaySynthGenerator::RayModel& rm = synthgen.result( imdl );
	ObjectSet<SeisTrc> trcs; rm.getTraces( trcs, true );
	SeisTrcBuf* tbuf = new SeisTrcBuf( true );
	for ( int idx=0; idx<trcs.size(); idx++ )
	{
	    SeisTrc* trc = trcs[idx];
	    trc->info().binid = BinID( bid0.inl, bid0.crl + imdl * crlstep );
	    trc->info().coord = SI().transform( trc->info().binid );
	    tbuf->add( trc );
	}
	tbufs += tbuf;
    }

    SyntheticData* sd = 0;
    if ( genparams_.isps_ )
    {
	ObjectSet<PreStack::Gather> gatherset;
	while ( tbufs.size() )
	{
	    SeisTrcBuf* tbuf = tbufs.removeSingle( 0 );
	    PreStack::Gather* gather = new PreStack::Gather();
	    if ( !gather->setFromTrcBuf( *tbuf, 0 ) )
		{ delete gather; continue; }

	    gatherset += gather;
	}
	PreStack::GatherSetDataPack* dp = 
		new PreStack::GatherSetDataPack( genparams_.name_, gatherset );
	sd = new PreStackSyntheticData( genparams_, *dp );
    }
    else
    {
	SeisTrcBuf* dptrcbuf = new SeisTrcBuf( true );
	while ( tbufs.size() )
	{
	    SeisTrcBuf* tbuf = tbufs.removeSingle( 0 );
	    SeisTrcPropChg stpc( *tbuf->get( 0 ) );
	    while ( tbuf->size() > 1 )
	    {
		SeisTrc* trc = tbuf->remove( tbuf->size()-1 );
		stpc.stack( *trc );
		delete trc;
	    }
	    dptrcbuf->add( *tbuf );
	}
	SeisTrcBufDataPack* dp = new SeisTrcBufDataPack( *dptrcbuf, Seis::Line,
				   SeisTrcInfo::TrcNr, genparams_.name_ );	
	sd = new PostStackSyntheticData( genparams_, *dp );
    }

    sd->id_ = ++lastsyntheticid_;

    ObjectSet<TimeDepthModel> tmpd2ts;
    for ( int imdl=0; imdl<nraimdls; imdl++ )
    {
	Seis::RaySynthGenerator::RayModel& rm = synthgen.result( imdl );
	rm.getD2T( tmpd2ts, true );
	if ( !tmpd2ts.isEmpty() )
	    sd->d2tmodels_ += tmpd2ts.removeSingle(0);
	deepErase( tmpd2ts );
    }

    return sd;
}


void StratSynth::generateOtherQuantities( PostStackSyntheticData& sd, 
					  const Strat::LayerModel& lm ) 
{
    const PropertyRefSelection& props = lm.propertyRefs();

    for ( int iprop=1; iprop<props.size(); iprop++ )
    {
	SeisTrcBufDataPack* dp = new SeisTrcBufDataPack( 
					(SeisTrcBufDataPack&)sd.getPack() );

	BufferString nm( "[" ); nm += props[iprop]->name(); nm += "]";

	const StepInterval<double>& zrg = dp->posData().range( false );

	SeisTrcBuf* trcbuf = new SeisTrcBuf( dp->trcBuf() );
	const int bufsz = trcbuf->size();

	for ( int iseq=0; iseq<lm.size(); iseq ++ )
	{
	    const Strat::LayerSequence& seq = lm.sequence( iseq ); 
	    const TimeDepthModel& t2d = *sd.d2tmodels_[iseq];

	    int layidx = 0;
	    float laydpt = seq.startDepth();
	    float val = mUdf(float);
	    TypeSet<float> vals; 
	    for ( int idz=0; idz<zrg.nrSteps()+1; idz++ )
	    {
		const float time = zrg.atIndex( idz );
		const float dpt = t2d.getDepth( time );

		const Strat::Layer* lay = 0;
		while ( layidx < seq.size() - 1 )
		{
		    if ( dpt <= laydpt )
			break;

		    lay = seq.layers()[layidx];
		    laydpt += lay->thickness();
		    layidx ++;
		}
		if ( lay && iprop < lay->nrValues() )
		    val = lay->value( iprop );

		vals += val;
	    }
	    SeisTrc* trc = iseq < bufsz ? trcbuf->get( iseq ) : 0;
	    if ( !trc ) continue;
	    Array1DImpl<float> outvals( vals.size() );
	    AntiAlias( 1/(float)5, vals.size(), vals.arr(), outvals.arr() );
	    for ( int idz=0; idz<vals.size(); idz++ )
		trc->set( idz, outvals.get(idz), 0 );
	}


	dp->setBuffer( trcbuf, Seis::Line, SeisTrcInfo::TrcNr );	
	dp->setName( nm );
	PropertyRefSyntheticData* prsd = 
	    new PropertyRefSyntheticData( genparams_, *dp, *props[iprop] );
	prsd->id_ = ++lastsyntheticid_;
	prsd->setName( nm );

	deepCopy( prsd->d2tmodels_, sd.d2tmodels_ );
	synthetics_ += prsd;
    }
}


const char* StratSynth::errMsg() const
{
    return errmsg_.isEmpty() ? 0 : errmsg_.buf();
}


bool StratSynth::fillElasticModel( const Strat::LayerModel& lm, 
				ElasticModel& aimodel, int seqidx )
{
    const Strat::LayerSequence& seq = lm.sequence( seqidx ); 
    const ElasticPropSelection& eps = lm.elasticPropSel();
    const PropertyRefSelection& props = lm.propertyRefs();
    if ( !eps.isValidInput( &errmsg_ ) )
	return false; 

    ElasticPropGen elpgen( eps, props );
    const Strat::Layer* lay = 0;
    for ( int idx=0; idx<seq.size(); idx++ )
    {
	lay = seq.layers()[idx];
	float dval, pval, sval;
	elpgen.getVals( dval, pval, sval, lay->values(), props.size() );

	ElasticLayer ail ( lay->thickness(), pval, sval, dval );
	BufferString msg( "Can not derive synthetic layer property " );
	bool isudf = mIsUdf( dval ) || mIsUdf( pval );
	if ( mIsUdf( dval ) )
	{
	    msg += "'Density'";
	}
	if ( mIsUdf( pval ) )
	{
	    msg += "'P-Wave'";
	}
	msg += ". \n Please check its definition.";
	if ( isudf )
	    { errmsg_ = msg; return false; }

	aimodel += ail;
    }

    bool dovelblock = false; float blockthreshold;
    genparams_.raypars_.getYN( RayTracer1D::sKeyVelBlock(), dovelblock );
    genparams_.raypars_.get( RayTracer1D::sKeyVelBlockVal(), blockthreshold );
    if ( dovelblock )
	blockElasticModel( aimodel, blockthreshold );

    return true;
}


void StratSynth::snapLevelTimes( SeisTrcBuf& trcs, 
			const ObjectSet<const TimeDepthModel>& d2ts ) 
{
    if ( !level_ ) return;

    TypeSet<float> times = level_->zvals_;
    for ( int imdl=0; imdl<times.size(); imdl++ )
	times[imdl] = d2ts.validIdx(imdl) ? 
	    	d2ts[imdl]->getTime( times[imdl] ) : mUdf(float);

    for ( int idx=0; idx<trcs.size(); idx++ )
    {
	const SeisTrc& trc = *trcs.get( idx );
	SeisTrcPropCalc stp( trc );
	float z = times.validIdx( idx ) ? times[idx] : mUdf( float );
	trcs.get( idx )->info().zref = z;
	if ( !mIsUdf( z ) && level_->snapev_ != VSEvent::None )
	{
	    Interval<float> tg( z, trc.startPos() );
	    mFlValSerEv ev1 = stp.find( level_->snapev_, tg, 1 );
	    tg.start = z; tg.stop = trc.endPos();
	    mFlValSerEv ev2 = stp.find( level_->snapev_, tg, 1 );
	    float tmpz = ev2.pos;
	    const bool ev1invalid = mIsUdf(ev1.pos) || ev1.pos < 0;
	    const bool ev2invalid = mIsUdf(ev2.pos) || ev2.pos < 0;
	    if ( ev1invalid && ev2invalid )
		continue;
	    else if ( ev2invalid )
		tmpz = ev1.pos;
	    else if ( fabs(z-ev1.pos) < fabs(z-ev2.pos) )
		tmpz = ev1.pos;

	    z = tmpz;
	}
	trcs.get( idx )->info().pick = z;
    }
}


void StratSynth::setLevel( const Level* lvl )
{ delete level_; level_ = lvl; }



void StratSynth::flattenTraces( SeisTrcBuf& tbuf ) const
{
    if ( tbuf.isEmpty() )
	return;

    float tmax = tbuf.get(0)->info().sampling.start;
    float tmin = tbuf.get(0)->info().sampling.atIndex( tbuf.get(0)->size() );
    for ( int idx=tbuf.size()-1; idx>=1; idx-- )
    {
	tmin = mMIN(tmin,tbuf.get(idx)->info().pick);
	tmax = mMAX(tmax,tbuf.get(idx)->info().pick);
    }

    for ( int idx=tbuf.size()-1; idx>=0; idx-- )
    {
	const SeisTrc* trc = tbuf.get( idx );
	const float start = trc->info().sampling.start - tmax;
	const float stop  = trc->info().sampling.atIndex( trc->size()-1 ) -tmax;
	SeisTrc* newtrc = trc->getRelTrc( ZGate(start,stop) );
	if ( newtrc )
	    delete tbuf.replace( idx, newtrc );
    }
}	


void StratSynth::decimateTraces( SeisTrcBuf& tbuf, int fac ) const
{
    for ( int idx=tbuf.size()-1; idx>=0; idx-- )
    {
	if ( idx%fac )
	    delete tbuf.remove( idx ); 
    }
}


SyntheticData::SyntheticData( const SynthGenParams& sgp, DataPack& dp )
    : NamedObject(sgp.name_)
    , datapack_(dp)
    , id_(-1) 
{
    useGenParams( sgp );
}


SyntheticData::~SyntheticData()
{
    deepErase( d2tmodels_ );
    removePack(); 
}


void SyntheticData::setName( const char* nm )
{
    NamedObject::setName( nm );
    datapack_.setName( nm );
}


void SyntheticData::removePack()
{
    const DataPack::FullID dpid = datapackid_;
    DataPackMgr::ID packmgrid = DataPackMgr::getID( dpid );
    const DataPack* dp = DPM(packmgrid).obtain( DataPack::getID(dpid) );
    if ( dp )
	DPM(packmgrid).release( dp->id() );
}


float SyntheticData::getTime( float dpt, int seqnr ) const
{
    return d2tmodels_.validIdx( seqnr ) ? d2tmodels_[seqnr]->getTime( dpt ) 
					: mUdf( float );
}


float SyntheticData::getDepth( float time, int seqnr ) const
{
    return d2tmodels_.validIdx( seqnr ) ? d2tmodels_[seqnr]->getDepth( time ) 
					: mUdf( float );
}
 


PostStackSyntheticData::PostStackSyntheticData( const SynthGenParams& sgp,
						SeisTrcBufDataPack& dp)
    : SyntheticData(sgp,dp)
{
    DataPackMgr::ID pmid = DataPackMgr::FlatID();
    DPM( pmid ).add( &dp );
    datapackid_ = DataPack::FullID( pmid, dp.id());
}


PostStackSyntheticData::~PostStackSyntheticData()
{
}


const SeisTrc* PostStackSyntheticData::getTrace( int seqnr ) const
{ return postStackPack().trcBuf().get( seqnr ); }



PreStackSyntheticData::PreStackSyntheticData( const SynthGenParams& sgp,
					     PreStack::GatherSetDataPack& dp)
    : SyntheticData(sgp,dp)
{
    DataPackMgr::ID pmid = DataPackMgr::CubeID();
    DPM( pmid ).add( &dp );
    datapackid_ = DataPack::FullID( pmid, dp.id());
}


const Interval<float> PreStackSyntheticData::offsetRange() const
{
    Interval<float> offrg( 0, 0 );
    const ObjectSet<PreStack::Gather>& gathers = preStackPack().getGathers();
    if ( !gathers.isEmpty() ) 
    {
	const PreStack::Gather& gather = *gathers[0];
	offrg.set(gather.getOffset(0),gather.getOffset( gather.size(true)-1));
    }
    return offrg;
}


bool PreStackSyntheticData::hasOffset() const
{ return offsetRange().width() > 0; }


const SeisTrc* PreStackSyntheticData::getTrace( int seqnr, int* offset ) const
{ return preStackPack().getTrace( seqnr, offset ? *offset : 0 ); }


SeisTrcBuf* PreStackSyntheticData::getTrcBuf( float offset, 
					const Interval<float>* stackrg ) const
{
    SeisTrcBuf* tbuf = new SeisTrcBuf( true );
    Interval<float> offrg = stackrg ? *stackrg : Interval<float>(offset,offset);
    preStackPack().fill( *tbuf, offrg );
    return tbuf;
}


PropertyRefSyntheticData::PropertyRefSyntheticData( const SynthGenParams& sgp,
						    SeisTrcBufDataPack& dp,
						    const PropertyRef& pr )
    : PostStackSyntheticData( sgp, dp ) 
    , prop_(pr)
{}



void SyntheticData::fillGenParams( SynthGenParams& sgp ) const
{
    sgp.raypars_ = raypars_;
    sgp.wvltnm_ = wvltnm_;
    sgp.name_ = name();
    sgp.isps_ = isPS();
}


void SyntheticData::useGenParams( const SynthGenParams& sgp )
{
    raypars_ = sgp.raypars_;
    wvltnm_ = sgp.wvltnm_;
    setName( sgp.name_ );
}
