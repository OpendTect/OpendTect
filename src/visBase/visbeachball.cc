/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Karthika
 Date:          Aug 2009
________________________________________________________________________

-*/
static const char* rcsID = "$Id: visbeachball.cc,v 1.2 2009-08-12 15:57:49 cvskarthika Exp $";

#include "visbeachball.h"
#include "vistransform.h"

#include "iopar.h"

#include "SoBeachBall.h"
#include "color.h"
#include "SoShapeScale.h"
#include "UTMPosition.h"

#include <Inventor/nodes/SoSeparator.h>
#include <Inventor/nodes/SoScale.h>
#include <Inventor/nodes/SoTranslation.h>

mCreateFactoryEntry( visBase::BeachBall );

namespace visBase
{

const char* BeachBall::radiusstr()	{ return "Radius"; }


BeachBall::BeachBall()
    : VisualObjectImpl( false )
    , ball_( 0 )
    , shape_( 0 )
    , translation_( new SoTranslation )
    , xytranslation_( 0 )
    , scale_( new SoShapeScale )
    , transformation_( 0 )
{
    addChild( translation_ );
    addChild( scale_ );
    ball_ = new SoBeachBall();
//    setScreenSize( cDefaultScreenSize() );
}


BeachBall::~BeachBall()
{
     if ( transformation_ ) transformation_->unRef();
}


void BeachBall::fillPar( IOPar& par, TypeSet<int>& saveids ) const
{
    VisualObjectImpl::fillPar( par, saveids );

    par.set( radiusstr(), getRadius() );
}


int BeachBall::usePar( const IOPar& par )
{
    int res = VisualObjectImpl::usePar( par );
    if ( res!=1 ) return res;

    float rd = getRadius();
    par.get( radiusstr(), rd );
    setRadius( rd );

    return 1;
}


void BeachBall::setCenterPosition( Coord3 c )
{
    Coord3 pos( c );
    
    if ( transformation_ ) pos = transformation_->transform( pos );
/*
    if ( !xytranslation && (fabs(pos.x)>1e5 || fabs(pos.y)>1e5) )
    {
	xytranslation = new UTMPosition;
	insertChild( childIndex( translation ), xytranslation );
    }

    if ( xytranslation )
    {
	xytranslation->utmposition.setValue( pos.x, pos.y, 0 );
	pos.x = 0; pos.y = 0;
    }*/
    translation_->translation.setValue( pos.x, pos.y, pos.z );
}


Coord3 BeachBall::getCenterPosition() const
{
    Coord3 res;
    SbVec3f pos = translation_->translation.getValue();

/*    if ( xytranslation )
    {
	res.x = xytranslation->utmposition.getValue()[0];
	res.y = xytranslation->utmposition.getValue()[1];
    }
    else*/
    {
	res.x = pos[0];
	res.y = pos[1];
    }

    res.z = pos[2];

    return res;
}


void BeachBall::setRadius( float r )
{
    // to do!
}


float BeachBall::getRadius() const
{
    // to do!
    return 1;
}


void BeachBall::setColor1( Color col )
{
    // to do! create a material and set the color
}


Color BeachBall::getColor1() const
{
    // to do!
    Color c(0, 0, 0);
    return c;
}


void BeachBall::setColor2( Color col )
{
    // to do! create a material and set the color
}


Color BeachBall::getColor2() const
{
    // to do!
    Color c(0, 0, 0);
    return c;
}


Transformation* BeachBall::getDisplayTransformation()
{ 
    return transformation_; 
}


void BeachBall::setDisplayTransformation( Transformation* nt )
{
    const Coord3 pos = getCenterPosition();
    if ( transformation_ ) transformation_->unRef();
    transformation_ = nt;
    if ( transformation_ ) transformation_->ref();
    setCenterPosition( pos );
}


} // namespace visBase
