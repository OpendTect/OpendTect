/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          Feb 2008
________________________________________________________________________

-*/
static const char* rcsID = "$Id: inituiseis.cc,v 1.1 2009-07-26 04:14:18 cvskris Exp $";

#include "inituiseis.h"
#include "uiveldesc.h"

void uiSeis::initStdClasses()
{
    uiTime2Depth::initClass();
    uiDepth2Time::initClass();
}
