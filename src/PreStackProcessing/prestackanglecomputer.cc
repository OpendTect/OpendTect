/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : Y. Liu
 * DATE     : January 2010
-*/

static const char* rcsID mUsedVar = "$Id$";

#include "prestackanglecomputer.h"

#include "mathfunc.h"
#include "position.h"
#include "prestackgather.h"
#include "raytrace1d.h"
#include "smoother1d.h"
#include "survinfo.h"
#include "velocitycalc.h"
#include "velocityfunction.h"

using namespace PreStack;


AngleComputer::AngleComputer()
    : thresholdparam_(0.01)
    , raytracer_(0)
    , needsraytracing_(true)
{
    maxthickness_ = SI().depthsInFeet() ? 165.0f : 50.0f;
    iopar_.set( sKeySmoothType(), TimeAverage );
    iopar_.set( sKeyWinFunc(), HanningWindow::sName() );
    iopar_.set( sKeyWinParam(), 0.95f );
    iopar_.set( sKeyWinLen(), 500 );
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


void AngleComputer::fftSmoothing( Array2D<float>& angledata )
{
    //TODO : Implement FFT Based Low Pass Filter;
}


void AngleComputer::averageSmoothing( Array2D<float>& angledata )
{
    BufferString windowname;
    float smoothingparam; int smoothinglength;

    iopar_.get( sKeyWinFunc(), windowname );
    iopar_.get( sKeyWinParam(), smoothingparam );
    iopar_.get( sKeyWinLen(), smoothinglength );

    const int offsetsize = outputsampling_.nrPts( true );
    const int zsize = outputsampling_.nrPts( false );

    Smoother1D<float> sm;
    mAllocVarLenArr( float, arr1dinp, zsize );
    float* arr1dout = angledata.getData();
    for ( int ofsidx=0; ofsidx<offsetsize; ofsidx++ )
    {
	memcpy( arr1dinp, arr1dout, zsize*sizeof(int) );
	sm.setInput( arr1dinp, zsize );
	sm.setOutput( arr1dout );
	sm.setWindow( windowname, smoothingparam, smoothinglength );
	sm.execute();

	arr1dout = arr1dout + zsize;
    }
}


bool AngleComputer::fillandInterpArray( Array2D<float>& angledata )
{
    if ( !raytracer_ )
	return false;

    TypeSet<float> offsets;
    outputsampling_.getPositions( true, offsets );

    const int offsetsize = outputsampling_.nrPts( true );
    const int zsize = outputsampling_.nrPts( false );
    const StepInterval<double> outputzrg = outputsampling_.range( false );
    ManagedObjectSet<PointBasedMathFunction> anglevals;

    TimeDepthModel td;
    raytracer_->getTDModel( 0, td );
    for ( int ofsidx=0; ofsidx<offsetsize; ofsidx++ )
    {
	anglevals += new PointBasedMathFunction( 
				    PointBasedMathFunction::Linear,
				    PointBasedMathFunction::ExtraPolGradient );

	if ( offsets[ofsidx] )
	    anglevals[ofsidx]->add( 0, M_PI_2 );
	else
	    anglevals[ofsidx]->add( 0, 0 );

	int prevzidx=0; float layerz = 0; float depth = 0;
	for ( int layeridx=0; layeridx<elasticmodel_.size(); layeridx++ )
	{
	    depth += elasticmodel_[layeridx].thickness_;
	    layerz = SI().zDomain().isTime() ? td.getTime( depth ) : depth;

	    int zidx = outputzrg.getIndex( layerz );
	    if ( zidx == prevzidx )
		continue;

	    const float angle = asin(raytracer_->getSinAngle(layeridx,ofsidx));
	    anglevals[ofsidx]->add( (float)zidx, angle );
	    prevzidx = zidx;
	}

	for ( int zidx=0; zidx<zsize; zidx++ )
	{
	    float angle = anglevals[ofsidx]->getValue( (float)zidx );
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
	raytracer_->setModel( elasticmodel_ );
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
	averageSmoothing( angledata );
    else
	fftSmoothing( angledata );

    return gather;
}


VelocityBasedAngleComputer::VelocityBasedAngleComputer()
    : velsource_( 0 )
    , trcid_(trcid_.std3DGeomID(),0,0)
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
    elasticmodel_.erase();
    const bool zit =  SI().zDomain().isTime();
    const int zsize = zrange.nrSteps();
    ElasticModel firstrawem, secondrawem;
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
    
    ElasticLayer firstelayer( firstlayerthickness, pvel[firstidx], svel, den );
    firstrawem.add( firstelayer );

    for ( int idx=firstidx+1; idx<zsize; idx++ )
    {
	const float layerthickness = zit ? zrange.step*pvel[idx]/2.0f
					 : zrange.step;

	ElasticLayer elayer( layerthickness, pvel[idx], svel, den );
	firstrawem.add( elayer );
    }

    if ( firstrawem.isEmpty() )
	return false;
    
    BlockElasticModel( firstrawem, secondrawem, thresholdparam_, true );
    SetMaxThicknessElasticModel( secondrawem, elasticmodel_, maxthickness_ );
    return true;
}


Gather* VelocityBasedAngleComputer::computeAngles()
{
    RefMan<const Survey::Geometry> geom =
	SI().geomManager().getGeometry( trcid_.geomid_ );
    
    if ( geom->is2D() )
    {
	pErrMsg( "Only 3D is supported at this time" );
	return 0;
    }
    
    RefMan<Vel::FunctionSource> source = velsource_;
    if ( !source )
	return 0;

    RefMan<const Vel::Function> func =
	source->getFunction( BinID(trcid_.lineNr(),trcid_.trcNr()) );
    if ( !func )
	return 0;
    
    VelocityDesc veldesc = func->getDesc();
    if ( !veldesc.isVelocity() )
	return 0;

    const StepInterval<float> availz = func->getAvailableZ();
    const int availzsize = availz.nrSteps() + 1;
    TypeSet<float> vel;
    vel.setCapacity( availzsize );
    for( int idx=0; idx<availzsize; idx++ )
	vel += func->getVelocity( availz.atIndex(idx) );
    
    if ( veldesc.type_ != VelocityDesc::Interval )
    {
	mAllocVarLenArr( float, velocityvalues, availzsize );
	if ( !checkAndConvertVelocity(vel.arr(),veldesc,availz,velocityvalues) )
	    return 0;

	if ( !createElasticModel(availz,velocityvalues) )
	    return 0;
    }
    else if ( !createElasticModel(availz,vel.arr()) )
	return 0;

    return computeAngleData();
}


void ModelBasedAngleComputer::setElasticModel( ElasticModel& em, 
					       bool block, bool pvelonly )
{
    if ( block )
    {
	ElasticModel rawem;
	BlockElasticModel( em, rawem, thresholdparam_, pvelonly );
	SetMaxThicknessElasticModel( rawem, em, maxthickness_ );
    }

    elasticmodel_ = em;
}


void ModelBasedAngleComputer::setRayTracer( RayTracer1D* rt )
{
    raytracer_ = rt;
    if ( raytracer_ )
	needsraytracing_ = false;
}


Gather* ModelBasedAngleComputer::computeAngles()
{
    return computeAngleData();
}

