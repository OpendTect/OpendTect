/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          Oct 2010
________________________________________________________________________

-*/
static const char* rcsID = "$Id: inituistrat.cc,v 1.1 2010-10-19 08:52:03 cvsbert Exp $";

#include "inituistrat.h"
#include "uistratsinglayseqgendesc.h"

void uiStrat::initStdClasses()
{
    uiSingleLayerSequenceGenDesc::initClass();
}
