/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          Oct 2010
________________________________________________________________________

-*/
static const char* rcsID = "$Id: initstrat.cc,v 1.1 2010-10-19 08:50:58 cvsbert Exp $";

#include "initstrat.h"
#include "stratsinglaygen.h"

void Strat::initStdClasses()
{
    SingleLayerGenerator::initClass();
}
