/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          Feb 2008
________________________________________________________________________

-*/
static const char* rcsID = "$Id: inituiseis.cc,v 1.3 2011-08-23 06:54:12 cvsbert Exp $";

#include "inituiseis.h"
#include "uiveldesc.h"
#include "uit2dvelconvselgroup.h"

void uiSeis::initStdClasses()
{
    mIfNotFirstTime( return );

    uiTime2Depth::initClass();
    uiDepth2Time::initClass();
    uiT2DVelConvSelGroup::initClass();
}
