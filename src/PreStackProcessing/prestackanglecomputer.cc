/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : Y. Liu
 * DATE     : January 2010
-*/

static const char* rcsID mUsedVar = "$Id$";

#include "prestackanglecomputer.h"

#include "arrayndimpl.h"
#include "arrayndinfo.h"
#include "arrayndutils.h"
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

using namespace PreStack;


DefineEnumNames(AngleComputer,smoothingType,0,"Smoothing Type")
{
	"Time-Average",
	"Low-pass frequency filter", 
	0
};


static const float deftimestep = 0.004;


AngleComputer::AngleComputer()
    : thresholdparam_(0.01)
    , needsraytracing_(true)
    , raytracer_(0)
    , trcid_(trcid_.std3DGeomID(),0,0)
{
    maxthickness_ = SI().depthsInFeet() ? 165.0f : 50.0f;
    iopar_.set( sKeySmoothType(), TimeAverage );
    iopar_.set( sKeyWinFunc(), HanningWindow::sName() );
    iopar_.set( sKeyWinParam(), 0.95f );
    iopar_.set( sKeyWinLen(), 125 );
}


AngleComputer::~AngleComputer()
{
    delete raytracer_;
}


void AngleComputer::setOutputSampling( const FlatPosData& os )
{ outputsampling_  = os; }


void AngleComputer::setSmoothingPars( const IOPar& iopar )
{
    iopar_ = iopar;
}


void AngleComputer::setRayTracer( const IOPar& raypar )
{
    BufferString errormsg;
    raytracer_ = RayTracer1D::createInstance( raypar, errormsg );
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

    float freqf3;
    iopar_.get( sKeyFreqF3(), freqf3 );
    TimeDepthModel td;

    for ( int ofsidx=0; ofsidx<offsetsize; ofsidx++ )
    {
	PointBasedMathFunction anglevals( PointBasedMathFunction::Linear,
					  PointBasedMathFunction::EndVal );

	rt->getTDModel( ofsidx, td );
	float layertwt = 0, prevlayertwt = mUdf(float);
	for ( int zidx=0; zidx<zsize; zidx++ )
	{
	    const float depth = mCast( float, zrange.atIndex(zidx) );
	    layertwt = td.getTime( depth );
	    if ( mIsEqual(layertwt,prevlayertwt,1e-3) )
		continue;

	    anglevals.add( layertwt, angledata.get(ofsidx, zidx) );
	    prevlayertwt = layertwt;
	}

	const int zsizeintime = mCast( int, layertwt/deftimestep );
	const float df = Fourier::CC::getDf( deftimestep, zsizeintime );
	filter.setLowPass( df, freqf3, false );

	mAllocVarLenArr( float, inputarray, zsizeintime );
	mAllocVarLenArr( float, outputarray, zsizeintime );
	for ( int zidx=0; zidx<zsizeintime; zidx++ )
	    inputarray[zidx] = anglevals.getValue( zidx*deftimestep );
	
	filter.apply( inputarray, outputarray, zsizeintime );

	PointBasedMathFunction anglevalsindepth( PointBasedMathFunction::Linear,
					    PointBasedMathFunction::EndVal );

	float layerdepth = 0, prevlayerdepth = mUdf(float);
	for ( int zidx=0; zidx<zsizeintime; zidx++ )
	{
	    const float time = zidx * deftimestep;
	    layerdepth = td.getDepth( time );
	    if ( mIsEqual(layerdepth,prevlayerdepth,1e-3) )
		continue;

	    anglevalsindepth.add( layerdepth, outputarray[zidx] );
	    prevlayerdepth = layerdepth;
	}

	for ( int zidx=0; zidx<zsize; zidx++ )
	{
	    const float depth = mCast( float, zrange.atIndex(zidx) );
	    arr1doutput[zidx] = anglevalsindepth.getValue( depth );
	}

	arr1doutput = arr1doutput + zsize;
    }
}


