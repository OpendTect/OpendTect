/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : Y. Liu
 * DATE     : January 2010
-*/

static const char* rcsID mUsedVar = "$Id$";

#include "prestackanglecomputer.h"

#include "ailayer.h"
#include "arrayndimpl.h"
#include "arrayndinfo.h"
#include "arrayndalgo.h"
#include "fourier.h"
#include "fftfilter.h"
#include "mathfunc.h"
#include "position.h"
#include "prestackgather.h"
#include "raytrace1d.h"
#include "smoother1d.h"
#include "survinfo.h"
#include "velocitycalc.h"
#include "velocityfunction.h"

#include "hiddenparam.h"


namespace PreStack
{

static HiddenParam<AngleComputer,int> gathercorrectedparammgr_( 0 );

mDefineEnumUtils(AngleComputer,smoothingType,"Smoothing Type")
{
	"None",
	"Moving-Average",
	"Low-pass frequency filter",
	0
};

const char* AngleComputer::sKeySmoothType()	{ return "Smoothing type"; }
const char* AngleComputer::sKeyWinFunc()	{ return "Window function"; }
const char* AngleComputer::sKeyWinParam()	{ return "Window parameter"; }
const char* AngleComputer::sKeyWinLen()		{ return "Window length"; }
const char* AngleComputer::sKeyFreqF3()		{ return "F3 freq"; }
const char* AngleComputer::sKeyFreqF4()		{ return "F4 freq"; }

static const float deftimestep = 0.004f;
static const float maxtwttime = 100.0f;


AngleComputer::AngleComputer()
    : thresholdparam_(0.01)
    , needsraytracing_(true)
    , raytracer_(0)
    , trckey_(TrcKey::std3DSurvID(),BinID(0,0))
    , maxthickness_(25.f)
{
    gathercorrectedparammgr_.setParam( this, 1 );
}


AngleComputer::~AngleComputer()
{
    delete raytracer_;
    gathercorrectedparammgr_.removeParam( this );
}


void AngleComputer::setOutputSampling( const FlatPosData& os )
{ outputsampling_  = os; }


void AngleComputer::setGatherIsNMOCorrected( bool yn )
{
    gathercorrectedparammgr_.setParam( this, yn ? 1 : 0 );
}


void AngleComputer::setRayTracer( const IOPar& raypar )
{
    uiString errormsg;
    raytracer_ = RayTracer1D::createInstance( raypar, errormsg );
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


void AngleComputer::fftDepthSmooth(::FFTFilter& filter,
				   Array2D<float>& angledata )
{
    const RayTracer1D* rt = curRayTracer();
    if ( !rt )
	return;

    float* arr1doutput = angledata.getData();
    if ( !arr1doutput )
	return;

    const StepInterval<double> zrange = outputsampling_.range( false );
    const int zsize = zrange.nrSteps() + 1;
    const int offsetsize = outputsampling_.nrPts( true );

    TimeDepthModel td;
    for ( int ofsidx=0; ofsidx<offsetsize; ofsidx++ )
    {
	rt->getTDModel( ofsidx, td );
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
    { pErrMsg("f3 must be <= f4"); Swap( freqf3, freqf4 ); }

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


bool AngleComputer::fillandInterpArray( Array2D<float>& angledata )
{
    const RayTracer1D* rt = curRayTracer();
    if ( !rt ) return false;

    const bool iscorrected_ = gathercorrectedparammgr_.getParam( this ) == 1;
    const int nrlayers = rt->getModel().size();
    mAllocVarLenArr( float, depths, nrlayers );
    mAllocVarLenArr( float, times, nrlayers );
    if ( !mIsVarLenArrOK(depths) || !mIsVarLenArrOK(times) ) return false;
    const bool zistime = SI().zIsTime();
    if ( !zistime || iscorrected_ )
    {
	for ( int layeridx=0; layeridx<nrlayers; layeridx++ )
	    depths[layeridx] = rt->getDepth( layeridx );

	if ( iscorrected_ )
	{
	    TimeDepthModel tdmodel; rt->getZeroOffsTDModel( tdmodel );
	    for ( int layeridx=0; layeridx<nrlayers; layeridx++ )
		times[layeridx] = tdmodel.getTime( depths[layeridx] );
	}
    }

    const bool zinfeet	= SI().zInFeet();
    TypeSet<float> offsets;
    outputsampling_.getPositions( true, offsets );
    const int offsetsize = outputsampling_.nrPts( true );
    const int zsize = outputsampling_.nrPts( false );
    const StepInterval<double> outputzrg = outputsampling_.range( false );
    for ( int ofsidx=0; ofsidx<offsetsize; ofsidx++ )
    {
	const float offset = offsets[ofsidx];
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
	    float sinangle = rt->getSinAngle( layeridx, ofsidx );
	    if ( mIsUdf(sinangle) )
		continue;

	    if ( fabs(sinangle) > 1.0f )
		sinangle = sinangle > 0.f ? 1.0f : -1.0f;

	    const float zval = zistime
		  ? (iscorrected_ ? times[layeridx]
				  : rt->getTime(layeridx,ofsidx))
		  : depths[layeridx];
	    sinanglevals.add( zval, sinangle );
	    anglevals.add( zval, Math::ASin(sinangle) );
	}

	for ( int zidx=0; zidx<zsize; zidx++ )
	{
	    const double layerz = outputzrg.atIndex( zidx );
	    const float zval = mCast(float, zinfeet ? layerz * mFromFeetFactorD
						    : layerz );
	    const float sinangle = sinanglevals.getValue( zval );
	    float angle = asin( sinangle );
	    if ( mIsUdf(sinangle) || !Math::IsNormalNumber(angle) )
		angle = anglevals.getValue( zval );

	    angledata.set( ofsidx, zidx, angle );
	}
    }

    return true;
}


Gather* AngleComputer::computeAngleData()
{
    Gather* gather = new Gather( outputsampling_ );
    Array2D<float>& angledata = gather->data();

    if ( needsraytracing_ )
    {
	if ( !raytracer_ )
	{
	    IOPar iopar;
	    const int lastitem = RayTracer1D::factory().size()-1;
	    iopar.set( sKey::Type(), lastitem >= 0
		    ? RayTracer1D::factory().getNames().get( lastitem ).str()
		    : VrmsRayTracer1D::sFactoryKeyword() );
	    uiString errormsg;
	    raytracer_ = RayTracer1D::createInstance( iopar, errormsg );
	    if ( !errormsg.isEmpty() )
		return 0;
	}

	raytracer_->setup().doreflectivity( false );
	TypeSet<float> offsets;
	outputsampling_.getPositions( true, offsets );
	raytracer_->setOffsets( offsets );
	if ( !raytracer_->setModel(curElasticModel()) )
	    return 0;

	if ( !raytracer_->execute() )
	    return 0;
    }

    if ( !fillandInterpArray(angledata) )
	return 0;

    int smtype;
    iopar_.get( sKeySmoothType(), smtype );

    if ( smtype == MovingAverage )
	averageSmooth( angledata );
    else if ( smtype == FFTFilter )
	fftSmooth( angledata );

    return gather;
}



// VelocityBasedAngleComputer
VelocityBasedAngleComputer::VelocityBasedAngleComputer()
    : AngleComputer()
    , velsource_( 0 )
{}


VelocityBasedAngleComputer::~VelocityBasedAngleComputer()
{
    if ( velsource_ ) velsource_->unRef();
}


bool VelocityBasedAngleComputer::setMultiID( const MultiID& mid )
{
    if ( velsource_ ) velsource_->unRef();
    velsource_ = Vel::FunctionSource::factory().create( 0, mid, false );
    if ( velsource_ ) velsource_->ref();

    return velsource_;
}


Gather* VelocityBasedAngleComputer::computeAngles()
{
    ConstRefMan<Survey::Geometry> geom =
	Survey::GM().getGeometry( trckey_.geomID() );

    if ( geom && geom->is2D() )
	{ pErrMsg( "Only 3D is supported at this time" ); return 0; }

    RefMan<Vel::FunctionSource> source = velsource_;
    if ( !source )
	return 0;

    ConstRefMan<Vel::Function> func = source->getFunction( trckey_.position() );
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

    if ( !convertToVintIfNeeded(velsrc,veldesc,zrange,vel) ||
	 !elasticmodel_.createFromVel(zrange,vel) )
	return 0;

    elasticmodel_.setMaxThickness( maxthickness_ );

    return computeAngleData();
}



// ModelBasedAngleComputer
const ElasticModel&
	ModelBasedAngleComputer::ModelTool::elasticModel() const
{ return rt_ ? rt_->getModel() : *em_; }


ModelBasedAngleComputer::ModelBasedAngleComputer()
    : AngleComputer()
{
}


void ModelBasedAngleComputer::setElasticModel( const TrcKey& tk,
					       bool block, bool pvelonly,
					       ElasticModel& em	)
{
    if ( block )
    {
	em.block( thresholdparam_, pvelonly );
	em.setMaxThickness( maxthickness_ );
    }

    ModelTool* tool = new ModelTool( em, tk );
    const int toolidx = tools_.indexOf( tool );
    if ( toolidx<0 )
	tools_ += tool;
    else
	delete tools_.replace( toolidx, tool );
    trckey_ = tk;
    needsraytracing_ = true;
}


void ModelBasedAngleComputer::setRayTracer( const RayTracer1D* rt,
					    const TrcKey& tk )
{
    ModelTool* tool = new ModelTool( rt, tk );
    const int toolidx = tools_.indexOf( tool );
    if ( toolidx<0 )
	tools_ += tool;
    else
	delete tools_.replace( toolidx, tool );
    trckey_ = tk;
    needsraytracing_ = false;
    splitModelIfNeeded();
}


void ModelBasedAngleComputer::splitModelIfNeeded()
{
    const RayTracer1D* rt1d = curRayTracer();
    if ( !rt1d )
	return;

    const ElasticModel& em = rt1d->getModel();
    bool dosplit = false;
    for ( int idx=0; idx<em.size(); idx++ )
    {
	if ( em[idx].thickness_ > maxthickness_ )
	{
	    dosplit = true;
	    break;
	}
    }

    if ( !dosplit )
	return;

    ElasticModel newem( em );
    const TrcKey tk = curModelTool()->trcKey();
    const int toolidx = tools_.indexOf( curModelTool() );
    delete tools_.removeSingle( toolidx );
    setElasticModel( tk, true, true, newem );
}


const ModelBasedAngleComputer::ModelTool*
				ModelBasedAngleComputer::curModelTool() const
{
    for ( int idx=0; idx<tools_.size(); idx++ )
	if ( tools_[idx]->trcKey() == trckey_ )
	    return tools_[idx];
    return 0;
}


const ElasticModel& ModelBasedAngleComputer::curElasticModel() const
{
    return curModelTool() ? curModelTool()->elasticModel() : elasticmodel_;
}


const RayTracer1D* ModelBasedAngleComputer::curRayTracer() const
{
    if ( raytracer_ ) return raytracer_;
    return curModelTool() ? curModelTool()->rayTracer() : 0;
}


Gather* ModelBasedAngleComputer::computeAngles()
{
    return computeAngleData();
}

} // namespace PreStack
