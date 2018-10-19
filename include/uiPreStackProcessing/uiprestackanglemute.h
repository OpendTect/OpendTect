#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Y. Liu
 Date:		January 2011
________________________________________________________________________


-*/

#include "uiprestackprocessingmod.h"
#include "uidialog.h"
#include "uigroup.h"
#include "prestackanglemute.h"
#include "uistring.h"

class uiGenInput;
class uiLabel;
class uiPushButton;
class uiRayTracerSel;
class uiVelSel;

namespace PreStack
{

class Processor;
class uiAngleCompAdvParsDlg;

mExpClass(uiPreStackProcessing) uiAngleCompGrp : public uiGroup
{ mODTextTranslationClass(uiAngleCompGrp);
public:
				uiAngleCompGrp(uiParent*,
					       PreStack::AngleCompParams&,
					       bool dooffset=false,
					       bool isformute=true);

    void			updateFromParams();

    bool			acceptOK();

protected:
    PreStack::AngleCompParams&	params_;

    void			advPushButCB(CallBacker*);

    uiVelSel*			velfuncsel_;
    uiGenInput*			anglefld_;
    uiLabel*			anglelbl_;
    uiPushButton*		advpushbut_;
    uiAngleCompAdvParsDlg*	advpardlg_;

    bool			isformute_;
    bool			dooffset_;
};


mExpClass(uiPreStackProcessing) uiAngleCompAdvParsDlg : public uiDialog
{ mODTextTranslationClass(uiAngleCompAdvParsDlg);
public:
			uiAngleCompAdvParsDlg(uiParent*,
					      PreStack::AngleCompParams&,
					      bool dooffset=false,
					      bool isformute=true);

    void		updateFromParams();

protected :

    bool		acceptOK();
    void		createAngleCompFields();
    bool		isSmoothTypeMovingAverage();
    bool		isSmoothTypeFFTFilter();
    void		smoothTypeSel(CallBacker*);
    void		smoothWindowSel(CallBacker*);
    void		getRayTracerPars();
    void		setRayTracerPars();
    void		finaliseCB(CallBacker*);

    uiRayTracerSel*	raytracerfld_;
    uiGenInput*		smoothtypefld_;
    uiGenInput*		smoothwindowfld_;
    uiGenInput*		smoothwinparamfld_;
    uiGenInput*		smoothwinlengthfld_;
    uiGenInput*		freqf3fld_;
    uiGenInput*		freqf4fld_;

    bool		isformute_;
    PreStack::AngleCompParams&	params_;

    friend class uiAngleCompGrp;
};


mExpClass(uiPreStackProcessing) uiAngleMute : public uiDialog
{ mODTextTranslationClass(uiAngleMute);
public:

    static void		initClass();
			uiAngleMute(uiParent*,AngleMute*);

protected:

    bool		acceptOK();
    static uiDialog*	create(uiParent*,Processor*);

    void		createPushedCB(CallBacker*);
    void		editPushedCB(CallBacker*);
    void		selDoneCB(CallBacker*);

    AngleMute*		processor_;

    uiAngleCompGrp*	anglecompgrp_;
    uiGenInput*		taperlenfld_;
    uiGenInput*		topfld_;
};


}; //namespace
