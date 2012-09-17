/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        K. Tingdahl
 Date:          Feb 2002
________________________________________________________________________

-*/
static const char* rcsID = "$Id: viscube.cc,v 1.22 2011/12/16 15:57:21 cvskris Exp $";

#include "viscube.h"
#include "vistransform.h"
#include "iopar.h"

#include <Inventor/nodes/SoCube.h>
#include <Inventor/nodes/SoTranslation.h>

mCreateFactoryEntry( visBase::Cube );

namespace visBase
{

const char* Cube::centerposstr = "Center Pos";
const char* Cube::widthstr = "Width";


Cube::Cube()
    : Shape( new SoCube )
    , position( new SoTranslation )
    , transformation( 0 )
{
    insertNode( position );
}


Cube::~Cube()
{ setDisplayTransformation(0); }


void Cube::setDisplayTransformation( const mVisTrans* nt )
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


void Cube::setCenterPos( const Coord3& pos_ )
{
    const Coord3 pos = transformation ? transformation->transform(pos_) : pos_;
    position->translation.setValue( pos.x, pos.y, pos.z );
}


Coord3 Cube::centerPos() const
{
    Coord3 res;
    SbVec3f pos = position->translation.getValue();

    res.x = pos[0];
    res.y = pos[1];
    res.z = pos[2];

    return transformation ? transformation->transformBack(res) : res;
}


void Cube::setWidth( const Coord3& n )
{
    const Coord3 transscale = transformation ? transformation->getScale() :
			      Coord3(1,1,1);

    SoCube* cube = reinterpret_cast<SoCube*>( shape_ );
    cube->width.setValue(n.x*transscale.x);
    cube->height.setValue(n.y*transscale.y);
    cube->depth.setValue(n.z*transscale.z);
}


Coord3 Cube::width() const
{
    Coord3 res;
    
    SoCube* cube = reinterpret_cast<SoCube*>( shape_ );
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


int Cube::usePar( const IOPar& iopar )
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


void Cube::fillPar( IOPar& iopar, TypeSet<int>& saveids ) const
{
    Shape::fillPar( iopar, saveids );

    Coord3 pos = centerPos();
    iopar.set( centerposstr, pos.x, pos.y, pos.z );

    pos = width();
    iopar.set( widthstr, pos.x, pos.y, pos.z );
}


}; //namespace
