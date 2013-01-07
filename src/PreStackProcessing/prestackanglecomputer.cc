/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : Y. Liu
 * DATE     : January 2010
-*/

static const char* rcsID mUsedVar = "$Id$";

#include "prestackanglecomputer.h"

#include "survinfo.h"
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


Gather* AngleComputer::computeAngles( const TraceID& trcid )
{
    RefMan<const Survey::Geometry> geom =
	SI().geomManager().getGeomety( trcid.geomid_ );
    
    if ( !geom->is3D() )
    {
	pErrMsg( "Only 3D is supported at this time" );
	return 0;
    }
    
    RefMan<Vel::FunctionSource> source = velsource_;
    if ( !source )
	return 0;
    
    RefMan<const Vel::Function> func =
		source->getFunction( BinID(trcid.line_,trcid.trcnr_) );
    if ( !func )
	return 0;
    
    //Check type of velocity, and convert if necessary
    
    //Block velocity to say 20 points
    
    //For each offset
    // Raytrace
    // Fill offset column in array with angle values. Interpolate between pts
    
    
    return 0;
}
