/*+
________________________________________________________________________

 CopyRight:     (C) de Groot-Bril Earth Sciences B.V.
 Author:        N. Hemstra
 Date:          July 2002
 RCS:           $Id: vismarker.cc,v 1.6 2003-01-20 09:36:07 kristofer Exp $
________________________________________________________________________

-*/

#include "vismarker.h"
#include "visshapescale.h"
#include "viscube.h"
#include "iopar.h"
#include "vistransform.h"

#include <Inventor/nodes/SoTranslation.h>
#include <Inventor/nodes/SoRotationXYZ.h>
#include <Inventor/nodes/SoGroup.h>
#include <Inventor/nodes/SoScale.h>
#include <Inventor/nodes/SoCube.h>
#include <Inventor/nodes/SoSphere.h>
#include <Inventor/nodes/SoCylinder.h>
#include <Inventor/nodes/SoCone.h>
#include <Inventor/nodes/SoText3.h>

mCreateFactoryEntry( visBase::Marker );

const char* visBase::Marker::centerposstr = "Center Pos";


visBase::Marker::Marker()
    : VisualObjectImpl(true)
    , group( new SoGroup )
    , position( new SoTranslation )
    , scale( new SoScale )
    , shapescale( visBase::ShapeScale::create() )
    , markertype( Cube )
    , transformation( 0 )
{
    addChild( position );
    addChild( group );
    group->ref();
    scale = new SoScale;
    group->addChild( scale );
    SoRotationXYZ* rot = new SoRotationXYZ;
    rot->angle = -M_PI/2;
    rot->axis = SoRotationXYZ::X;
    group->addChild( rot );
    group->addChild( shapescale->getData() );
    setType( markertype );

//  Creation only used for being able to read old session files with Cubes
    visBase::Cube* cube = visBase::Cube::create();
}


visBase::Marker::~Marker()
{
    group->unref();
    if ( transformation ) transformation->unRef();
}


void visBase::Marker::setCenterPos( const Coord3& pos_ )
{
    Coord3 pos( pos_ );

    if ( transformation ) pos = transformation->transform( pos );
    position->translation.setValue( pos.x, pos.y, pos.z );
}


Coord3 visBase::Marker::centerPos(bool displayspace) const
{
    Coord3 res;
    SbVec3f pos = position->translation.getValue();

    res.x = pos[0];
    res.y = pos[1];
    res.z = pos[2];

    if ( !displayspace && transformation )
	res = transformation->transformBack( res );

    return res;
}


void visBase::Marker::setType( Type type )
{
    switch ( type )
    {
    case Cube: {
	shapescale->setShape( new SoCube );
	} break;
    case Cone: {
	shapescale->setShape( new SoCone );
	} break;
    case Cylinder: {
	shapescale->setShape( new SoCylinder );
	} break;
    case Sphere: {
	shapescale->setShape( new SoSphere );
	} break;
    case Cross: {
	SoText3* xshape = new SoText3;
	xshape->string.setValue( "x" );
	shapescale->setShape( xshape );
	} break;
    case Star: {
	SoText3* oshape = new SoText3;
	oshape->string.setValue( "*" );
	shapescale->setShape( oshape );
	} break;
    }

    markertype = type;
}


void visBase::Marker::setSize( const float r )
{
    shapescale->setSize( r );
}


float visBase::Marker::getSize() const
{
    return shapescale->getSize();
}


void visBase::Marker::setScale( const Coord3& pos )
{
    scale->scaleFactor.setValue( pos.x, pos.y, pos.z );
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

