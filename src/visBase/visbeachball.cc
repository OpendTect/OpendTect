/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Karthika
 Date:          Aug 2009
________________________________________________________________________

-*/
static const char* rcsID = "$Id: visbeachball.cc,v 1.4 2009-08-17 15:20:05 cvskarthika Exp $";

#include "visbeachball.h"
#include "vistransform.h"

#include "iopar.h"

#include "SoBeachBall.h"
#include "color.h"
#include "SoShapeScale.h"
#include "UTMPosition.h"
#include "visdrawstyle.h"
#include <Inventor/nodes/SoSeparator.h>
#include <Inventor/nodes/SoScale.h>
#include <Inventor/nodes/SoMaterial.h>
#include <Inventor/nodes/SoMaterialBinding.h>
#include <Inventor/nodes/SoTranslation.h>

mCreateFactoryEntry( visBase::BeachBall );

namespace visBase
{

const char* BeachBall::radiusstr()	{ return "Radius"; }


BeachBall::BeachBall()
    : VisualObjectImpl(true) 
    , ball_(new SoBeachBall)
    , material_(new SoMaterial)
    , translation_(new SoTranslation)
    , xytranslation_(0)
    , scale_(new SoShapeScale)
    , style_ (0)
    , transformation_(0)
{
    material_->ref();
    material_->diffuseColor.setNum( 2 );
    material_->diffuseColor.set1Value( 0, SbColor( 1, 1, 1 ) );
    material_->diffuseColor.set1Value( 1, SbColor( 1, 0, 0 ) );
    addChild( material_ );

    SoMaterialBinding* matbinding = new SoMaterialBinding;
    matbinding->ref();
    matbinding->value = SoMaterialBinding::PER_PART;
    addChild( matbinding );
    
    translation_->ref();
    addChild( translation_ );
    
    scale_->ref();
//    scale_->restoreProportions = true;
//    scale_->screenSize.setValue( 5 ); // to do: check! cDefaultScreenSize();
    addChild( scale_ );

    style_ = DrawStyle::create();
    style_->ref();
    style_->setDrawStyle( DrawStyle::Filled );
    addChild( style_->getInventorNode() );

    ball_->ref();
    addChild( ball_ );
}


BeachBall::~BeachBall()
{
    material_->unref();
    translation_->unref();
    scale_->unref();
    //transformation_->unRef(); 
    // transformation_ is deleted even before control reaches here!
//    ball_->unref();
    // cannot unref scale_ and transformation_.
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


void BeachBall::setCenterPosition( Coord3 c )
{
    Coord3 pos( 607903, 6077213, 0.5 );
//    Coord3 pos;
    
    if ( transformation_ ) pos = transformation_->transform( pos );

    if ( !xytranslation_ && (fabs(pos.x)>1e5 || fabs(pos.y)>1e5) )
    {
	xytranslation_ = new UTMPosition;
	insertChild( childIndex( translation_ ), xytranslation_ );
    }

    if ( xytranslation_ )
    {
	xytranslation_->utmposition.setValue( pos.x, pos.y, 0 );
	pos.x = 0; pos.y = 0;
    }
    translation_->translation.setValue( pos.x, pos.y, pos.z );
}


Coord3 BeachBall::getCenterPosition() const
{
    Coord3 res;
    SbVec3f pos = translation_->translation.getValue();

    if ( xytranslation_ )
    {
	res.x = xytranslation_->utmposition.getValue()[0];
	res.y = xytranslation_->utmposition.getValue()[1];
    }
    else
    {
	res.x = pos[0];
	res.y = pos[1];
    }

    res.z = pos[2];

    return res;
}


void BeachBall::setRadius( float r )
{
    // to do! check
    scale_->screenSize.setValue( r );
}


float BeachBall::getRadius() const
{
    // to do! check
    return scale_->screenSize.getValue();
}


void BeachBall::setColor1( Color col )
{
    material_->diffuseColor.set1Value( 0, col.r(), col.g(), col.b() );
}


Color BeachBall::getColor1() const
{
    SbColor col = material_->diffuseColor[0];
    return Color( col[0], col[1], col[2] );
}


void BeachBall::setColor2( Color col )
{
    material_->diffuseColor.set1Value( 1, col.r(), col.g(), col.b() );
}


Color BeachBall::getColor2() const
{
    SbColor col = material_->diffuseColor[1];
    return Color( col[0], col[1], col[2] );
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


} // namespace visBase
