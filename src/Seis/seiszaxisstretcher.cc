/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : K. Tingdahl
 * DATE     : January 2008
-*/


#include "seiszaxisstretcher.h"

#include "arrayndimpl.h"
#include "cubesubsel.h"
#include "genericnumer.h"
#include "linesubsel.h"
#include "posinfo.h"
#include "seisioobjinfo.h"
#include "seisprovider.h"
#include "seispacketinfo.h"
#include "seisrangeseldata.h"
#include "seisstorer.h"
#include "seistrc.h"
#include "seistrctr.h"
#include "survinfo.h"
#include "timedepthconv.h"
#include "uistrings.h"
#include "velocitycalc.h"
#include "valseriesinterpol.h"
#include "zaxistransform.h"

#define mMaxNrTrcPerChunk	5000


SeisZAxisStretcher::SeisZAxisStretcher( const IOObj& in, const IOObj& out,
					ZAxisTransform& ztf,
					bool forward, bool stretchz,
					const RangeSelData* sd )
    : Executor("Z Axis Stretcher")
    , ztransform_(ztf)
    , ist2d_(forward)
    , stretchz_(stretchz)
    , is2d_(sd && sd->is2D())
{
    ztransform_.ref();
    SeisIOObjInfo info( in );
    is2d_ = info.is2D();

    uiRetVal uirv;
    provider_ = Seis::Provider::create( in, &uirv );
    if ( !provider_ )
	{ msg_ = uirv; return; }


    if ( sd )
	provider_->setSelData( sd->clone() );

    totalnr_ = provider_->totalNr();
    storer_ = new Seis::Storer( out );
    msg_ = tr("Stretching data");
}


SeisZAxisStretcher::~SeisZAxisStretcher()
{
    delete storer_;
    delete provider_;
    delete [] resamplebuf_;
    delete curlhss_;
    delete curchss_;

    if ( voiid_>=0 )
	ztransform_.removeVolumeOfInterest( voiid_ );
    ztransform_.unRef();
}


bool SeisZAxisStretcher::isOK() const
{
    return provider_ && storer_ && ztransform_.isOK();
}


const Seis::RangeSelData* SeisZAxisStretcher::selData() const
{
    return (RangeSelData*)(provider_ ? provider_->selData() : 0);
}


uiString SeisZAxisStretcher::nrDoneText() const
{
    return uiStrings::sPositionsDone();
}


