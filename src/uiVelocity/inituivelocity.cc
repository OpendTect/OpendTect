/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : K. Tingdahl
 * DATE     : July 2008
-*/

static const char* rcsID = "$Id: inituivelocity.cc,v 1.5 2011-08-23 06:54:12 cvsbert Exp $";

#include "inituivelocity.h"
#include "uivelocityfunctionvolume.h"
#include "uivelocityfunctionstored.h"

namespace uiVelocity
{

void initStdClasses()
{
    mIfNotFirstTime( return );

    Vel::uiVolumeFunction::initClass();
    Vel::uiStoredFunction::initClass();
}

}; //namespace
