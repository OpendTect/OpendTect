/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          Oct 2010
________________________________________________________________________

-*/
static const char* rcsID mUnusedVar = "$Id: inituistrat.cc,v 1.6 2012-05-02 15:12:18 cvskris Exp $";

#include "moddepmgr.h"
#include "uistratbasiclayseqgendesc.h"

mDefModInitFn(uiStrat)
{
    mIfNotFirstTime( return );

    uiBasicLayerSequenceGenDesc::initClass();
}
