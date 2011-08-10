/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bruno
 Date:          July 2011
________________________________________________________________________

-*/
static const char* rcsID = "$Id: stratsynth.cc,v 1.7 2011-08-10 15:03:51 cvsbruno Exp $";


#include "stratsynth.h"

#include "elasticpropsel.h"
#include "flatposdata.h"
#include "prestackgather.h"
#include "propertyref.h"
#include "survinfo.h"
#include "seisbufadapters.h"
#include "seistrc.h"
#include "stratlayermodel.h"
#include "stratlayersequence.h"
#include "velocitycalc.h"
#include "wavelet.h"


StratSynth::StratSynth( const Strat::LayerModel& lm )
    : lm_(lm)
    , wvlt_(0)
{}

void StratSynth::setWavelet( const Wavelet& wvlt )
{
    wvlt_ = &wvlt;
}


#define mErrRet( msg, act ) { if ( errmsg ) *errmsg = msg; act; }
DataPack* StratSynth::genTrcBufDataPack( const RayParams& raypars,
				ObjectSet<const TimeDepthModel>& d2ts,
				BufferString* errmsg ) const 
{
    ObjectSet<SeisTrcBuf> seisbufs;
    if ( !genSeisBufs( raypars, d2ts, seisbufs, errmsg ) || seisbufs.isEmpty() )
	return 0;

    SeisTrcBuf* tbuf = new SeisTrcBuf( true );
    const int crlstep = SI().crlStep();
    const BinID bid0( SI().inlRange(false).stop + SI().inlStep(),
	    	      SI().crlRange(false).stop + crlstep );

    const int nraimdls = raypars.cs_.nrInl();
    ObjectSet<const SeisTrc> trcs;
    for ( int imdl=0; imdl<nraimdls; imdl++ )
    {
	tbuf->stealTracesFrom( *seisbufs[imdl] );
    }
    deepErase( seisbufs );

    if ( tbuf->isEmpty() ) 
	mErrRet("No seismic trace genereated ", return 0)

    SeisTrcBufDataPack* tdp = new SeisTrcBufDataPack( 
			tbuf, Seis::Line, SeisTrcInfo::TrcNr, "Seismic" ) ;
    const SeisTrc& trc0 = *tbuf->get(0);
    StepInterval<double> zrg( trc0.info().sampling.start,
			      trc0.info().sampling.atIndex(trc0.size()-1),
			      trc0.info().sampling.step );
    tdp->posData().setRange( true, StepInterval<double>(1,tbuf->size(),1) );
    tdp->posData().setRange( false, zrg );
    tdp->setName( raypars.synthname_ );

    return tdp;
}


DataPack* StratSynth::genGatherDataPack( const RayParams& raypars,
				ObjectSet<const TimeDepthModel>& d2ts,
       				BufferString* errmsg ) const 
{
    ObjectSet<SeisTrcBuf> seisbufs;
    genSeisBufs( raypars, d2ts, seisbufs );
    if ( seisbufs.isEmpty() )
	return 0;

    ObjectSet<PreStack::Gather> gathers;
    const int nraimdls = raypars.cs_.nrInl();
    for ( int imdl=0; imdl<nraimdls; imdl++ )
    {
	SeisTrcBuf& tbuf = *seisbufs[imdl];
	PreStack::Gather* gather = new PreStack::Gather();
	if ( !gather->setFromTrcBuf( tbuf, 0 ) )
	    { delete gather; continue; }

	gather->posData().setRange(true,StepInterval<double>(1,tbuf.size(),1));
	gathers += gather;
    }
    deepErase( seisbufs );

    if ( gathers.isEmpty() ) 
	mErrRet("No seismic trace genereated ", return 0)

    PreStack::GatherSetDataPack* pdp = new PreStack::GatherSetDataPack(
							"GatherSet", gathers );
    pdp->setName( raypars.synthname_ );
    return pdp;
}


