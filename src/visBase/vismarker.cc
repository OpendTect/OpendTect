/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        N. Hemstra
 Date:          July 2002
 RCS:           $Id: vismarker.cc,v 1.9 2003-11-28 15:40:55 nanne Exp $
________________________________________________________________________

-*/

#include "vismarker.h"
#include "viscube.h"
#include "iopar.h"
#include "vistransform.h"

#include "SoMarkerScale.h"
#include "SoArrow.h"
#include <Inventor/nodes/SoTranslation.h>
#include <Inventor/nodes/SoScale.h>
#include <Inventor/nodes/SoCube.h>
#include <Inventor/nodes/SoSphere.h>
#include <Inventor/nodes/SoCylinder.h>
#include <Inventor/nodes/SoCone.h>
#include <Inventor/nodes/SoText3.h>

mCreateFactoryEntry( visBase::Marker );

const char* visBase::Marker::centerposstr = "Center Pos";

DefineEnumNames(visBase::Marker,Type,0,"Marker type")
{
    "Cube",
    "Cone",
    "Cylinder",
    "Sphere",
    "Arrow",
    "Cross",
    0
};


visBase::Marker::Marker()
    : VisualObjectImpl(true)
    , markertype( Cube )
    , transformation( 0 )
    , markerscale( new SoMarkerScale )
    , shape( 0 )
{
    addChild( markerscale );
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
	break;
    case Cylinder:
	shape = new SoCylinder;
	break;
    case Sphere:
	shape = new SoSphere;
	break;
    case Arrow:
	shape = new SoArrow;
	break;
    case Cross:
	{
	    SoText3* xshape = new SoText3;
	    xshape->string.setValue( "x" );
	    shape = xshape;
	}
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

