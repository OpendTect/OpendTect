/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : Y. Liu
 * DATE     : January 2010
-*/

static const char* rcsID mUsedVar = "$Id$";

#include "prestackanglecomputer.h"

#include "array1dinterpol.h"
#include "arrayndslice.h"
#include "prestackgather.h"
#include "raytrace1d.h"
#include "survinfo.h"
#include "velocitycalc.h"
#include "velocityfunction.h"

using namespace PreStack;

AngleComputer::AngleComputer()
    : velsource_( 0 )
{}


AngleComputer::~AngleComputer()
{
    if ( velsource_ ) velsource_->unRef();
}


bool AngleComputer::setMultiID( const MultiID& mid )
{
    if ( velsource_ ) velsource_->unRef();
    velsource_ = Vel::FunctionSource::factory().create( 0, mid );
    if ( velsource_ ) velsource_->ref();
    
    return velsource_;
}


void AngleComputer::setOutputSampling( const FlatPosData& os )
{ outputsampling_  = os; }


void createElasticModel( ElasticModel& em, const Vel::Function* fun )
{
    StepInterval<float> availz = fun->getAvailableZ();
    const int nrlayers = 20;
    float layerthickness = availz.width() / nrlayers;
    em.erase();
    //Block velocity to say 20 points
    em.setCapacity( nrlayers );
    float pvel(mUdf(float)), svel(mUdf(float)), den(mUdf(float)); 
    for ( int layernum=0; layernum<nrlayers; layernum++ )
    {
	pvel = fun->getVelocity( layernum * layerthickness + layerthickness/2 );
	ElasticLayer elayer( layerthickness, pvel, svel, den );
	em.add( elayer );
    }
}


bool fillandInterpArray( Array2D<float>& array2d, const RayTracer1D* raytracer,
		         const FlatPosData& outputsamp, const ElasticModel& em )
{
    TypeSet<float> offsets;
    outputsamp.getPositions( true, offsets );
    const StepInterval<double>& outputzrg = outputsamp.range( false );
    Array2DInfoImpl array2dinfo;
    array2dinfo.setSize( 0, offsets.size() );
    array2dinfo.setSize( 1, outputzrg.nrSteps()+1 );
    array2d.setInfo( array2dinfo );
    array2d.setAll( mUdf(float) );

    const float layerthickness = em[0].thickness_;
    for ( int ofsidx=0; ofsidx<offsets.size(); ofsidx++ )
    {
	for ( int layeridx=0; layeridx<em.size(); layeridx++ )
	{
	    const float angle = asin( raytracer->getSinAngle(layeridx,ofsidx) );
	    const float layerz = layeridx * layerthickness + layerthickness/2;
	    const int zidx = outputzrg.getIndex( layerz );
	    array2d.set( ofsidx, zidx, angle ); 
	}

	Array1DSlice<float> array1dslice( array2d );
	array1dslice.setDimMap( 0 , 1 );
	array1dslice.setPos( 0, ofsidx );
	LinearArray1DInterpol array1dinterp;
	array1dinterp.setArray( array1dslice );
	if ( !array1dinterp.execute() )
	    return false;
    }

    return true;
}


Gather* AngleComputer::computeAngles( const TraceID& trcid )
{
    RefMan<const Survey::Geometry> geom =
	SI().geomManager().getGeometry( trcid.geomid_ );
    
    if ( geom->is2D() )
    {
	pErrMsg( "Only 3D is supported at this time" );
	return 0;
    }
    
    RefMan<Vel::FunctionSource> source = velsource_;
    if ( !source )
	return 0;
    
    RefMan<const Vel::Function> func =
		source->getFunction( BinID(trcid.lineNr(),trcid.trcNr()) );
    if ( !func )
	return 0;
    
    //Check type of velocity, and convert if necessary
    VelocityDesc veldesc = func->getDesc();
    if ( !veldesc.isVelocity() )
	return 0;

    if ( veldesc.type_ != VelocityDesc::RMS )
	return 0; //TODO Convert if not Vrms.

    IOPar iopar;
    iopar.set( sKey::Type(), VrmsRayTracer1D::sFactoryKeyword() );
    TypeSet<float> offsets;
    outputsampling_.getPositions( true, offsets );
    iopar.set( sKey::Offset(), offsets );
    BufferString errormsg;
    RayTracer1D* raytracer = RayTracer1D::createInstance( iopar, errormsg );

    ElasticModel em;
    createElasticModel( em, func );
    raytracer->setModel( em );
    //For each offset
    // Raytrace
    if ( !raytracer->execute() )
	return 0;

    PreStack::Gather* gather = new PreStack::Gather;
    Array2D<float>& array2d = gather->data();
    // Fill offset column in array with angle values. Interpolate between pts
    if ( !fillandInterpArray(array2d,raytracer,outputsampling_,em) )
	return 0;

    return gather;
    //return 0;
}
