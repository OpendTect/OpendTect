/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : Y. Liu
 * DATE     : January 2010
-*/


#include "prestackanglecomputer.h"

#include "arrayndinfo.h"
#include "arrayndalgo.h"
#include "elasticmodel.h"
#include "fourier.h"
#include "fftfilter.h"
#include "mathfunc.h"
#include "position.h"
#include "prestackgather.h"
#include "raytrace1d.h"
#include "smoother1d.h"
#include "survinfo.h"
#include "trckey.h"
#include "velocitycalc.h"
#include "velocityfunction.h"

mDefineEnumUtils(PreStack::AngleComputer,smoothingType,"Smoothing Type")
{
	"None",
	"Moving-Average",
	"Low-pass frequency filter",
	0
};

template<>
void EnumDefImpl<PreStack::AngleComputer::smoothingType>::init()
{
    uistrings_ += uiStrings::sNone();
    uistrings_ += mEnumTr("Moving-Average",0);
    uistrings_ += mEnumTr("Low-Pass Frequency Filter",0);
}

namespace PreStack
{
const char* AngleComputer::sKeySmoothType()	{ return "Smoothing type"; }
const char* AngleComputer::sKeyWinFunc()	{ return "Window function"; }
const char* AngleComputer::sKeyWinParam()	{ return "Window parameter"; }
const char* AngleComputer::sKeyWinLen()		{ return "Window length"; }
const char* AngleComputer::sKeyFreqF3()		{ return "F3 freq"; }
const char* AngleComputer::sKeyFreqF4()		{ return "F4 freq"; }

static const float deftimestep = 0.004f;
static const float maxtwttime = 100.0f;


AngleComputer::AngleComputer()
{
}


AngleComputer::~AngleComputer()
{
    delete elasticmodel_;
}


int AngleComputer::nrLayers() const
{
    return raytracedata_ ? raytracedata_->nrLayers()
			 : ( elasticmodel_ ? elasticmodel_->size() : 0 );
}


void AngleComputer::setOutputSampling( const FlatPosData& os )
{ outputsampling_  = os; }


void AngleComputer::gatherIsNMOCorrected( bool yn )
{
    iscorrected_ = yn;
}


void AngleComputer::setRayTracerPars( const IOPar& raypar )
{
    raypar_ = raypar;
}


void AngleComputer::setSmoothingPars( const IOPar& smpar )
{
    int smoothtype = 0;
    smpar.get( AngleComputer::sKeySmoothType(), smoothtype );

    if ( smoothtype == AngleComputer::None )
	setNoSmoother();

    else if ( smoothtype == AngleComputer::MovingAverage )
    {
	float winlength;
	smpar.get( sKeyWinLen(), winlength );
	BufferString winfunc;
	smpar.get( sKeyWinFunc(), winfunc );
	if ( winfunc == CosTaperWindow::sName() )
	{
	    float param;
	    smpar.get( sKeyWinParam(), param );
	    setMovingAverageSmoother( winlength, winfunc, param );
	}
	else
	    setMovingAverageSmoother( winlength, winfunc );
    }

    else if ( smoothtype == AngleComputer::FFTFilter )
    {
	float freqf3;
	smpar.get( sKeyFreqF3(), freqf3 );
	float freqf4;
	smpar.get( sKeyFreqF4(), freqf4 );
	setFFTSmoother( freqf3, freqf4 );
    }
}


void AngleComputer::setNoSmoother()
{
    iopar_.set( sKeySmoothType(), None );
}


void AngleComputer::setMovingAverageSmoother( float length, BufferString win,
					      float param )
{
    iopar_.set( sKeySmoothType(), MovingAverage );
    iopar_.set( sKeyWinLen(), length );
    iopar_.set( sKeyWinFunc(), win );
    if ( win == CosTaperWindow::sName() && param >= 0 && param <= 1 )
	iopar_.set( sKeyWinParam(), param );
}


void AngleComputer::setFFTSmoother( float freqf3, float freqf4 )
{
    iopar_.set( sKeySmoothType(), FFTFilter );
    iopar_.set( sKeyFreqF3(), freqf3 );
    iopar_.set( sKeyFreqF4(), freqf4 );
}


bool AngleComputer::isOK() const
{
    return nrLayers() > 0 &&
	   raytracedata_ &&
	   raytracedata_->nrOffset() == outputsampling_.nrPts( true );
}


void AngleComputer::fftDepthSmooth(::FFTFilter& filter,
				   Array2D<float>& angledata )
{
    if ( !raytracedata_.ptr() )
	return;

    float* arr1doutput = angledata.getData();
    if ( !arr1doutput )
	return;

    const StepInterval<double> zrange = outputsampling_.range( false );
    const int zsize = zrange.nrSteps() + 1;
    const int offsetsize = outputsampling_.nrPts( true );

    for ( int ofsidx=0; ofsidx<offsetsize; ofsidx++ )
    {
	const TimeDepthModel& td = raytracedata_->getTDModel( ofsidx );
	if ( !td.isOK() )
	{
	    arr1doutput = arr1doutput + zsize;
	    continue;
	}

	PointBasedMathFunction sinanglevals( PointBasedMathFunction::Poly,
				     PointBasedMathFunction::ExtraPolGradient );

	float layertwt = 0, prevlayertwt = mUdf(float);
	for ( int zidx=0; zidx<zsize; zidx++ )
	{
	    const float depth = mCast( float, zrange.atIndex(zidx) );
	    layertwt = td.getTime( depth );
	    if ( mIsEqual(layertwt,prevlayertwt,1e-3) )
		continue;

	    sinanglevals.add( layertwt, sin(angledata.get(ofsidx, zidx)) );
	    prevlayertwt = layertwt;
	}

	const int zsizeintime = mCast( int, layertwt/deftimestep );
	if ( mIsUdf(layertwt) || layertwt < 0 || layertwt > maxtwttime ||
	     zsizeintime <= 0 )
	{
	    arr1doutput = arr1doutput + zsize;
	    continue;
	}

	Array1DImpl<float> angles( zsizeintime );
	for ( int zidx=0; zidx<zsizeintime; zidx++ )
	    angles.set( zidx, asin(sinanglevals.getValue( zidx*deftimestep )) );

	filter.apply( angles );
	PointBasedMathFunction sinanglevalsindepth(PointBasedMathFunction::Poly,
				    PointBasedMathFunction::ExtraPolGradient );

	float layerdepth = 0, prevlayerdepth = mUdf(float);
	for ( int zidx=0; zidx<zsizeintime; zidx++ )
	{
	    const float time = zidx * deftimestep;
	    layerdepth = td.getDepth( time );
	    if ( mIsEqual(layerdepth,prevlayerdepth,1e-3) )
		continue;

	    sinanglevalsindepth.add( layerdepth, sin(angles.get(zidx) ) );
	    prevlayerdepth = layerdepth;
	}

	for ( int zidx=0; zidx<zsize; zidx++ )
	{
	    const float depth = mCast( float, zrange.atIndex(zidx) );
	    arr1doutput[zidx] = asin( sinanglevalsindepth.getValue( depth ) );
	}

	arr1doutput = arr1doutput + zsize;
    }
}


void AngleComputer::fftTimeSmooth( ::FFTFilter& filter,
				   Array2D<float>& angledata )
{
    const StepInterval<double> zrange = outputsampling_.range( false );
    const int zsize = zrange.nrSteps() + 1;
    const int offsetsize = outputsampling_.nrPts( true );

    float* arr1doutput = angledata.getData();
    if ( !arr1doutput )
	return;

    int startidx = 0;
    for ( int ofsidx=0; ofsidx<offsetsize; ofsidx++ )
    {
	Array1DImpl<float> angles( zsize );
	for ( int idx=0; idx<zsize; idx++ )
	    angles.set( idx, arr1doutput[startidx+idx] );

	if ( !filter.apply(angles) )
	{
	    startidx += zsize;
	    continue;
	}

	for ( int idx=0; idx<zsize; idx++ )
	    arr1doutput[startidx+idx] = angles.get( idx );

	startidx += zsize;
    }
}


void AngleComputer::fftSmooth( Array2D<float>& angledata )
{
    float freqf3=mUdf(float), freqf4=mUdf(float);
    iopar_.get( sKeyFreqF3(), freqf3 );
    iopar_.get( sKeyFreqF4(), freqf4 );

    if ( mIsUdf(freqf3) || mIsUdf(freqf4) )
	return;

    if ( freqf3 > freqf4 )
    { pErrMsg("f3 must be <= f4"); std::swap( freqf3, freqf4 ); }

    const StepInterval<double> zrange = outputsampling_.range( false );
    const int zsize = zrange.nrSteps() + 1;
    const bool survintime = SI().zDomain().isTime();

    ::FFTFilter filter( zsize, (float)zrange.step );
    filter.setLowPass( freqf3, freqf4 );
    survintime ? fftTimeSmooth( filter, angledata )
	       : fftDepthSmooth( filter, angledata );
}


void AngleComputer::averageSmooth( Array2D<float>& angledata )
{
    BufferString windowname;
    float smoothingparam( mUdf(float) ); float smoothinglength( mUdf(float) );

    iopar_.get( sKeyWinFunc(), windowname );
    iopar_.get( sKeyWinParam(), smoothingparam );
    iopar_.get( sKeyWinLen(), smoothinglength );

    const int offsetsize = outputsampling_.nrPts( true );
    const int zsize = outputsampling_.nrPts( false );
    const float zstep = mCast( float, outputsampling_.range( false ).step );
    const int filtersz = !mIsUdf(smoothinglength)
		? mNINT32( smoothinglength/zstep ) : mUdf(int);

    Smoother1D<float> sm;
    mAllocVarLenArr( float, arr1dinput, zsize );
    float* arr1doutput = angledata.getData();
    for ( int ofsidx=0; ofsidx<offsetsize; ofsidx++ )
    {
	OD::memCopy( arr1dinput, arr1doutput, zsize*sizeof(float) );
	sm.setInput( arr1dinput, zsize );
	sm.setOutput( arr1doutput );
	sm.setWindow( windowname, smoothingparam, filtersz );
	sm.execute();

	arr1doutput = arr1doutput + zsize;
    }
}


bool AngleComputer::fillAndInterpolateAngleData( Array2D<float>& angledata )
{
    if ( !raytracedata_.ptr() )
	return false;

    const int nrlayers = raytracedata_->nrLayers();
    mAllocVarLenArr( float, depths, nrlayers );
    mAllocVarLenArr( float, times, nrlayers );
    if ( !mIsVarLenArrOK(depths) || !mIsVarLenArrOK(times) ) return false;
    const bool zistime = SI().zIsTime();
    if ( !zistime || iscorrected_ )
    {
	for ( int layeridx=0; layeridx<nrlayers; layeridx++ )
	    depths[layeridx] = raytracedata_->getDepth( layeridx );

	if ( iscorrected_ )
	{
	    const TimeDepthModel& tdmodel = raytracedata_->getZeroOffsTDModel();
	    for ( int layeridx=0; layeridx<nrlayers; layeridx++ )
		times[layeridx] = tdmodel.getTime( depths[layeridx] );
	}
    }

    TypeSet<float> offsets;
    outputsampling_.getPositions( true, offsets );
    const int offsetsize = raytracedata_->nrOffset();
    const int zsize = outputsampling_.nrPts( false );
    const StepInterval<double> outputzrg = outputsampling_.range( false );
    for ( int ofsidx=0; ofsidx<offsetsize; ofsidx++ )
    {
	const float offset = raytracedata_->getOffset( ofsidx );
	const bool zerooffset = mIsZero(offset,1e-1f);
	PointBasedMathFunction sinanglevals(
				    PointBasedMathFunction::Poly,
				    PointBasedMathFunction::ExtraPolGradient ),
			       anglevals(
				    PointBasedMathFunction::Linear,
				    PointBasedMathFunction::ExtraPolGradient );

	sinanglevals.add( 0.f, zerooffset ? 0.f : 1.f );
	anglevals.add( 0.f, zerooffset ? 0.f : M_PI_2f );

	for ( int layeridx=0; layeridx<nrlayers; layeridx++ )
	{
	    float sinangle = raytracedata_->getSinAngle( layeridx, ofsidx );
	    if ( mIsUdf(sinangle) )
		continue;

	    if ( fabs(sinangle) > 1.0f )
		sinangle = sinangle > 0.f ? 1.0f : -1.0f;

	    const float zval = zistime
		  ? (iscorrected_ ? times[layeridx]
				  : raytracedata_->getTime(layeridx,ofsidx))
		  : depths[layeridx];
	    sinanglevals.add( zval, sinangle );
	    anglevals.add( zval, Math::ASin(sinangle) );
	}

	for ( int zidx=0; zidx<zsize; zidx++ )
	{
	    const double layerz = outputzrg.atIndex( zidx );
	    const float zval = mCast(float, layerz );
	    const float sinangle = sinanglevals.getValue( zval );
	    float angle = asin( sinangle );
	    if ( mIsUdf(sinangle) || !Math::IsNormalNumber(angle) )
		angle = anglevals.getValue( zval );

	    angledata.set( ofsidx, zidx, angle );
	}
    }

    return true;
}


void AngleComputer::convertToDegrees( Array2D<float>& angles )
{
    ArrayMath::getScaled<float,float,float>( angles, 0, mRad2DegF, 0.f, false,
					     false );
}


bool AngleComputer::needsRaytracing() const
{
    if ( !raytracedata_ )
	return true;

    TypeSet<float> offsets;
    outputsampling_.getPositions( true, offsets );
    const int nroffsets = offsets.size();
    if ( raytracedata_->nrOffset() != nroffsets )
	return true;

    for ( int ioff=0; ioff<nroffsets; ioff++ )
    {
	if ( !mIsEqual(offsets[ioff],raytracedata_->getOffset(ioff),1e-1f) )
	    return true;
    }

    return false;
}


bool AngleComputer::doRaytracing( uiString& msg )
{
    if ( !elasticmodel_ || elasticmodel_->size() < 2 )
    {
	msg = elasticmodel_ ? tr("Elastic model is too small.")
		: tr("No elastic model provided for angle computer.");
	return false;
    }

    if ( raypar_.isEmpty() )
    {
	const int lastitem = RayTracer1D::factory().size()-1;
	raypar_.set( sKey::Type(), lastitem >= 0
		    ? RayTracer1D::factory().getKeys().get( lastitem ).str()
		    : VrmsRayTracer1D::sFactoryKeyword() );
    }

    PtrMan<RayTracer1D> raytracer =
			RayTracer1D::createInstance( raypar_, msg );
    if ( !raytracer || !msg.isEmpty() )
	return false;

    raytracer->setup().doreflectivity( false );
    TypeSet<float> offsets;
    outputsampling_.getPositions( true, offsets );
    raytracer->setOffsets( offsets );
    if ( !raytracer->setModel(*elasticmodel_) || !raytracer->execute() )
    {
	msg = raytracer->message();
	return false;
    }

    raytracedata_ = raytracer->results();
    return raytracedata_;
}


RefMan<Gather> AngleComputer::computeAngles()
{
    uiString msg;
    if ( needsRaytracing() && !doRaytracing(msg) )
	return 0;

    RefMan<Gather> gather = new Gather( outputsampling_ );
    gather->setAmpType( Seis::IncidenceAngle,
			outputindegrees_ ? Gather::Deg : Gather::Rad );
    gather->setCorrected( iscorrected_ );
    Array2D<float>& angledata = gather->data();
    if ( !isOK() || !fillAndInterpolateAngleData(angledata) )
	return 0;

    int smtype;
    iopar_.get( sKeySmoothType(), smtype );

    if ( smtype == MovingAverage )
	averageSmooth( angledata );
    else if ( smtype == FFTFilter )
	fftSmooth( angledata );

    if ( outputindegrees_ )
	convertToDegrees( angledata );

    return gather;
}



// VelocityBasedAngleComputer
VelocityBasedAngleComputer::VelocityBasedAngleComputer()
    : AngleComputer()
    , tk_(*new TrcKey)
{
}


VelocityBasedAngleComputer::~VelocityBasedAngleComputer()
{
    if ( velsource_ )
	velsource_->unRef();
    delete &tk_;
}


bool VelocityBasedAngleComputer::isOK() const
{
    if ( !AngleComputer::isOK() )
	return false;

    return velsource_ && !tk_.isUdf();
}


void VelocityBasedAngleComputer::setTrcKey(const TrcKey& tk )
{
    tk_ = tk;
}


bool VelocityBasedAngleComputer::setDBKey( const DBKey& dbky )
{
    if ( velsource_ )
	velsource_->unRef();
    velsource_ = Vel::FunctionSource::factory().createSuitable( dbky );
    if ( velsource_ )
	velsource_->ref();

    return velsource_;
}


RefMan<Gather> VelocityBasedAngleComputer::computeAngles()
{
    if ( tk_.is2D() )
	{ pErrMsg( "Only 3D is supported at this time" ); return 0; }

    RefMan<Vel::FunctionSource> source = velsource_;
    if ( !source )
	return 0;

    ConstRefMan<Vel::Function> func = source->getFunction( tk_.position() );
    if ( !func )
	return 0;

    VelocityDesc veldesc = func->getDesc();
    if ( !veldesc.isVelocity() )
	return 0;

    const StepInterval<float> desiredzrange = func->getDesiredZ();
    StepInterval<float> zrange = func->getAvailableZ();
    zrange.limitTo( desiredzrange );

    const int zsize = zrange.nrSteps() + 1;
    mAllocVarLenArr( float, velsrc, zsize );
    mAllocVarLenArr( float, vel, zsize );
    if ( !mIsVarLenArrOK(velsrc) || !mIsVarLenArrOK(vel) )
	return 0;

    for( int idx=0; idx<zsize; idx++ )
	velsrc[idx] = func->getVelocity( zrange.atIndex(idx) );

    if ( !convertToVintIfNeeded(velsrc,veldesc,zrange,vel) )
	return 0;

    if ( !elasticmodel_ )
	elasticmodel_ = new ElasticModel;

    if ( !elasticmodel_->createFromVel(zrange,vel) )
	return 0;

    elasticmodel_->setMaxThickness( maxthickness_ );

    return AngleComputer::computeAngles();
}



// ModelBasedAngleComputer

ModelBasedAngleComputer::ModelBasedAngleComputer()
    : AngleComputer()
{
}


void ModelBasedAngleComputer::setElasticModel( const ElasticModel& em,
					       bool block, bool pvelonly )
{
    if ( !elasticmodel_ || (elasticmodel_ && &em != elasticmodel_) )
    {
	if ( elasticmodel_ )
	    *elasticmodel_ = em;
	else
	    elasticmodel_ = new ElasticModel( em );
    }

    if ( block )
    {
	elasticmodel_->block( thresholdparam_, pvelonly );
	elasticmodel_->setMaxThickness( maxthickness_ );
    }
}


void ModelBasedAngleComputer::setRayTraceData( const RayTracerData& data )
{
    raytracedata_ = &data;
    splitModelIfNeeded();
}


void ModelBasedAngleComputer::splitModelIfNeeded()
{
    if ( !raytracedata_ )
	return;

    bool dosplit = false;
    for ( int idx=0; idx<raytracedata_->nrLayers(); idx++ )
    {
	const float thickness = idx==0 ? raytracedata_->getDepth( idx )
			      : raytracedata_->getDepth( idx ) -
			        raytracedata_->getDepth( idx - 1 );
	if ( thickness > maxthickness_ )
	{
	    dosplit = true;
	    break;
	}
    }

    if ( !dosplit )
	return;

    if ( elasticmodel_ )
	elasticmodel_->setEmpty();
    else
	elasticmodel_ = new ElasticModel;

    elasticmodel_->add( AILayer( raytracedata_->getDepth(0),
				 2.f * raytracedata_->getDepth(0) /
				 raytracedata_->getTnmo( 0 ), mUdf(float) ) );

    for ( int idx=1; idx<raytracedata_->nrLayers(); idx++ )
    {
	const double curdepth = raytracedata_->getDepth( idx );
	const double prevdepth = raytracedata_->getDepth( idx - 1 );
	const double curtwt = raytracedata_->getTnmo( idx );
	const double prevtwt = raytracedata_->getTnmo( idx - 1 );
	const double thickness = raytracedata_->getDepth( idx ) -
			        raytracedata_->getDepth( idx - 1 );
	const double pvel = 2. * ( curdepth - prevdepth ) /
				 ( curtwt - prevtwt );
	elasticmodel_->add( AILayer( mCast(float,thickness),
				     mCast(float,pvel), mUdf(float) ) );
    }

    elasticmodel_->mergeSameLayers();
    setElasticModel( *elasticmodel_, true, true );

    raytracedata_ = 0;
}

} // namespace PreStack