void AngleComputer::fftTimeSmooth(::FFTFilter& filter, 
				  Array2D<float>& angledata )
{
    const StepInterval<double> zrange = outputsampling_.range( false );
    const int zsize = zrange.nrSteps() + 1;
    const int offsetsize = outputsampling_.nrPts( true );
    float freqf3;
    iopar_.get( sKeyFreqF3(), freqf3 );

    const float df = Fourier::CC::getDf( mCast(float,zrange.step), zsize );
    filter.setLowPass( df, freqf3, false );

    mAllocVarLenArr( float, arr1dinput, zsize );
    float* arr1doutput = angledata.getData();
    if ( !arr1doutput )
	return;

    for ( int ofsidx=0; ofsidx<offsetsize; ofsidx++ )
    {
	memcpy( arr1dinput, arr1doutput, zsize*sizeof(float) );
	filter.apply( arr1dinput, arr1doutput, zsize );
	arr1doutput = arr1doutput + zsize;
    }
}


void AngleComputer::fftSmooth( Array2D<float>& angledata )
{
    float freqf3=mUdf(float), freqf4=mUdf(float);
    iopar_.get( sKeyFreqF3(), freqf3 );
    iopar_.get( sKeyFreqF4(), freqf4 );

    if ( mIsUdf(freqf3) || mIsUdf(freqf4) )
	return;

    ::FFTFilter filter;
    const int nyquistfreq = (int)( Fourier::CC::getNyqvist( 
			SI().zDomain().isTime() ? SI().zStep() : deftimestep) ); 
    int winsz = 2*( nyquistfreq-(int)freqf3 );
    if ( nyquistfreq<=(int)freqf3 || mIsEqual( freqf3, freqf4, 0.5 ) )
	winsz = 0;

    Array1DImpl<float> lwin( winsz/2 );
    if ( winsz > 0 )
    {
	float taperwinrelsz = 1-(freqf4-freqf3) / (nyquistfreq - freqf3);
			  
	if ( taperwinrelsz >=0 && taperwinrelsz <= 1 )
	{
	    ArrayNDWindow window( Array1DInfoImpl(winsz), false,
				  CosTaperWindow().sName(), taperwinrelsz );
	    float* winvals = window.getValues();
	    for ( int idx=0; idx<winsz/2 && winvals; idx++ )
		lwin.set( idx, 1-winvals[idx] );
	    filter.setFreqBorderWindow( lwin.getData(), winsz/2, true );
	}
    }

    SI().zDomain().isTime() ? fftTimeSmooth( filter, angledata )
			    : fftDepthSmooth( filter, angledata );
}


void AngleComputer::averageSmooth( Array2D<float>& angledata )
{
    BufferString windowname;
    float smoothingparam; int smoothinglength;

    iopar_.get( sKeyWinFunc(), windowname );
    iopar_.get( sKeyWinParam(), smoothingparam );
    iopar_.get( sKeyWinLen(), smoothinglength );

    const int offsetsize = outputsampling_.nrPts( true );
    const int zsize = outputsampling_.nrPts( false );

    Smoother1D<float> sm;
    mAllocVarLenArr( float, arr1dinput, zsize );
    float* arr1doutput = angledata.getData();
    for ( int ofsidx=0; ofsidx<offsetsize; ofsidx++ )
    {
	memcpy( arr1dinput, arr1doutput, zsize*sizeof(float) );
	sm.setInput( arr1dinput, zsize );
	sm.setOutput( arr1doutput );
	sm.setWindow( windowname, smoothingparam, smoothinglength );
	sm.execute();

	arr1doutput = arr1doutput + zsize;
    }
}


