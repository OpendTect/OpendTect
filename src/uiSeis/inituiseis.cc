/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          Feb 2008
________________________________________________________________________

-*/
static const char* rcsID = "$Id: inituiseis.cc,v 1.2 2010-07-08 05:54:22 cvsnageswara Exp $";

#include "inituiseis.h"
#include "uiveldesc.h"
#include "uit2dvelconvselgroup.h"

void uiSeis::initStdClasses()
{
    uiTime2Depth::initClass();
    uiDepth2Time::initClass();
    uiT2DVelConvSelGroup::initClass();
}
