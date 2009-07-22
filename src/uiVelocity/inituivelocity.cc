/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : K. Tingdahl
 * DATE     : July 2008
-*/

static const char* rcsID = "$Id: inituivelocity.cc,v 1.4 2009-07-22 16:01:43 cvsbert Exp $";

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
