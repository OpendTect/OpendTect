/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Kristofer Tingdahl
 Date:          December 2007
________________________________________________________________________

-*/

#include "moddepmgr.h"
#include "prestackagc.h"
#include "prestackmute.h"
#include "prestacklateralstack.h"
#include "prestackanglemute.h"
#include "prestackeventtransl.h"
#include "prestackmutedeftransl.h"
#include "prestackprocessortransl.h"
#include "prestacktrimstatics.h"

mDefModInitFn(PreStackProcessing)
{
    mIfNotFirstTime( return );
    
    PSEventTranslatorGroup::initClass();
    MuteDefTranslatorGroup::initClass();
    PreStackProcTranslatorGroup::initClass();
    
    dgbPSEventTranslator::initClass();
    dgbMuteDefTranslator::initClass();
    dgbPreStackProcTranslator::initClass();

    PreStack::Mute::initClass();
    PreStack::AGC::initClass();
    PreStack::LateralStack::initClass();
    PreStack::AngleMute::initClass();
#ifdef __debug__
    PreStack::TrimStatics::initClass();
#endif
}
