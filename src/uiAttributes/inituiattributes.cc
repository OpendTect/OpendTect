/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Satyaki
 Date:          December 2007
________________________________________________________________________

-*/


#include "moddepmgr.h"
#include "uiconvolveattrib.h"
#include "uideltaresampleattrib.h"
#include "uidipfilterattrib.h"
#include "uienergyattrib.h"
#include "uieventattrib.h"
#include "uifingerprintattrib.h"
#include "uifrequencyattrib.h"
#include "uifreqfilterattrib.h"
#include "uigapdeconattrib.h"
#include "uiinstantattrib.h"
#include "uimatchdeltaattrib.h"
#include "uimathattrib.h"
#include "uiprestackattrib.h"
#include "uipositionattrib.h"
#include "uireferenceattrib.h"
#include "uireliefattrib.h"
#include "uisamplevalueattrib.h"
#include "uiscalingattrib.h"
#include "uisemblanceattrib.h"
#include "uishiftattrib.h"
#include "uisimilarityattrib.h"
#include "uispecdecompattrib.h"
#include "uitextureattrib.h"
#include "uivolstatsattrib.h"


mDefModInitFn(uiAttributes)
{
    mIfNotFirstTime( return );

    uiConvolveAttrib::initClass();
    uiDeltaResampleAttrib::initClass();
    uiDipFilterAttrib::initClass();
    uiEnergyAttrib::initClass();
    uiEventAttrib::initClass();
    uiFingerPrintAttrib::initClass();
    uiFrequencyAttrib::initClass();
    uiFreqFilterAttrib::initClass();
    uiGapDeconAttrib::initClass();
    uiInstantaneousAttrib::initClass();
    uiMatchDeltaAttrib::initClass();
    uiMathAttrib::initClass();
    uiPreStackAttrib::initClass();
    uiPositionAttrib::initClass();
    uiReferenceAttrib::initClass();
    uiReliefAttrib::initClass();
    uiSampleValueAttrib::initClass();
    uiScalingAttrib::initClass();
    uiSemblanceAttrib::initClass();
    uiShiftAttrib::initClass();
    uiSimilarityAttrib::initClass();
    uiSpecDecompAttrib::initClass();
    uiTextureAttrib::initClass();
    uiVolumeStatisticsAttrib::initClass();
}
