/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        N. Hemstra
 Date:          July 2002
 RCS:           $Id: vismarker.cc,v 1.15 2004-05-24 13:56:59 kristofer Exp $
________________________________________________________________________

-*/

#include "vismarker.h"
#include "iopar.h"
#include "vistransform.h"


#include "SoShapeScale.h"
#include "SoArrow.h"
#include <Inventor/nodes/SoCube.h>
#include <Inventor/nodes/SoSphere.h>
#include <Inventor/nodes/SoCylinder.h>
#include <Inventor/nodes/SoTranslation.h>
#include <Inventor/nodes/SoCone.h>
#include <Inventor/nodes/SoRotation.h>

#include <math.h>

mCreateFactoryEntry( visBase::Marker );

const char* visBase::Marker::centerposstr = "Center Pos";


visBase::Marker::Marker()
    : VisualObjectImpl(true)
    , transformation(0)
    , translation(new SoTranslation)
    , rotation(new SoRotation)
    , markerscale(new SoShapeScale)
    , shape(0)
    , direction(0,0,0)
{
    addChild( translation );
    addChild( markerscale );
    addChild( rotation );
    setType( MarkerStyle3D::Cube );

    markerscale->restoreProportions = true;
    markerscale->dorotate = true;
}


visBase::Marker::~Marker()
{
    if ( transformation ) transformation->unRef();
}


void visBase::Marker::setCenterPos( const Coord3& pos_ )
{
    Coord3 pos( pos_ );

    if ( transformation ) pos = transformation->transform( pos );
    translation->translation.setValue( pos.x, pos.y, pos.z );
}


Coord3 visBase::Marker::centerPos(bool displayspace) const
{
    Coord3 res;
    SbVec3f pos = translation->translation.getValue();

    res.x = pos[0];
    res.y = pos[1];
    res.z = pos[2];

    if ( !displayspace && transformation )
	res = transformation->transformBack( res );

    return res;
}


void visBase::Marker::setMarkerStyle( const MarkerStyle3D& ms )
{
    setType( ms.type );
    setScreenSize( (float)ms.size );
}


MarkerStyle3D::Type visBase::Marker::getType() const
{
    return markerstyle.type;
}


void visBase::Marker::setType( MarkerStyle3D::Type type )
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
    }

    markerstyle.type = type;
}


void visBase::Marker::setMarkerShape(SoNode* newshape)
{
    if ( shape ) removeChild(shape);
    shape = newshape;
    addChild( shape );
}


void visBase::Marker::setScreenSize( const float sz )
{
    markerscale->screenSize.setValue( sz );
    markerstyle.size = (int)sz;
}


float visBase::Marker::getScreenSize() const
{
    return markerscale->screenSize.getValue();
}


void visBase::Marker::doFaceCamera(bool yn)
{ markerscale->dorotate = yn; }


bool visBase::Marker::facesCamera() const
{ return markerscale->dorotate.getValue(); }


void visBase::Marker::doRestoreProportions(bool yn)
{ markerscale->restoreProportions = yn; }


bool visBase::Marker::restoresProportions() const
{ return markerscale->restoreProportions.getValue(); }


void visBase::Marker::setRotation( const Coord3& vec, float angle )
{
    rotation->rotation.setValue( SbVec3f(vec[0],vec[1],vec[2]), angle );
}


void visBase::Marker::setArrowDir( const ::Sphere& dir )
{
    mDynamicCastGet(SoArrow*,arrow,shape)
    if ( !arrow ) return;

    Coord3 newcrd = spherical2Cartesian( dir, false );
    newcrd /= dir.radius;

    SbVec3f orgvec(1,0,0);
    SbRotation newrot( orgvec, SbVec3f(newcrd.x,newcrd.y,-newcrd.z) );
    rotation->rotation.setValue( newrot );
    
    float length = dir.radius;
    if ( length > 1 ) length = 1;
    else if ( length <= 0 ) length = 0;

    float orglength = arrow->lineLength.getValue();
    arrow->lineLength.setValue( orglength*length );
}


void visBase::Marker::setTransformation( visBase::Transformation* nt )
{
    const Coord3 pos = centerPos();
    if ( transformation ) transformation->unRef();
    transformation = nt;
    if ( transformation ) transformation->ref();
    setCenterPos( pos );
}


visBase::Transformation* visBase::Marker::getTransformation()
{ return transformation; }


int visBase::Marker::usePar( const IOPar& iopar )
{
    int res = VisualObjectImpl::usePar( iopar );
    if ( res != 1 ) return res;

    Coord3 pos;
    if ( !iopar.get( centerposstr, pos.x, pos.y, pos.z ) )
        return -1;
    setCenterPos( pos );

    return 1;
}


void visBase::Marker::fillPar( IOPar& iopar, TypeSet<int>& saveids ) const
{
    VisualObjectImpl::fillPar( iopar, saveids );

    Coord3 pos = centerPos();
    iopar.set( centerposstr, pos.x, pos.y, pos.z );
}

