/*+
 * COPYRIGHT: (C) dGB Beheer B.V.
 * AUTHOR   : K. Tingdahl
 * DATE     : Oct 1999
-*/

static const char* rcsID = "$Id: viscube.cc,v 1.16 2004-11-16 09:28:33 kristofer Exp $";

#include "viscube.h"

#include "vistransform.h"
#include "iopar.h"

#include "Inventor/nodes/SoCube.h"
#include "Inventor/nodes/SoTranslation.h"

mCreateFactoryEntry( visBase::Cube );

const char* visBase::Cube::centerposstr = "Center Pos";
const char* visBase::Cube::widthstr = "Width";


visBase::Cube::Cube()
    : Shape( new SoCube )
    , position( new SoTranslation )
    , transformation( 0 )
{
    insertNode( position );
}


visBase::Cube::~Cube()
{ setDisplayTransformation(0); }


void visBase::Cube::setDisplayTransformation( Transformation* nt )
{
    if ( transformation==nt ) return;

    const Coord3 oldpos = centerPos();
    const Coord3 oldwidth = width();

    if ( transformation )
	transformation->unRef();

    transformation = nt;

    if ( transformation )
	transformation->ref();

    setCenterPos( oldpos );
    setWidth( oldwidth );
}


void visBase::Cube::setCenterPos( const Coord3& pos_ )
{
    const Coord3 pos = transformation ? transformation->transform(pos_) : pos_;
    position->translation.setValue( pos.x, pos.y, pos.z );
}


Coord3 visBase::Cube::centerPos() const
{
    Coord3 res;
    SbVec3f pos = position->translation.getValue();

    res.x = pos[0];
    res.y = pos[1];
    res.z = pos[2];

    return transformation ? transformation->transformBack(res) : res;
}


void visBase::Cube::setWidth( const Coord3& n )
{
    const Coord3 transscale = transformation ? transformation->getScale() :
			      Coord3(1,1,1);

    SoCube* cube = reinterpret_cast<SoCube*>( shape );
    cube->width.setValue(n.x*transscale.x);
    cube->height.setValue(n.y*transscale.y);
    cube->depth.setValue(n.z*transscale.z);
}


Coord3 visBase::Cube::width() const
{
    Coord3 res;
    
    SoCube* cube = reinterpret_cast<SoCube*>( shape );
    res.x = cube->width.getValue();
    res.y = cube->height.getValue();
    res.z = cube->depth.getValue();

    if ( transformation )
    {
	 const Coord3 transscale = transformation->getScale();
	 res.x /= transscale.x;
	 res.y /= transscale.y;
	 res.z /= transscale.z;
    }

    return res;
}


int visBase::Cube::usePar( const IOPar& iopar )
{
    int res = Shape::usePar( iopar );
    if ( res != 1 ) return res;

    Coord3 pos;
    if ( !iopar.get( centerposstr, pos.x, pos.y, pos.z ) )
	return -1;

    setCenterPos( pos );

    if ( !iopar.get( widthstr, pos.x, pos.y, pos.z ) )
	return -1;

    setWidth( pos );

    return 1;
}


void visBase::Cube::fillPar( IOPar& iopar, TypeSet<int>& saveids ) const
{
    Shape::fillPar( iopar, saveids );

    Coord3 pos = centerPos();
    iopar.set( centerposstr, pos.x, pos.y, pos.z );

    pos = width();
    iopar.set( widthstr, pos.x, pos.y, pos.z );
}

