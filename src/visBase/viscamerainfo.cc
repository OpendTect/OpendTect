/*
___________________________________________________________________

 * COPYRIGHT: (C) de Groot-Bril Earth Sciences B.V.
 * AUTHOR   : K. Tingdahl
 * DATE     : Jul 2003
___________________________________________________________________

-*/

static const char* rcsID = "$Id: viscamerainfo.cc,v 1.1 2003-09-02 12:36:03 kristofer Exp $";

#include "viscamerainfo.h"
#include "SoCameraInfo.h"

mCreateFactoryEntry( visBase::CameraInfo );

visBase::CameraInfo::CameraInfo()
    : camerainfo( new SoCameraInfo )
{
    camerainfo->ref();
}


visBase::CameraInfo::~CameraInfo()
{
    camerainfo->unref();
}


void visBase::CameraInfo::setInteractive(bool yn)
{
    if ( isInteractive()==yn ) return;
    camerainfo->cameraInfo = camerainfo->cameraInfo.getValue() +
			     (yn ? SoCameraInfo::INTERACTIVE
			        : -SoCameraInfo::INTERACTIVE);
}


bool visBase::CameraInfo::isInteractive() const
{
    return (camerainfo->cameraInfo.getValue() & SoCameraInfo::INTERACTIVE);
}


void visBase::CameraInfo::setMoving(bool yn)
{
    if ( isMoving()==yn ) return;
    camerainfo->cameraInfo = camerainfo->cameraInfo.getValue() +
			 (yn ? SoCameraInfo::MOVING : -SoCameraInfo::MOVING);
}


bool visBase::CameraInfo::isMoving() const
{
    return (camerainfo->cameraInfo.getValue() & SoCameraInfo::MOVING);
}


SoNode* visBase::CameraInfo::getData()
{
    return camerainfo;
}
