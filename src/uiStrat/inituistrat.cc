/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          Oct 2010
________________________________________________________________________

-*/
static const char* rcsID = "$Id: inituistrat.cc,v 1.3 2011-08-23 06:54:12 cvsbert Exp $";

#include "inituistrat.h"
#include "uistratbasiclayseqgendesc.h"

void uiStrat::initStdClasses()
{
    mIfNotFirstTime( return );

    uiBasicLayerSequenceGenDesc::initClass();
}