bool AngleComputer::fillandInterpArray( Array2D<float>& angledata )
{
    const RayTracer1D* rt = curRayTracer();
    const ElasticModel& curem = curElasticModel();
    if ( !rt )
	return false;

    TypeSet<float> offsets;
    outputsampling_.getPositions( true, offsets );

    const int offsetsize = outputsampling_.nrPts( true );
    const int zsize = outputsampling_.nrPts( false );
    const StepInterval<double> outputzrg = outputsampling_.range( false );
    ManagedObjectSet<PointBasedMathFunction> anglevals;

    TimeDepthModel td;
    rt->getTDModel( 0, td );
    for ( int ofsidx=0; ofsidx<offsetsize; ofsidx++ )
    {
	anglevals += new PointBasedMathFunction( 
				    PointBasedMathFunction::Linear,
				    PointBasedMathFunction::ExtraPolGradient );

	if ( offsets[ofsidx] )
	    anglevals[ofsidx]->add( 0, M_PI_2 );
	else
	    anglevals[ofsidx]->add( 0, 0 );

	float depth = mCast( float, -1.0 * SI().seismicReferenceDatum() );
	for ( int layeridx=0; layeridx<curem.size(); layeridx++ )
	{
	    depth += curem[layeridx].thickness_;
	    float sinangle = rt->getSinAngle(layeridx,ofsidx);
	    if ( mIsUdf(sinangle) || fabs(sinangle) > 1.001f )
		continue;

	    if ( fabs(sinangle) > 1.0f )
		sinangle = sinangle > 0.f ? 1.0f : -1.0f;

	    const float angle = asin(sinangle);
	    anglevals[ofsidx]->add( depth, angle );
	}

	for ( int zidx=0; zidx<zsize; zidx++ )
	{
	    const float layerz = outputzrg.atIndex( zidx );
	    const float depth = SI().zDomain().isTime() ? td.getDepth( layerz )
							: layerz;
	    const float angle = anglevals[ofsidx]->getValue( depth );
	    angledata.set( ofsidx, zidx, angle );
	}
    }

    return true;
}


Gather* AngleComputer::computeAngleData()
{
    PreStack::Gather* gather = new PreStack::Gather( outputsampling_ );
    Array2D<float>& angledata = gather->data();

    if ( needsraytracing_ )
    {
	if ( !raytracer_ )
	{
	    IOPar iopar;
	    iopar.set( sKey::Type(), VrmsRayTracer1D::sFactoryKeyword() );
	    BufferString errormsg;
	    raytracer_ = RayTracer1D::createInstance( iopar, errormsg );
	}

	raytracer_->setup().doreflectivity( false );
	raytracer_->setModel( curElasticModel() );
	TypeSet<float> offsets;
	outputsampling_.getPositions( true, offsets );
	raytracer_->setOffsets( offsets );
	if ( !raytracer_->execute() )
	    return 0;
    }

    if ( !fillandInterpArray(angledata) )
	return 0;
    
    int smtype;
    iopar_.get( sKeySmoothType(), smtype );

    if ( smtype == TimeAverage )
	averageSmooth( angledata );
    else if ( smtype == FFTFilter )
	fftSmooth( angledata );

    return gather;
}


VelocityBasedAngleComputer::VelocityBasedAngleComputer()
    : velsource_( 0 )
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


bool VelocityBasedAngleComputer::checkAndConvertVelocity( 
			    const float* inpvel, const VelocityDesc& veldesc,
			    const StepInterval<float>& zrange, float* outvel )
{
    const int nrzvals = zrange.nrSteps() + 1;
    if ( veldesc.type_ == VelocityDesc::Avg )
    {
	TypeSet<float> zvals( nrzvals, mUdf(float) );
	for ( int idx=0; idx<nrzvals; idx++ )
	    zvals[idx] = zrange.atIndex( idx );
	
	if ( !computeVint(inpvel,zrange.start,zvals.arr(),nrzvals,outvel) )
	    return false;
    }
    else if ( veldesc.type_ == VelocityDesc::RMS &&
	      !computeDix(inpvel,zrange,nrzvals,outvel) )
	      return false;
    
    return true;
}


bool VelocityBasedAngleComputer::createElasticModel( 
					    const StepInterval<float>& zrange, 
					    const float* pvel ) 
{
    elasticmodel_.setEmpty();
    const bool zit =  SI().zDomain().isTime();
    const int zsize = zrange.nrSteps();
    const float svel(mUdf(float)), den(mUdf(float));

    const float srddepth = -1.0f * (float) SI().seismicReferenceDatum();

    int firstidx = 0; float firstlayerthickness;
    if ( zrange.start < srddepth )
    {
	firstidx = zrange.getIndex( srddepth );
	if ( firstidx < 0 )
	    firstidx = 0;

	firstlayerthickness = 
		zit ? (zrange.atIndex(firstidx+1)-srddepth)*pvel[firstidx]/2.0f
		    :  zrange.atIndex(firstidx+1)-srddepth;
    }
    else 
	firstlayerthickness = 
		zit ? (zrange.start+zrange.step-srddepth)*pvel[firstidx]/2.0f 
		    :  zrange.start+zrange.step-srddepth;
    
    ElasticLayer firstlayer( firstlayerthickness, pvel[firstidx], svel, den );
    elasticmodel_ += firstlayer;

    for ( int idx=firstidx+1; idx<zsize; idx++ )
    {
	const float layerthickness = zit ? zrange.step*pvel[idx]/2.0f
					 : zrange.step;

	ElasticLayer elayer( layerthickness, pvel[idx], svel, den );
	elasticmodel_ += elayer;
    }

    if ( elasticmodel_.isEmpty() )
	return false;

    elasticmodel_.block( thresholdparam_, true );
    elasticmodel_.setMaxThickness( maxthickness_ );
    return true;
}


