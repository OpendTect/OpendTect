/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          Oct 2010
________________________________________________________________________

-*/
static const char* rcsID = "$Id: initstrat.cc,v 1.2 2011-08-23 06:54:11 cvsbert Exp $";

#include "initstrat.h"
#include "stratsinglaygen.h"

void Strat::initStdClasses()
{
    mIfNotFirstTime( return );

    SingleLayerGenerator::initClass();
}
