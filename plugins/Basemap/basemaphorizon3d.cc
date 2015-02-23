/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Henrique Mageste
 Date:		December 2014
________________________________________________________________________

-*/

static const char* rcsID mUsedVar = "$Id$";

#include "basemaphorizon3d.h"
#include "odimage.h"

namespace Basemap
{

Horizon3DObject::Horizon3DObject()
    : BaseMapObject(0)
    , rgbimage_(0)
{
}


Horizon3DObject::~Horizon3DObject()
{
    delete rgbimage_;
}


void Horizon3DObject::setImage( int, OD::RGBImage* image )
{
    delete rgbimage_;
    rgbimage_ = image;
}


void Horizon3DObject::updateGeometry()
{
    changed.trigger();
}


int Horizon3DObject::nrShapes() const
{ return 1; }


const char* Horizon3DObject::getShapeName(int) const
{ return name().buf(); }


} // namespcae Basemap
