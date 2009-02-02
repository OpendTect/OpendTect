/*+
 * COPYRIGHT: (C) dGB Beheer B.V.
 * AUTHOR   : K. Tingdahl
 * DATE     : July 2008
-*/

static const char* rcsID = "$Id: inituivelocity.cc,v 1.2 2009-02-02 07:03:23 cvsranojay Exp $";

#include "inituivelocity.h"
#include "uivelocityfunctionvolume.h"

namespace uiVelocity
{

void initStdClasses()
{
    Vel::uiVolumeFunction::initClass();
}

}; //namespace
