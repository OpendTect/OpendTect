/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        N. Hemstra
 Date:          July 2002
 RCS:           $Id: vismarker.cc,v 1.10 2003-12-11 16:28:10 nanne Exp $
________________________________________________________________________

-*/

#include "vismarker.h"
#include "viscube.h"
#include "iopar.h"
#include "vistransform.h"

#include "SoMarkerScale.h"
#include "SoArrow.h"
#include <Inventor/nodes/SoCube.h>
#include <Inventor/nodes/SoSphere.h>
#include <Inventor/nodes/SoCylinder.h>
#include <Inventor/nodes/SoCone.h>
#include <Inventor/nodes/SoRotation.h>

#include <math.h>

mCreateFactoryEntry( visBase::Marker );

const char* visBase::Marker::centerposstr = "Center Pos";

DefineEnumNames(visBase::Marker,Type,0,"Marker type")
{
    "Cube",
    "Cone",
    "Cylinder",
    "Sphere",
    "Arrow",
    0
};


visBase::Marker::Marker()
    : VisualObjectImpl(true)
    , markertype(Cube)
    , transformation(0)
    , rotation(new SoRotation)
    , markerscale(new SoMarkerScale)
    , shape(0)
    , direction(0,0,0)
{
    addChild( markerscale );
    addChild( rotation );
    setType( markertype );

//  Creation only used for being able to read old session files with Cubes
    visBase::Cube* cube = visBase::Cube::create();
}


visBase::Marker::~Marker()
{
    if ( transformation ) transformation->unRef();
}


void visBase::Marker::setCenterPos( const Coord3& pos_ )
{
    Coord3 pos( pos_ );

    if ( transformation ) pos = transformation->transform( pos );
    markerscale->translation.setValue( pos.x, pos.y, pos.z );
}


Coord3 visBase::Marker::centerPos(bool displayspace) const
{
    Coord3 res;
    SbVec3f pos = markerscale->translation.getValue();

    res.x = pos[0];
    res.y = pos[1];
    res.z = pos[2];

    if ( !displayspace && transformation )
	res = transformation->transformBack( res );

    return res;
}


void visBase::Marker::setType( Type type )
{
    if ( shape ) removeChild(shape);
    switch ( type )
    {
    case Cube: {
	shape = new SoCube;
	} break;
    case Cone:
	shape = new SoCone;
	setRotation( Coord3(1,0,0), M_PI/2 );
	break;
    case Cylinder:
	shape = new SoCylinder;
	setRotation( Coord3(1,0,0), M_PI/2 );
	break;
    case Sphere:
	shape = new SoSphere;
	break;
    case Arrow:
	shape = new SoArrow;
	break;
    }

    addChild( shape );

    markertype = type;
}


void visBase::Marker::setSize( const float r )
{
    markerscale->screenSize.setValue(r);
}


float visBase::Marker::getSize() const
{
    return markerscale->screenSize.getValue();
}


void visBase::Marker::setScale( const Coord3& pos )
{
    markerscale->scaleFactor.setValue( pos.x, pos.y, pos.z );
}


void visBase::Marker::setRotation( const Coord3& vec, float angle )
{
    rotation->rotation.setValue( SbVec3f(vec[0],vec[1],vec[2]), angle );
}


void visBase::Marker::setDirection( const Coord3& dir )
{
    mDynamicCastGet(SoArrow*,arrow,shape)
    if ( !arrow ) return;

    direction = dir;
    SbVec3f orgdir( 1, 0, 0 );
    SbVec3f newdir( dir[0], dir[1], dir[2] );
    float length = newdir.length();

    newdir.normalize();
    SbVec3f rot = orgdir.cross( newdir );
    float angle = acos( orgdir.dot(newdir) );
    if ( rot.sqrLength() > 0 )
	rotation->rotation.setValue( rot, angle );

    setLength( length );
}


void visBase::Marker::setLength( float length )
{
    mDynamicCastGet(SoArrow*,arrow,shape)
    if ( arrow )
	arrow->lineLength.setValue( length );
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

