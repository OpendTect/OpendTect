/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Satyaki
 Date:          December 2007
________________________________________________________________________

-*/
static const char* rcsID = "$Id: inituiattributes.cc,v 1.4 2009-07-22 16:01:37 cvsbert Exp $";

#include "inituiattributes.h"

#include "uicoherencyattrib.h"
#include "uiconvolveattrib.h"
#include "uidipfilterattrib.h"
#include "uienergyattrib.h"
#include "uieventattrib.h"
#include "uifingerprintattrib.h"
#include "uifrequencyattrib.h"
#include "uifreqfilterattrib.h"
#include "uiinstantattrib.h"
#include "uimathattrib.h"
#include "uiprestackattrib.h"
#include "uipositionattrib.h"
#include "uireferenceattrib.h"
#include "uiscalingattrib.h"
#include "uishiftattrib.h"
#include "uisimilarityattrib.h"
#include "uispecdecompattrib.h"
#include "uivolstatsattrib.h"


void uiAttributes::initStdClasses()
{
    uiCoherencyAttrib::initClass();
    uiConvolveAttrib::initClass();
    uiDipFilterAttrib::initClass();
    uiEnergyAttrib::initClass();
    uiEventAttrib::initClass();
    uiFingerPrintAttrib::initClass();
    uiFrequencyAttrib::initClass();
    uiFreqFilterAttrib::initClass();
    uiInstantaneousAttrib::initClass();
    uiMathAttrib::initClass();
    uiPreStackAttrib::initClass();
    uiPositionAttrib::initClass();
    uiReferenceAttrib::initClass();
    uiScalingAttrib::initClass();
    uiShiftAttrib::initClass();
    uiSimilarityAttrib::initClass();
    uiSpecDecompAttrib::initClass();
    uiVolumeStatisticsAttrib::initClass();
}