bool StratSynth::genSeisBufs( const RayParams& raypars,
			    ObjectSet<const TimeDepthModel>& d2ts,
			    ObjectSet<SeisTrcBuf>& seisbufs,
			    BufferString* errmsg ) const 
{
    if ( lm_.isEmpty() ) 
	return false;

    const CubeSampling& cs = raypars.cs_;
    TypeSet<float> offsets;
    for ( int idx=0; idx<cs.nrCrl(); idx++ )
	offsets += cs.hrg.crlRange().atIndex(idx);

    Seis::RaySynthGenerator synthgen;
    synthgen.setRayParams( raypars.setup_, offsets, raypars.usenmotimes_ );
    synthgen.setWavelet( wvlt_, OD::UsePtr );
    const int nraimdls = cs.nrInl();

    for ( int iseq=0; iseq<nraimdls; iseq++ )
    {
	int seqidx = cs.hrg.inlRange().atIndex(iseq)-1;
	ElasticModel aimod; 
	if ( !fillElasticModel( aimod, lm_.sequence( seqidx ), errmsg ) )
	    return false;
	if ( aimod.isEmpty() )
	    mErrRet( "Layer model is empty", return false;) 
	else if ( aimod.size() == 1  )
	    mErrRet("Please add at least one layer to the model", return false;)

	synthgen.addModel( aimod );
    }

    if ( !synthgen.doWork() )
	mErrRet( synthgen.errMsg(), return 0 );

    const int crlstep = SI().crlStep();
    const BinID bid0( SI().inlRange(false).stop + SI().inlStep(),
	    	      SI().crlRange(false).stop + crlstep );

    ObjectSet<const SeisTrc> trcs;
    ObjectSet<const TimeDepthModel> tmpd2ts;
    for ( int imdl=0; imdl<nraimdls; imdl++ )
    {
	Seis::RaySynthGenerator::RayModel& rm = 
	    const_cast<Seis::RaySynthGenerator::RayModel&>( 
						    synthgen.result( imdl ) );
	trcs.erase(); 
	if ( raypars.dostack_ )
	    trcs += rm.stackedTrc();
	else
	    rm.getTraces( trcs, true );

	if ( trcs.isEmpty() )
	    continue;

	seisbufs += new SeisTrcBuf( true );
	for ( int idx=0; idx<trcs.size(); idx++ )
	{
	    SeisTrc* trc = const_cast<SeisTrc*>( trcs[idx] );
	    const int trcnr = imdl + 1;
	    trc->info().nr = trcnr;
	    trc->info().binid = BinID( bid0.inl, bid0.crl + imdl * crlstep );
	    trc->info().coord = SI().transform( trc->info().binid );
	    seisbufs[imdl]->add( trc );
	}
	rm.getD2T( tmpd2ts, true );
	if ( !tmpd2ts.isEmpty() )
	    d2ts += tmpd2ts.remove(0);
	deepErase( tmpd2ts );
    }
    return true;
}


bool StratSynth::fillElasticModel( ElasticModel& aimodel, 
				const Strat::LayerSequence& seq,
				BufferString* errmsg ) const
{
    const int sz = seq.size();
    if ( sz < 1 )
	return false;

    const ElasticPropSelection& eps = lm_.elasticPropSel();
    const PropertyRefSelection& props = lm_.propertyRefs();
    if ( !eps.isValidInput() )
	mErrRet( "No valid elastic propery found", return false; )

    const Strat::Layer* lay = 0;
    int didx = props.indexOf(eps.getPropertyRef(ElasticFormula::Den).name() );
    int pvidx = props.indexOf(eps.getPropertyRef(ElasticFormula::PVel).name() );
    int svidx = props.indexOf(eps.getPropertyRef(ElasticFormula::SVel).name() );
    for ( int idx=0; idx<sz; idx++ )
    {
	lay = seq.layers()[idx];
	const float den = didx >= 0 ? lay->value( didx ) : mUdf( float );
	const float pval = pvidx >= 0 ? lay->value( pvidx ) : mUdf( float );
	const float sval = svidx >= 0 ? lay->value( svidx ) : mUdf( float );

	AILayer ail ( lay->thickness(), den, pval );
	aimodel += ail;
    }
    return true;
}


const SyntheticData* StratSynth::generate( const RayParams& rp, bool isps, 
					BufferString* errmsg ) const
{
    SyntheticData* sd = new SyntheticData( rp.synthname_ );
    DataPack* dp = isps ? genGatherDataPack( rp, sd->d2tmodels_, errmsg )
		        : genTrcBufDataPack( rp, sd->d2tmodels_, errmsg );
    if ( !dp ) 
	{ delete sd; return 0; }

    DataPackMgr::ID pmid = isps ? DataPackMgr::CubeID() : DataPackMgr::FlatID();
    DPM( pmid ).add( dp );

    sd->wvlt_ = wvlt_;
    sd->isps_ = isps;
    sd->packid_ = DataPack::FullID( pmid, dp->id());

    return sd;
}



SyntheticData::~SyntheticData()
{
    deepErase( d2tmodels_ );
    const DataPack::FullID dpid = packid_;
    DataPackMgr::ID packmgrid = DataPackMgr::getID( dpid );
    const DataPack* dp = DPM(packmgrid).obtain( DataPack::getID(dpid) );
    if ( dp )
	DPM(packmgrid).release( dp->id() );
}