Gather* VelocityBasedAngleComputer::computeAngles()
{
    ConstRefMan<Survey::Geometry> geom =
	SI().geomManager().getGeometry( trcid_.geomid_ );
    
    if ( geom->is2D() )
    {
	pErrMsg( "Only 3D is supported at this time" );
	return 0;
    }
    
    RefMan<Vel::FunctionSource> source = velsource_;
    if ( !source )
	return 0;

    ConstRefMan<Vel::Function> func =
	source->getFunction( BinID(trcid_.lineNr(),trcid_.trcNr()) );
    if ( !func )
	return 0;
    
    VelocityDesc veldesc = func->getDesc();
    if ( !veldesc.isVelocity() )
	return 0;

    const StepInterval<float> desiredzrange = func->getDesiredZ();
    StepInterval<float> zrange = func->getAvailableZ();
    zrange.limitTo( desiredzrange );

    const int zsize = zrange.nrSteps() + 1;
    TypeSet<float> vel;
    vel.setCapacity( zsize );
    for( int idx=0; idx<zsize; idx++ )
	vel += func->getVelocity( zrange.atIndex(idx) );
    
    if ( veldesc.type_ != VelocityDesc::Interval )
    {
	mAllocVarLenArr( float, velocityvalues, zsize );
	if ( !checkAndConvertVelocity(vel.arr(),veldesc,zrange,velocityvalues) )
	    return 0;

	if ( !createElasticModel(zrange,velocityvalues) )
	    return 0;
    }
    else if ( !createElasticModel(zrange,vel.arr()) )
	return 0;

    return computeAngleData();
}


const ElasticModel& ModelBasedAngleComputer::ModelTool
					   ::elasticModel() const
{ return rt_ ? rt_->getModel() : *em_; } 


ModelBasedAngleComputer::ModelBasedAngleComputer()
    : AngleComputer()
{
}


void ModelBasedAngleComputer::setElasticModel( const TraceID& trcid,
					       bool block, bool pvelonly,
       					       ElasticModel& em	)
{
    if ( block )
    {
	ElasticModel rawem;
	em.block( thresholdparam_, pvelonly );
	em.setMaxThickness( maxthickness_ );
    }

    ModelTool* tool = new ModelTool( em, trcid );
    const int toolidx = tools_.indexOf( tool );
    if ( toolidx<0 )
	tools_ += tool;
    else
	delete tools_.replace( toolidx, tool );
}


void ModelBasedAngleComputer::setRayTracer( const RayTracer1D* rt,
					    const TraceID& trcid )
{
    ModelTool* tool = new ModelTool( rt, trcid );
    const int toolidx = tools_.indexOf( tool );
    if ( toolidx<0 )
	tools_ += tool;
    else
	delete tools_.replace( toolidx, tool );
    needsraytracing_ = false;
}


const ElasticModel& ModelBasedAngleComputer::curElasticModel() const
{
    for ( int idx=0; idx<tools_.size(); idx++ )
	if ( tools_[idx]->trcID() == trcid_ )
	    return tools_[idx]->elasticModel();
    return elasticmodel_;
}


const RayTracer1D* ModelBasedAngleComputer::curRayTracer() const
{
    if ( raytracer_ ) return raytracer_;
    for ( int idx=0; idx<tools_.size(); idx++ )
	if ( tools_[idx]->trcID() == trcid_ )
	    return tools_[idx]->rayTracer();
    return 0;
}


Gather* ModelBasedAngleComputer::computeAngles()
{
    return computeAngleData();
}

