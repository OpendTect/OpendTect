/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        K. Tingdahl
 Date:          Jul 2003
________________________________________________________________________

-*/
static const char* rcsID = "$Id: viscamerainfo.cc,v 1.6 2008-11-25 15:35:27 cvsbert Exp $";

#include "viscamerainfo.h"
#include "SoCameraInfo.h"

mCreateFactoryEntry( visBase::CameraInfo );

namespace visBase
{

CameraInfo::CameraInfo()
    : camerainfo( new SoCameraInfo )
{
    camerainfo->ref();
}


CameraInfo::~CameraInfo()
{
    camerainfo->unref();
}


void CameraInfo::setInteractive(bool yn)
{
    if ( isInteractive()==yn ) return;
    camerainfo->cameraInfo = camerainfo->cameraInfo.getValue() +
			     (yn ? SoCameraInfo::INTERACTIVE
			        : -SoCameraInfo::INTERACTIVE);
}


bool CameraInfo::isInteractive() const
{
    return (camerainfo->cameraInfo.getValue() & SoCameraInfo::INTERACTIVE);
}


void CameraInfo::setMoving(bool yn)
{
    if ( isMoving()==yn ) return;
    camerainfo->cameraInfo = camerainfo->cameraInfo.getValue() +
			 (yn ? SoCameraInfo::MOVING : -SoCameraInfo::MOVING);
}


bool CameraInfo::isMoving() const
{
    return (camerainfo->cameraInfo.getValue() & SoCameraInfo::MOVING);
}


SoNode* CameraInfo::getInventorNode()
{
    return camerainfo;
}

}; // namespace visBase