int SeisZAxisStretcher::nextStep()
{
    if ( !outtrc_ )
    {
	outsz_ = outzrg_.nrSteps() + 1;
	outtrc_ = new SeisTrc( outsz_ );
    }

    const SamplingData<float> outsd( outzrg_ );
    SeisTrc intrc;
    PtrMan<FloatMathFunction> intrcfunc = 0;
    PtrMan<ZAxisTransformSampler> sampler = 0;

    if ( !stretchz_ )
    {
	sampler = new ZAxisTransformSampler( ztransform_, true, outsd, is2d_ );
	intrcfunc = new SeisTrcFunction( intrc, 0 );
	if ( !intrcfunc )
	    { msg_ = mINTERNAL("no SeisTrcFunction"); return ErrorOccurred(); }
    }

    if ( !getInputTrace(intrc) )
	return msg_.isEmpty() ? Finished() : ErrorOccurred();

    const TrcKey curtrcky( intrc.info().trcKey() );
    ensureTransformSet( curtrcky );

    const int insz = intrc.size();
    outtrc_->info().sampling_ = outsd;

    if ( !stretchz_ )
    {
	if ( !sampler )
	    return false;
	sampler->setTrcKey( curtrcky );
	sampler->computeCache( Interval<int>( 0, outtrc_->size()-1) );

	ValueSeriesInterpolator<float>* interpol =
	    new ValueSeriesInterpolator<float>( intrc.interpolator() );
	interpol->udfval_ = mUdf(float);
	intrc.setInterpolator( interpol );

	if ( !resamplebuf_ )
	    resamplebuf_ = new float [outsz_];
	reSample( *intrcfunc, *sampler, resamplebuf_, outtrc_->size() );
	for ( int idx=outtrc_->size()-1; idx>=0; idx-- )
	    outtrc_->set( idx, resamplebuf_[idx], 0 );
    }
    else
    {
	bool usevint = isvint_;

	if ( isvrms_ )
	{
	    SeisTrcValueSeries tmpseistrcvsin( intrc, 0 );
	    SamplingData<float> inputsd( intrc.info().sampling_ );
	    float* vintarr = new float[ insz ];
	    if ( !vintarr )
		return false;

	    float* vrmsarr = tmpseistrcvsin.arr();
	    PtrMan<Array1DImpl<float> > inparr = 0;
	    if ( !vrmsarr )
	    {
		inparr = new Array1DImpl<float>( insz );
		tmpseistrcvsin.copytoArray( *inparr );
		vrmsarr = inparr->arr();
	    }

	    computeDix( vrmsarr, inputsd, insz, vintarr );

	    for ( int ids=0; ids<insz; ids++ )
		intrc.set( ids, vintarr[ids], 0 );

	    usevint = true;
	    delete [] vintarr;
	}

	//Computed from the input trace, not the model
	mAllocVarLenArr(float, twt, insz);
	mAllocVarLenArr(float, depths, insz);

	SamplingData<float> inputsd( intrc.info().sampling_ );
	SeisTrcValueSeries inputvs( intrc, 0 );

	ztransform_.transformTrc( curtrcky, inputsd, insz,
				  ist2d_ ? depths : twt );

	if ( ist2d_ )
	{
	    int idx=0;
	    for ( ; idx<insz; idx++ )
	    {
		if ( !mIsUdf(depths[idx]) && !mIsUdf(inputvs[idx]) )
		    break;
		twt[idx] = mUdf(float);
	    }

	    //Fill twt using depth array and input-values
	    if ( usevint )
	    {
		if ( idx!=insz )
		{
		    float prevdepth = depths[idx];
		    float prevtwt = twt[idx] = 2*prevdepth/inputvs[idx];
		    idx++;
		    for ( ; idx<insz; idx++)
		    {
			if ( mIsUdf(depths[idx]) || mIsUdf(inputvs[idx]) )
			    twt[idx] = mUdf(float);
			else
			{
			    prevtwt = twt[idx] = prevtwt +
				(depths[idx]-prevdepth)*2/inputvs[idx];
			    prevdepth = depths[idx];
			}
		    }
		}
	    }
	    else //Vavg
	    {
		for ( ; idx<insz; idx++ )
		{
		    if ( mIsUdf(depths[idx]) || mIsUdf(inputvs[idx]) )
			twt[idx] = mUdf(float);
		    else
			twt[idx] = depths[idx]*2/inputvs[idx];
		}

	    }
	}
	else
	{
	    int idx=0;
	    for ( ; idx<insz; idx++ )
	    {
		if ( !mIsUdf(twt[idx]) && !mIsUdf(inputvs[idx]) )
		    break;
		depths[idx] = mUdf(float);
	    }

	    //Fill depth using twt array and input-values
	    if ( usevint )
	    {
		if ( idx!=insz )
		{
		    float prevtwt = twt[idx];
		    float prevdepth = depths[idx] = prevtwt*inputvs[idx]/2;
		    idx++;
		    for ( ; idx<insz; idx++ )
		    {
			if ( mIsUdf(twt[idx]) || mIsUdf(inputvs[idx] ) )
			    depths[idx] = mUdf(float);
			else
			{
			    prevdepth = depths[idx] = prevdepth +
				(twt[idx]-prevtwt) * inputvs[idx]/2;
			    prevtwt = twt[idx];
			}
		    }
		}
	    }
	    else //Vavg
	    {
		for ( ; idx<insz; idx++ )
		{
		    if ( mIsUdf(depths[idx]) || mIsUdf(inputvs[idx]) )
			twt[idx] = mUdf(float);
		    else
			depths[idx] = twt[idx] * inputvs[idx]/2;
		}
	    }
	}

	PointBasedMathFunction dtfunc( PointBasedMathFunction::Linear,
			    PointBasedMathFunction::ExtraPolGradient );
	PointBasedMathFunction tdfunc( PointBasedMathFunction::Linear,
			    PointBasedMathFunction::ExtraPolGradient );
	float prevdepth;
	float prevtwt;

	if ( ist2d_ )
	{
	    for ( int idx=0; idx<insz; idx++ )
	    {
		if ( mIsUdf(depths[idx]) || mIsUdf(twt[idx]) )
		    continue;

		dtfunc.add( depths[idx], twt[idx] );
	    }

	    prevdepth = outsd.atIndex( 0 );
	    prevtwt = dtfunc.size()
		? dtfunc.getValue( prevdepth )
		: mUdf(float);
	}
	else
	{
	    for ( int idx=0; idx<insz; idx++ )
	    {
		if ( mIsUdf(depths[idx]) || mIsUdf(twt[idx]) )
		    continue;

		tdfunc.add( twt[idx], depths[idx] );
	    }

	    prevtwt = outsd.atIndex( 0 );
	    prevdepth = tdfunc.size()
		? tdfunc.getValue( prevtwt )
		: mUdf(float);
	}

	const bool alludf = ist2d_ ? dtfunc.isEmpty() : tdfunc.isEmpty();
	for ( int idx=1; idx<outsz_; idx++ )
	{
	    float vel;

	    if ( alludf )
		vel = mUdf(float);
	    else
	    {
		float curdepth;
		float curtwt;

		if ( ist2d_ )
		{
		    curdepth = outsd.atIndex( idx );
		    curtwt = dtfunc.getValue( curdepth );
		}
		else
		{
		    curtwt = outsd.atIndex( idx );
		    curdepth = tdfunc.getValue( curtwt );
		}


		if ( usevint )
		{
		    vel = (curdepth-prevdepth)/(curtwt-prevtwt) * 2;
		    prevtwt = curtwt;
		    prevdepth = curdepth;
		}
		else
		{
		    vel = curdepth/curtwt * 2;
		}
	    }

	    outtrc_->set( idx, vel, 0 );
	}

	outtrc_->set( 0, outtrc_->get( 1, 0 ), 0 );
    }

    outtrc_->info().setTrcKey( curtrcky );
    outtrc_->info().coord_ = intrc.info().coord_;
    auto uirv = storer_->put( *outtrc_ );
    if ( !uirv.isOK() )
	{ msg_ = uirv; return false; }

    nrdone_++;
    return true;
}


