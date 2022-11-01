/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "prestackanglecomputer.h"

#include "ailayer.h"
#include "arrayndimpl.h"
#include "reflectivitymodel.h"
#include "fftfilter.h"
#include "mathfunc.h"
#include "prestackgather.h"
#include "raytrace1d.h"
#include "smoother1d.h"
#include "survinfo.h"
#include "velocitycalc.h"
#include "velocityfunction.h"


namespace PreStack
{

mDefineEnumUtils(AngleComputer,smoothingType,"Smoothing Type")
{
	"None",
	"Moving-Average",
	"Low-pass frequency filter",
	nullptr
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
    : trckey_(BinID::udf())
{
}


AngleComputer::~AngleComputer()
{
}


void AngleComputer::setOutputSampling( const FlatPosData& os )
{
    outputsampling_  = os;
}


void AngleComputer::setRayTracerPars( const IOPar& raypar )
{
    raypars_ = raypar;
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


void AngleComputer::fftDepthSmooth( ::FFTFilter& filter,
				    Array2D<float>& angledata )
{
    const TimeDepthModelSet* refmodel = curRefModel();
    if ( !refmodel )
	return;

    float* arr1doutput = angledata.getData();
    if ( !arr1doutput )
	return;

    const StepInterval<double> zrange = outputsampling_.range( false );
    const int zsize = zrange.nrSteps() + 1;
    const int offsetsize = outputsampling_.nrPts( true );

    for ( int ofsidx=0; ofsidx<offsetsize; ofsidx++ )
    {
	const TimeDepthModel* td = refmodel->get( ofsidx );
	if ( !td || !td->isOK() )
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
	    layertwt = td->getTime( depth );
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
	{
	    const float sinval = sinanglevals.getValue( zidx*deftimestep );
	    const float asinval = Math::ASin( sinval );
	    angles.set( zidx, asinval );
	}

	filter.apply( angles );
	PointBasedMathFunction sinanglevalsindepth(PointBasedMathFunction::Poly,
				    PointBasedMathFunction::ExtraPolGradient );

	float layerdepth = 0, prevlayerdepth = mUdf(float);
	for ( int zidx=0; zidx<zsizeintime; zidx++ )
	{
	    const float time = zidx * deftimestep;
	    layerdepth = td->getDepth( time );
	    if ( mIsEqual(layerdepth,prevlayerdepth,1e-3) )
		continue;

	    sinanglevalsindepth.add( layerdepth, sin(angles.get(zidx) ) );
	    prevlayerdepth = layerdepth;
	}

	for ( int zidx=0; zidx<zsize; zidx++ )
	{
	    const float depth = sCast( float, zrange.atIndex(zidx) );
	    arr1doutput[zidx] =
		Math::ASin( sinanglevalsindepth.getValue(depth) );
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
    const ReflectivityModelBase* refmodel = curRefModel();
    if ( !refmodel || !refmodel->hasAngles() )
	return false;

    const float* depths = refmodel->getReflDepths();
    const float* times = refmodel->getReflTimes();
    const int nrlayers = refmodel->nrLayers();

    TypeSet<float> offsets;
    outputsampling_.getPositions( true, offsets );
    const int offsetsize = outputsampling_.nrPts( true );
    const int zsize = outputsampling_.nrPts( false );
    const StepInterval<double> outputzrg = outputsampling_.range( false );
    const bool zistime = SI().zIsTime();
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
	if ( !gatheriscorrected_ )
	{
	    times = refmodel->getReflTimes( ofsidx );
	    if ( !times )
		return false;
	}

	for ( int layeridx=0; layeridx<nrlayers; layeridx++ )
	{
	    float sinangle = refmodel->getSinAngle( ofsidx, layeridx );
	    if ( mIsUdf(sinangle) )
		continue;

	    if ( fabs(sinangle) > 1.0f )
		sinangle = sinangle > 0.f ? 1.0f : -1.0f;

	    const float zval = zistime ? times[layeridx] : depths[layeridx];
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


RefMan<Gather> AngleComputer::computeAngleData()
{
    RefMan<Gather> gather = new Gather( outputsampling_ );
    gather->setTrcKey( trckey_ );
    Array2D<float>& angledata = gather->data();

    if ( !curRefModel() )
    {
	const ElasticModel* emodel = curElasticModel();
	if ( !emodel )
	    return nullptr;

	if ( !raypars_.isPresent(sKey::Type()) )
	{
	    const int lastitem = RayTracer1D::factory().size()-1;
	    raypars_.set( sKey::Type(), lastitem >= 0
		    ? RayTracer1D::factory().getNames().get( lastitem ).str()
		    : VrmsRayTracer1D::sFactoryKeyword() );
	}

	uiString errormsg;
	RayTracer1D* raytracer =
		     RayTracer1D::createInstance( raypars_, errormsg );
	if ( !errormsg.isEmpty() )
	{
	    delete raytracer;
	    return nullptr;
	}

	raytracer->setup().doreflectivity( false );
	TypeSet<float> offsets;
	outputsampling_.getPositions( true, offsets );
	raytracer->setOffsets( offsets );
	if ( !raytracer->setModel(*emodel) )
	    return nullptr;

	if ( !raytracer->execute() )
	    return nullptr;

	ConstRefMan<OffsetReflectivityModel> refmodel =raytracer->getRefModel();
	if ( !refmodel )
	    return nullptr;

	setRefModel( *refmodel.ptr() );
    }

    if ( !fillandInterpArray(angledata) )
	return nullptr;

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
{}


VelocityBasedAngleComputer::~VelocityBasedAngleComputer()
{
}


bool VelocityBasedAngleComputer::setMultiID( const MultiID& mid )
{
    velsource_ = Vel::FunctionSource::factory().create( 0, mid, false );

    return velsource_;
}


const OffsetReflectivityModel* VelocityBasedAngleComputer::curRefModel() const
{
    return refmodel_.ptr();
}


void VelocityBasedAngleComputer::setRefModel(
				 const OffsetReflectivityModel& refmodel )
{
    refmodel_ = &refmodel;
}


RefMan<Gather> VelocityBasedAngleComputer::computeAngles()
{
    ConstRefMan<Survey::Geometry> geom =
	Survey::GM().getGeometry( const_cast<const TrcKey&>(trckey_).geomID() );

    if ( geom && geom->is2D() )
	{ pErrMsg( "Only 3D is supported at this time" ); return 0; }

    if ( !velsource_ )
	return nullptr;

    ConstRefMan<Vel::Function> func =
			       velsource_->getFunction( trckey_.position() );
    if ( !func )
	return nullptr;

    VelocityDesc veldesc = func->getDesc();
    if ( !veldesc.isVelocity() )
	return nullptr;

    const StepInterval<float> desiredzrange = func->getDesiredZ();
    StepInterval<float> zrange = func->getAvailableZ();
    zrange.limitTo( desiredzrange );

    const int zsize = zrange.nrSteps() + 1;
    mAllocVarLenArr( float, velsrc, zsize );
    mAllocVarLenArr( float, vel, zsize );
    if ( !mIsVarLenArrOK(velsrc) || !mIsVarLenArrOK(vel) )
	return nullptr;

    for( int idx=0; idx<zsize; idx++ )
	velsrc[idx] = func->getVelocity( zrange.atIndex(idx) );

    if ( !convertToVintIfNeeded(velsrc,veldesc,zrange,vel) ||
	 !elasticmodel_.createFromVel(zrange,vel) )
	return nullptr;

    elasticmodel_.setMaxThickness( maxthickness_ );

    return computeAngleData();
}



// ModelBasedAngleComputer
ModelBasedAngleComputer::ModelTool::ModelTool( const ElasticModel& em,
					       const TrcKey& tk )
    : em_(new ElasticModel(em))
    , trckey_(tk)
{
}


ModelBasedAngleComputer::ModelTool::ModelTool(
					const OffsetReflectivityModel& refmodel,
					const TrcKey& tk )
    : refmodel_(&refmodel)
    , trckey_(tk)
{
}


ModelBasedAngleComputer::ModelTool::~ModelTool()
{
    delete em_;
}


const OffsetReflectivityModel*
ModelBasedAngleComputer::ModelTool::curRefModel() const
{
    return refmodel_.ptr();
}


void ModelBasedAngleComputer::ModelTool::setRefModel(
				    const OffsetReflectivityModel& refmodel )
{
    refmodel_ = &refmodel;
}


void ModelBasedAngleComputer::ModelTool::splitModelIfNeeded( float maxthickness)
{
    if ( !refmodel_ )
	return;

    const TimeDepthModel& tdmodel = refmodel_->getDefaultModel();
    const float* depths = tdmodel.getDepths();
    bool dosplit = false;
    for ( int idx=1; idx<tdmodel.size(); idx++ )
    {
	const float thickness = depths[idx] - depths[idx-1];
	if ( thickness > maxthickness )
	{
	    dosplit = true;
	    break;
	}
    }

    if ( !dosplit )
	return;

    if ( em_ )
	em_->setEmpty();
    else
	em_ = new ElasticModel();

    const float* times = tdmodel.getTimes();
    for ( int idx=1; idx<tdmodel.size(); idx++ )
    {
	const double thickness = depths[idx] - depths[idx-1];
	const double pvel = 2. * thickness / (times[idx] - times[idx-1]);
	em_->add( new AILayer( float(thickness), float(pvel), mUdf(float) ) );
    }

    refmodel_ = nullptr;
    em_->setMaxThickness( maxthickness );
}



ModelBasedAngleComputer::ModelBasedAngleComputer()
    : AngleComputer()
{
}


ModelBasedAngleComputer::~ModelBasedAngleComputer()
{
    deepErase( tools_ );
}


bool ModelBasedAngleComputer::isOK() const
{
    return curElasticModel() ? curElasticModel()->size()
			     : (curRefModel() ? curRefModel()->size() : false);
}


void ModelBasedAngleComputer::setElasticModel( const TrcKey& tk,
					       bool block, bool pvelonly,
					       ElasticModel& em )
{
    if ( block )
    {
	em.block( thresholdparam_, pvelonly );
	em.setMaxThickness( maxthickness_ );
    }

    auto* tool = new ModelTool( em, tk );
    const int toolidx = tools_.indexOf( tool );
    if ( toolidx<0 )
	tools_ += tool;
    else
	delete tools_.replace( toolidx, tool );
    trckey_ = tk;
}


void ModelBasedAngleComputer::setRefModel(
				const OffsetReflectivityModel& refmodel )
{
    setRefModel( refmodel, trckey_ );
}


void ModelBasedAngleComputer::setRefModel(
					const OffsetReflectivityModel& refmodel,
					const TrcKey& tk )
{
    auto* tool = curModelTool();
    if ( !tool || tool->trcKey() != tk )
	tool = new ModelTool( refmodel, tk );

    const int toolidx = tools_.indexOf( tool );
    if ( toolidx<0 )
    {
	tools_.add( tool );
	trckey_ = tk;
    }
    else
	tool->setRefModel( refmodel );

    tool->splitModelIfNeeded( maxthickness_ );
}


const ModelBasedAngleComputer::ModelTool*
				ModelBasedAngleComputer::curModelTool() const
{
    return mSelf().curModelTool();
}


ModelBasedAngleComputer::ModelTool* ModelBasedAngleComputer::curModelTool()
{
    for ( int idx=0; idx<tools_.size(); idx++ )
	if ( tools_[idx]->trcKey() == trckey_ )
	    return tools_[idx];
    return nullptr;
}


const ElasticModel* ModelBasedAngleComputer::curElasticModel() const
{
    return curModelTool() ? curModelTool()->elasticModel() : &elasticmodel_;
}


const OffsetReflectivityModel* ModelBasedAngleComputer::curRefModel() const
{
    return curModelTool() ? curModelTool()->curRefModel() : nullptr;
}


RefMan<Gather> ModelBasedAngleComputer::computeAngles()
{
    return computeAngleData();
}

} // namespace PreStack
