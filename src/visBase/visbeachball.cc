/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Karthika
 Date:          Aug 2009
________________________________________________________________________

-*/
static const char* rcsID mUsedVar = "$Id$";

#include "visbeachball.h"
#include "vistransform.h"
#include "color.h"
#include "UTMPosition.h"
#include "iopar.h"
#include "beachballdata.h"

#include "SoBeachBall.h"

#include <Inventor/nodes/SoSeparator.h>
#include <Inventor/nodes/SoScale.h>
#include <Inventor/nodes/SoDrawStyle.h>
#include <Inventor/nodes/SoMaterial.h>
#include <Inventor/nodes/SoMaterialBinding.h>
#include <Inventor/nodes/SoTranslation.h>
#include <Inventor/nodes/SoComplexity.h>

mCreateFactoryEntry( visBase::BeachBall );

namespace visBase
{

const char* BeachBall::radiusstr()	{ return "Radius"; }
const char* BeachBall::centerstr()	{ return "Center"; }
const char* BeachBall::color1str()	{ return "Color1"; }
const char* BeachBall::color2str()	{ return "Color2"; }


BeachBall::BeachBall()
    : VisualObjectImpl(true) 
    , ball_(new SoBeachBall)
    , material_(new SoMaterial)
    , matbinding_(new SoMaterialBinding)
    , complexity_(new SoComplexity)
    , translation_(new SoTranslation)
    , xyTranslation_(0)
    , scale_(new SoScale)
    , transformation_(0)
    , radius_(1)
    , zScale_(2)
{
    material_->ref();
    material_->diffuseColor.setNum( 2 );
    material_->diffuseColor.set1Value( 0, SbColor( 1, 1, 1 ) );
    material_->diffuseColor.set1Value( 1, SbColor( 1, 0, 0 ) );
    addChild( material_ );

    matbinding_->ref();
    matbinding_->value = SoMaterialBinding::PER_PART;
    addChild( matbinding_ );
    
    complexity_->ref();
    complexity_->type = SoComplexity::SCREEN_SPACE;
    complexity_->value = 1.0;
    
    addChild( complexity_ );

    translation_->ref();
    addChild( translation_ );
    
    scale_->ref();
    addChild( scale_ );
    
    ball_->ref();
    // Specify the materialIndex of ball_ if matbinding is PER_PART_INDEXED.
    addChild( ball_ );
}


BeachBall::~BeachBall()
{
    material_->unrefNoDelete();
    matbinding_->unrefNoDelete();
    complexity_->unrefNoDelete();
    translation_->unrefNoDelete();
    scale_->unrefNoDelete();
    if (xyTranslation_)
	xyTranslation_->unrefNoDelete();
    if (transformation_)
	transformation_->unRef(); 
    ball_->unrefNoDelete();
}


const mVisTrans* BeachBall::getDisplayTransformation() const
{ 
    return transformation_; 
}


void BeachBall::setDisplayTransformation( const mVisTrans* nt )
{
    if ( transformation_ == nt )
	return;

    Coord3 pos = getCenterPosition();
    if ( transformation_ )
	transformation_->unRef();
    transformation_ = nt;
    if ( transformation_ ) transformation_->ref();
    setCenterPosition( pos );
}


void BeachBall::setZScale( float zScale )
{
    if (zScale == 0)
	return;
    scale_->scaleFactor.setValue( radius_, radius_, radius_/(2*zScale) );
    	// factor of 2 for 2-way time
    zScale_ = zScale;
}


float BeachBall::getZScale() const
{
    return zScale_;
}



void BeachBall::setCenterPosition( const Coord3& c )
{
    Coord3 pos( c );
    
    if ( transformation_ ) pos = transformation_->transform( c );

    if ( !xyTranslation_ && (fabs(pos.x)>1e5 || fabs(pos.y)>1e5) )
    {
	xyTranslation_ = new UTMPosition;
	insertChild( childIndex( translation_ ), xyTranslation_ );
    }

    if ( xyTranslation_ )
    {
	xyTranslation_->utmposition.setValue( pos.x, pos.y, 0 );
	pos.x = 0; pos.y = 0;
    }
    translation_->translation.setValue( (float) pos.x, 
				    (float) pos.y, (float) pos.z );
}


Coord3 BeachBall::getCenterPosition() const
{
    Coord3 res;
    SbVec3f pos = translation_->translation.getValue();

    if ( xyTranslation_ )
    {
	res.x = xyTranslation_->utmposition.getValue()[0];
	res.y = xyTranslation_->utmposition.getValue()[1];
    }
    else
    {
	res.x = pos[0];
	res.y = pos[1];
    }

    res.z = pos[2];
    if ( transformation_ ) res = transformation_->transformBack( res );
    return res;
}


void BeachBall::setRadius( float r )
{
    scale_->scaleFactor.setValue( r, r, r/zScale_ );
    radius_ = r;
}


float BeachBall::getRadius() const
{
    return radius_;
}


void BeachBall::setColor1( Color col )
{
    float r, g, b;
    r = col.r()/255.0f;
    g = col.g()/255.0f;
    b = col.b()/255.0f;
    material_->diffuseColor.set1Value( 0, r, g, b );
}


Color BeachBall::getColor1() const
{
    SbColor col = material_->diffuseColor[0];
    return Color( (unsigned char) (col[0]*255), (unsigned char) (col[1]*255), 
	    (unsigned char) (col[2]*255) );
}


void BeachBall::setColor2( Color col )
{
    float r, g, b;
    r = col.r()/255.0f;
    g = col.g()/255.0f;
    b = col.b()/255.0f;
    material_->diffuseColor.set1Value( 1, r, g, b );
}


Color BeachBall::getColor2() const
{
    SbColor col = material_->diffuseColor[1];
    return Color( (unsigned char) (col[0]*255), (unsigned char) (col[1]*255), 
	    (unsigned char) (col[2]*255) );
}


void BeachBall::fillPar( IOPar& par, TypeSet<int>& saveids ) const
{
    VisualObjectImpl::fillPar( par, saveids );

    par.set( radiusstr(), getRadius() );
    par.set( centerstr(), getCenterPosition() );
    par.set( color1str(), getColor1() );
    par.set( color2str(), getColor2() );
}


int BeachBall::usePar( const IOPar& par )
{
    pErrMsg("In usePar!");
    int res = VisualObjectImpl::usePar( par );
    if ( res != 1 ) return res;

    float rd = getRadius();
    if ( !par.get( radiusstr(), rd ) )
	return -1;
    setRadius( rd );

    Coord3 center = getCenterPosition();
    if ( !par.get( centerstr(), center ) )
	return -1;
    setCenterPosition( center );

    Color col = getColor1();
    if ( !par.get( color1str(), col ) )
	return -1;
    setColor1( col );

    col = getColor2();
    if ( !par.get( color2str(), col ) )
	return -1;
    setColor2( col );

    return 1;
}


void BeachBall::setBallProperties(const visBeachBall::BallProperties& bp)
{
    setRadius( bp.radius() );
    setColor1( bp.color1() );
    setColor2( bp.color2() );
    setCenterPosition( bp.pos() );
    // to do: use the elasticity later!
}


visBeachBall::BallProperties BeachBall::getBallProperties() const
{
    visBeachBall::BallProperties bp( "", getRadius(), getColor1(), getColor2(),
	    getCenterPosition() );
    // to do: use the elasticity later!
    return bp;
}


} // namespace visBase
