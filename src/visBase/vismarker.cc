/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        N. Hemstra
 Date:          July 2002
________________________________________________________________________

-*/
static const char* rcsID mUsedVar = "$Id$";

#include "vismarker.h"

#include "iopar.h"
#include "survinfo.h"
#include "viscube.h"
#include "vistransform.h"

#include "SoShapeScale.h"
#include "SoArrow.h"
#include "UTMPosition.h"

#include <Inventor/nodes/SoCube.h>
#include <Inventor/nodes/SoSphere.h>
#include <Inventor/nodes/SoCylinder.h>
#include <Inventor/nodes/SoCone.h>
#include <Inventor/nodes/SoRotation.h>
#include <Inventor/nodes/SoSeparator.h>
#include <Inventor/nodes/SoScale.h>
#include <Inventor/nodes/SoTranslation.h>

#include <math.h>

static float cDipFactor() { return SI().zIsTime() ? 1e-6f : 1e-3f; }

mCreateFactoryEntry( visBase::Marker );

namespace visBase
{

const char* Marker::centerposstr = "Center Pos";

Marker::Marker()
    : VisualObjectImpl(true)
    , transformation(0)
    , xytranslation( 0 )
    , translation(new SoTranslation)
    , rotation(0)
    , markerscale(new SoShapeScale)
    , shape(0)
    , direction(0.5,M_PI_2,0)
    , zstretch_(1)
    , inldip_(0)
    , crldip_(0)
{
    addChild( translation );
    addChild( markerscale );
    setType( MarkerStyle3D::Cube );

    markerscale->restoreProportions = true;
    markerscale->dorotate = true;
    setScreenSize( cDefaultScreenSize() );
}


Marker::~Marker()
{
    if ( transformation ) transformation->unRef();
    if ( shape ) removeChild(shape);
}


void Marker::setCenterPos( const Coord3& pos_ )
{
    Coord3 pos( pos_ );

    if ( transformation ) pos = transformation->transform( pos );

    if ( !xytranslation && (fabs(pos.x)>1e5 || fabs(pos.y)>1e5) )
    {
	xytranslation = new UTMPosition;
	insertChild( childIndex( translation ), xytranslation );
    }

    if ( xytranslation )
    {
	xytranslation->utmposition.setValue( pos.x, pos.y, 0 );
	pos.x = 0; pos.y = 0;
    }

    translation->translation.setValue( (float) pos.x, 
				    (float) pos.y, (float) pos.z );
}


Coord3 Marker::centerPos(bool displayspace) const
{
    Coord3 res;
    SbVec3f pos = translation->translation.getValue();


    if ( xytranslation )
    {
	res.x = xytranslation->utmposition.getValue()[0];
	res.y = xytranslation->utmposition.getValue()[1];
    }
    else
    {
	res.x = pos[0];
	res.y = pos[1];
    }

    res.z = pos[2];

    if ( !displayspace && transformation )
	res = transformation->transformBack( res );

    return res;
}


void Marker::setMarkerStyle( const MarkerStyle3D& ms )
{
    setType( ms.type_ );
    setScreenSize( (float)ms.size_ );
}


MarkerStyle3D::Type Marker::getType() const
{
    return markerstyle.type_;
}


static float getSurveyRotation()
{
    const RCol2Coord& b2c = SI().binID2Coord();
    const float xcrd = (float) b2c.getTransform(true).c;
    const float ycrd = (float) b2c.getTransform(false).c;
    const float angle = atan2( ycrd, xcrd );
    return angle;
}


void Marker::setType( MarkerStyle3D::Type type )
{
    switch ( type )
    {
    
    case MarkerStyle3D::Cube: {
	setMarkerShape(new SoCube);
	setRotation( Coord3(0,0,1), 0 );
	} break;
    case MarkerStyle3D::Cone:
	setMarkerShape(new SoCone);
	setRotation( Coord3(1,0,0), M_PI/2 );
	break;
    case MarkerStyle3D::Cylinder:
	setMarkerShape(new SoCylinder);
	setRotation( Coord3(1,0,0), M_PI/2 );
	break;
    case MarkerStyle3D::Sphere:
	setMarkerShape(new SoSphere);
	setRotation( Coord3(0,0,1), 0 );
	break;
    case MarkerStyle3D::Arrow:
	setMarkerShape(new SoArrow);
	setArrowDir( direction );
	break;
    case MarkerStyle3D::Cross:{
	SoGroup* group = new SoGroup;
	group->ref();

	SoCylinder* cyl = new SoCylinder;
	cyl->radius.setValue( 0.2 );
	group->addChild( cyl );

	SoRotation* rot1 = new SoRotation;
	rot1->rotation.setValue( SbVec3f(1,0,0), M_PI_2 );
	group->addChild( rot1 );
	group->addChild( cyl );

	SoRotation* rot2 = new SoRotation;
	rot2->rotation.setValue( SbVec3f(0,0,1), M_PI_2 );
	group->addChild( rot2 );
	group->addChild( cyl );

	setMarkerShape( group );
	group->unref();
	setRotation( Coord3(0,0,1), 0 );
	}break;
    case MarkerStyle3D::Plane: {
	Cube* plane = visBase::Cube::create();
	plane->setWidth( Coord3(6,5,1) );
	setMarkerShape( plane->getInventorNode() ); 
	setDip( inldip_, crldip_ );
	} break;
    case MarkerStyle3D::None:
    default:{
	    removeChild(shape);
	} break;
    }

    markerstyle.type_ = type;
}


void Marker::setMarkerShape(SoNode* newshape)
{
    if ( shape ) removeChild(shape);
    shape = newshape;
    addChild( shape );
}


void Marker::setScreenSize( const float sz )
{
    markerscale->screenSize.setValue( sz );
    markerstyle.size_ = (int)sz;
}


void Marker::setDirection( const ::Sphere& d )
{
    direction = d;
    if ( markerstyle.type_ == MarkerStyle3D::Arrow )
	setArrowDir( direction );
}


float Marker::getScreenSize() const
{
    return markerscale->screenSize.getValue();
}


void Marker::doFaceCamera(bool yn)
{ markerscale->dorotate = yn; }


bool Marker::facesCamera() const
{ return markerscale->dorotate.getValue(); }


void Marker::doRestoreProportions(bool yn)
{ markerscale->restoreProportions = yn; }


bool Marker::restoresProportions() const
{ return markerscale->restoreProportions.getValue(); }


void Marker::setRotation( const Coord3& vec, float angle )
{
    if ( !rotation )
    {
	rotation = new SoRotation;
	insertChild( childIndex( shape ), rotation );
    }

    rotation->rotation.setValue( SbVec3f((float) vec[0],
				    (float) vec[1],(float) vec[2]), angle );
}


void Marker::setArrowDir( const ::Sphere& dir )
{
    mDynamicCastGet(SoArrow*,arrow,shape)
    if ( !arrow ) return;

    Coord3 newcrd = spherical2Cartesian( dir, false );
    newcrd /= dir.radius;

    SbVec3f orgvec(1,0,0);
    SbRotation newrot( orgvec, SbVec3f((float) newcrd.x,
				    (float) newcrd.y,(float) -newcrd.z) );
    if ( !rotation )
    {
	rotation = new SoRotation;
	insertChild( childIndex( shape ), rotation );
    }

    rotation->rotation.setValue( newrot );
    
    float length = dir.radius;
    if ( length > 1 ) length = 1;
    else if ( length <= 0 ) length = 0;

    float orglength = arrow->lineLength.getValue();
    arrow->lineLength.setValue( orglength*length );
}


void Marker::setDip( float inldip, float crldip )
{
    if ( mIsUdf(inldip) || mIsUdf(crldip) )
	return;
    inldip_ = inldip;
    crldip_ = crldip;
    
    const float inldepth = (inldip*cDipFactor()) * zstretch_;
    const float crldepth = (crldip*cDipFactor()) * zstretch_;
    const float inlangle = atan( 2 * (SI().isClockWise() ? -inldepth : inldepth) );
    const float crlangle = atan( 2 * crldepth ); 

    SbRotation inlrot( SbVec3f(1,0,0), inlangle );
    SbRotation crlrot( SbVec3f(0,1,0), crlangle );
    SbRotation surveyrot( SbVec3f(0,0,1), getSurveyRotation() );
    SbRotation finalrot = inlrot * crlrot * surveyrot;
    rotation->rotation.setValue( finalrot );
}


void Marker::setDisplayTransformation( const mVisTrans* nt )
{
    const Coord3 pos = centerPos();
    if ( transformation ) transformation->unRef();
    transformation = nt;
    if ( transformation ) transformation->ref();
    setCenterPos( pos );
}


const mVisTrans* Marker::getDisplayTransformation() const
{ return transformation; }


int Marker::usePar( const IOPar& iopar )
{
    int res = VisualObjectImpl::usePar( iopar );
    if ( res != 1 ) return res;

    Coord3 pos;
    if ( !iopar.get( centerposstr, pos.x, pos.y, pos.z ) )
        return -1;
    setCenterPos( pos );

    return 1;
}


void Marker::fillPar( IOPar& iopar, TypeSet<int>& saveids ) const
{
    VisualObjectImpl::fillPar( iopar, saveids );

    Coord3 pos = centerPos();
    iopar.set( centerposstr, pos.x, pos.y, pos.z );
}


void Marker::setZStretch( float stretch )
{
    zstretch_ = stretch;
    if ( markerstyle.type_ == MarkerStyle3D::Plane )
	setDip( inldip_, crldip_ );
}

}; // namespace visBase

