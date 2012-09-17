/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          Oct 2010
________________________________________________________________________

-*/
static const char* rcsID = "$Id: initstrat.cc,v 1.3 2011/08/23 14:51:33 cvsbert Exp $";

#include "moddepmgr.h"
#include "stratsinglaygen.h"

mDefModInitFn(Strat)
{
    mIfNotFirstTime( return );

    Strat::SingleLayerGenerator::initClass();
}
