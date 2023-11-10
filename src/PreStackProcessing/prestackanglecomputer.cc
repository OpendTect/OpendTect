/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "prestackanglecomputer.h"

#include "ailayer.h"
#include "arrayndimpl.h"
#include "fftfilter.h"
#include "keystrs.h"
#include "mathfunc.h"
#include "prestackgather.h"
#include "raytrace1d.h"
#include "reflectivitymodel.h"
#include "smoother1d.h"
#include "survinfo.h"
#include "unitofmeasure.h"
#include "veldesc.h"
#include "velocityfunction.h"
#include "zvalseriesimpl.h"


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
    const float srd = getConvertedValue( SI().seismicReferenceDatum(),
				    UnitOfMeasure::surveyDefSRDStorageUnit(),
				    UnitOfMeasure::surveyDefDepthUnit() );
    rtsu_.startdepth( -srd ).doreflectivity( false )
	 .depthtype( SI().depthType() );

    raypars_.set( sKey::Type(), RayTracer1D::factory().getDefaultName() );
}


AngleComputer::~AngleComputer()
{
    delete zdomaininfo_;
}


void AngleComputer::setOutputSampling( const FlatPosData& os,
				       Seis::OffsetType offstyp,
				       const ZDomain::Info& zinfo )
{
    if ( !zinfo.isTime() && !zinfo.isDepth() )
	return;

    if ( outputsampling_ != os )
    {
	outputsampling_  = os;
	refmodel_ = nullptr;
    }

    setZDomain( zinfo );
    rtsu_.offsettype( offstyp );
    if ( zinfo.isDepth() )
	rtsu_.depthtype( zinfo.depthType() );
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


AngleComputer& AngleComputer::setZDomain( const ZDomain::Info& zinfo )
{
    if ( (!zinfo.isTime() && !zinfo.isDepth()) ||
	  (zdomaininfo_ && zinfo == *zDomain()) )
	return *this;

    delete zdomaininfo_;
    zdomaininfo_ = new ZDomain::Info( zinfo );
    return *this;
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
	    float sinangle = sinanglevalsindepth.getValue( depth );
	    if ( sinangle < 0.f ) sinangle = 0.f;
	    else if ( sinangle > 1.f ) sinangle = 1.f;
	    arr1doutput[zidx] = Math::ASin( sinangle );
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
	{
	    float angle = angles.get( idx );
	    if ( angle < 0.f ) angle = 0.f;
	    else if ( angle > M_PI_2f ) angle = M_PI_2f;
	    arr1doutput[startidx+idx] = angle;
	}

	startidx += zsize;
    }
}


void AngleComputer::fftSmooth( Array2D<float>& angledata )
{
    if ( !zDomain() )
	return;

    float freqf3=mUdf(float), freqf4=mUdf(float);
    iopar_.get( sKeyFreqF3(), freqf3 );
    iopar_.get( sKeyFreqF4(), freqf4 );

    if ( mIsUdf(freqf3) || mIsUdf(freqf4) )
	return;

    if ( freqf3 > freqf4 )
    { pErrMsg("f3 must be <= f4"); Swap( freqf3, freqf4 ); }

    const StepInterval<double> zrange = outputsampling_.range( false );
    const int zsize = zrange.nrSteps() + 1;

    ::FFTFilter filter( zsize, (float)zrange.step );
    filter.setLowPass( freqf3, freqf4 );
    zDomain()->isTime() ? fftTimeSmooth( filter, angledata )
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
    const int filtersz = mIsUdf(smoothinglength)
		       ? mUdf(int) : mNINT32( smoothinglength/zstep );

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
    if ( !refmodel || !refmodel->hasAngles() || !zDomain() )
	return false;

    const float* depths = refmodel->getReflDepths();
    const float* times = refmodel->getReflTimes();
    const int nrlayers = refmodel->nrLayers();

    TypeSet<float> offsets;
    outputsampling_.getPositions( true, offsets );
    const int offsetsize = outputsampling_.nrPts( true );
    const int zsize = outputsampling_.nrPts( false );
    const StepInterval<double> outputzrg = outputsampling_.range( false );
    const bool zistime = zDomain()->isTime();
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
	    float sinangle = sinanglevals.getValue( zval );
	    if ( !mIsUdf(sinangle) && Math::IsNormalNumber(sinangle) )
	    {
		if ( sinangle < 0. ) sinangle = 0.f;
		else if ( sinangle > 1.f ) sinangle = 1.f;
		angledata.set( ofsidx, zidx, asin(sinangle) );
		continue;
	    }

	    float angle = anglevals.getValue( zval );
	    if ( angle < 0.f ) angle = 0.f;
	    else if ( angle > M_PI_2f ) angle = M_PI_2f;
	    angledata.set( ofsidx, zidx, angle );
	}
    }

    return true;
}


