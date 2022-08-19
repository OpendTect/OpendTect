/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
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
