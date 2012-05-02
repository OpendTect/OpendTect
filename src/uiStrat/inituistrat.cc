/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          Oct 2010
________________________________________________________________________

-*/
static const char* mUnusedVar rcsID = "$Id: inituistrat.cc,v 1.5 2012-05-02 11:53:54 cvskris Exp $";

#include "moddepmgr.h"
#include "uistratbasiclayseqgendesc.h"

mDefModInitFn(uiStrat)
{
    mIfNotFirstTime( return );

    uiBasicLayerSequenceGenDesc::initClass();
}
