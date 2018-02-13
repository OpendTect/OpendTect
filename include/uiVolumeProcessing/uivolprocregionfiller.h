#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Y.C. Liu
 Date:		November 2007
 RCS:		$Id: uivolprocregionfiller.h 34917 2014-05-28 08:13:46Z bart.degroot@dgbes.com $
________________________________________________________________________

-*/

#include "uivolumeprocessingmod.h"
#include "uivolprocstepdlg.h"
#include "volprocregionfiller.h"

class uiBodyRegionGrp;
class uiGenInput;
class uiGroup;
class uiHorizonAuxDataSel;
class uiPushButton;

namespace VolProc
{

mExpClass(uiVolumeProcessing) uiRegionFiller: public uiStepDialog
{ mODTextTranslationClass(uiRegionFiller)
public:
    mDefaultFactoryInstantiationBase(
		RegionFiller::sFactoryKeyword(),
		RegionFiller::sFactoryDisplayName())
        mDefaultFactoryInitClassImpl( uiStepDialog, createInstance )

    static uiString		sGradientLabel();

protected:
				uiRegionFiller(uiParent*,RegionFiller*,
						bool is2d);
				~uiRegionFiller();

    static uiStepDialog*	createInstance(uiParent*,Step*,bool is2d);
    uiGroup*			createVelGrp();
    void			statsPushCB(CallBacker*);
    void			startvalCB(CallBacker*);
    void			gradientCB(CallBacker*);
    bool			acceptOK();

    RegionFiller*		regionfiller_;
    uiBodyRegionGrp*		regiongrp_;
    uiGenInput*			constvalfld_;
    uiPushButton*		statsbut_;

    uiGroup*			velocitygrp_;
    uiGenInput*			startvalselfld_;
    uiGenInput*			startvalfld_;
    uiHorizonAuxDataSel*	starthorfld_;
    uiGenInput*			gradvalselfld_;
    uiGenInput*			gradvalfld_;
    uiHorizonAuxDataSel*	gradhorfld_;

};

} // namespace VolProc