RefMan<Gather> AngleComputer::computeAngleData()
{
    if ( !zDomain() )
	return nullptr;

    RefMan<Gather> gather = new Gather( outputsampling_, rtsu_.offsettype_,
					*zDomain() );
    gather->setTrcKey( trckey_ );
    Array2D<float>& angledata = gather->data();

    if ( !curRefModel() )
    {
	const ElasticModel* emodel = curElasticModel();
	if ( !emodel )
	{
	    errmsg_ = tr("Cannot retrieve current elastic model");
	    return nullptr;
	}

	PtrMan<RayTracer1D> raytracer =
		     RayTracer1D::createInstance( raypars_, errmsg_, &rtsu_ );
	if ( !raytracer || !errmsg_.isEmpty() )
	    return nullptr;

	TypeSet<float> offsets;
	outputsampling_.getPositions( true, offsets );
	raytracer->setOffsets( offsets, gather->offsetType() );
	if ( !raytracer->setModel(*emodel) || !raytracer->execute() )
	{
	    errmsg_ = raytracer->uiMessage();
	    return nullptr;
	}

	ConstRefMan<OffsetReflectivityModel> refmodel =raytracer->getRefModel();
	if ( !refmodel )
	{
	    errmsg_ = raytracer->uiMessage();
	    return nullptr;
	}

	setRefModel( *refmodel.ptr() );
    }

    if ( !fillandInterpArray(angledata) )
    {
	errmsg_ = tr("Cannot interpolate angles");
	return nullptr;
    }

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


bool VelocityBasedAngleComputer::isOK() const
{
    return velsource_.ptr() && velsource_->getDesc().isVelocity();
}


bool VelocityBasedAngleComputer::setMultiID( const MultiID& mid )
{
    /* mid can be for either a seismic dataset or a velocity function,
       thus two factory implementations need to be checked */
    velsource_ = Vel::FunctionSource::factory().create( nullptr, mid, false );

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


bool VelocityBasedAngleComputer::getLayers( const TrcKey& tk, float startdepth,
					Vel::FunctionSource& velsource,
					ElasticModel& emodel, uiString& errmsg )
{
    ConstRefMan<Vel::Function> func = velsource.getFunction( tk.position() );
    if ( !func )
    {
	errmsg = mToUiStringTodo( velsource.errMsg() );
	return false;
    }

    func.getNonConstPtr()->setZDomain( func->getSource().zDomain() );
    const ZSampling zrange = func->getAvailableZ();
    func.getNonConstPtr()->setDesiredZRange( zrange );

    const ZDomain::Info& zinfo = func->zDomain();
    const RegularZValues zvals_func( zrange, zinfo );
    const od_int64 nrvels = zvals_func.size();
    ArrayValueSeries<double,float> vels( nrvels );
    for ( od_int64 idx=0; idx<nrvels; idx++ )
	vels.setValue( idx, func->getVelocity( float (zvals_func[idx]) ) );

    const UnitOfMeasure* depthuom = UnitOfMeasure::surveyDefDepthUnit();
    const double srd = -startdepth;
    const Vel::Worker worker( func->getDesc(), srd, depthuom );
    const VelocityDesc vintdesc( OD::VelocityType::Interval,
				 UnitOfMeasure::meterSecondUnit() );
    if ( !worker.convertVelocities(vels,zvals_func,vintdesc,vels) )
	return false;

    ConstPtrMan<ZValueSeries> zvals = Vel::Worker::getZVals( zvals_func,
							     srd, depthuom );
    mDynamicCastGet(const RegularZValues*,zvalsin,zvals.ptr());
    if ( !emodel.createFromVel(*zvalsin,vels.storArr()) )
    {
	errmsg = tr("Cannot create the model from the velocities");
	return false;
    }

    return true;
}


RefMan<Gather> VelocityBasedAngleComputer::computeAngles()
{
    if ( trckey_.is2D() )
    {
	errmsg_ = tr("Only 3D is supported at this time" );
	return nullptr;
    }

    if ( !zDomain() )
	return nullptr;

    const float startdepth = rtsu_.startdepth_;
    if ( !velsource_ ||
	 !getLayers(trckey_,startdepth,*velsource_,elasticmodel_,errmsg_) )
	return nullptr;

    if ( !mIsUdf(maxthickness_) )
	elasticmodel_.setMaxThickness( maxthickness_ );

    refmodel_ = nullptr;
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


// ModelBasedAngleComputer

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