bool SeisZAxisStretcher::getInputTrace( SeisTrc& trc )
{
    uiRetVal uirv = provider_->getNext( trc );
    if ( !uirv.isOK() )
    {
	if ( !isFinished(uirv) )
	    msg_ = uirv;
	else
	{
	    uirv = storer_->close();
	    if ( !uirv.isOK() )
		msg_ = uirv;
	}

	deleteAndZeroPtr( provider_ );
	return false;
    }
    return true;
}


bool SeisZAxisStretcher::ensureTransformSet( const TrcKey& tk )
{
    TrcKeyZSampling tkzs;
    const auto* seldata = selData();
    if ( is2d_ )
    {
	const Pos::GeomID geomid( tk.geomID() );
	if ( curlhss_ && curlhss_->geomID() == geomid )
	    return true;
	const auto idxof = seldata->indexOf( geomid );
	delete curlhss_;
	if ( idxof < 0 )
	{
	    tkzs = TrcKeyZSampling( geomid );
	    curlhss_ = new LineHorSubSel( geomid );
	}
	else
	{
	    const LineSubSel lss( seldata->fullSubSel().lineSubSel(idxof) );
	    tkzs = TrcKeyZSampling( lss );
	    curlhss_ = new LineHorSubSel( lss );
	}
    }
    else
    {
	const BinID bid( tk.binID() );
	if ( curchss_ && curchss_->includes(bid) )
	    return true;

	CubeSubSel css;
	if ( seldata )
	    css = seldata->fullSubSel().cubeSubSel();
	auto inlrg = css.inlRange();
	const auto crlrg = css.crlRange();
	const auto nrcrl = crlrg.nrSteps() + 1;
	auto nrinl = nrcrl / mMaxNrTrcPerChunk;
	if ( nrinl < 1 )
	    nrinl = 1;
	auto endinl = bid.inl() + inlrg.step * (nrinl - 1);
	if ( endinl < inlrg.stop )
	    inlrg.stop = endinl;
	delete curchss_;
	curchss_ = new CubeHorSubSel( css );
	curchss_->setInlRange( inlrg );
	tkzs = TrcKeyZSampling( css );
    }

    if ( voiid_<0 )
	voiid_ = ztransform_.addVolumeOfInterest( tkzs, true );
    else
	ztransform_.setVolumeOfInterest( voiid_, tkzs, true );

    return ztransform_.loadDataIfMissing( voiid_, SilentTaskRunnerProvider() );
}
