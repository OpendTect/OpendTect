/*+
 * COPYRIGHT: (C) dGB Beheer B.V.
 * AUTHOR   : K. Tingdahl
 * DATE     : July 2008
-*/

static const char* rcsID = "$Id: inituivelocity.cc,v 1.3 2009-03-18 18:45:26 cvskris Exp $";

#include "inituivelocity.h"
#include "uivelocityfunctionvolume.h"
#include "uivelocityfunctionstored.h"

namespace uiVelocity
{

void initStdClasses()
{
    Vel::uiVolumeFunction::initClass();
    Vel::uiStoredFunction::initClass();
}

}; //namespace
