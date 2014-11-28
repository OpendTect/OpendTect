/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Henrique Mageste
 Date:		November 2014
________________________________________________________________________

-*/

static const char* rcsID mUsedVar = "$Id$";

#include "basemapoutline.h"

#include "ioman.h"
#include "ioobj.h"
#include "ptrman.h"
//#include "seis2ddata.h"
//#include "seisreaderset.h"

namespace Basemap
{

OutlineObject::OutlineObject( const MultiID& mid )
    : BaseMapObject(0)
    , seismid_(mid)
{
    setMultiID( mid );
}


OutlineObject::~OutlineObject()
{
}


void OutlineObject::setMultiID( const MultiID& mid )
{
    seismid_ = mid;
    //wd_.setEmpty();
    PtrMan<IOObj> ioobj = IOM().get( seismid_ );
    if ( !ioobj ) return;

    setName( ioobj->name() );
    //Well::Reader rdr( *ioobj, wd_ );
    //rdr.getInfo();

    updateGeometry();
}


void OutlineObject::updateGeometry()
{
    changed.trigger();
}


int OutlineObject::nrShapes() const
{ return 1; }


const char* OutlineObject::getShapeName(int) const
{ return name().buf(); }


void OutlineObject::getPoints( int shapeidx, TypeSet<Coord>& pts ) const
{

}

} // namepace Basemap
