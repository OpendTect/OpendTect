#ifndef uiprestackanglemute_h
#define uiprestackanglemute_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Y. Liu
 Date:		January 2011
 RCS:		$Id$
________________________________________________________________________


-*/

#include "uiprestackprocessingmod.h"
#include "uidialog.h"
#include "uigroup.h"
#include "prestackanglemute.h"

class CtxtIOObj;
class uiCheckBox;
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
{
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

    uiGenInput*			velsource_;
    uiVelSel*			velfuncsel_;
    uiGenInput*			anglefld_;
    uiLabel*			anglelbl_;
    uiPushButton*		advpushbut_;
    uiAngleCompAdvParsDlg*	advpardlg_;

    bool			isformute_;
};


mExpClass(uiPreStackProcessing) uiAngleCompAdvParsDlg : public uiDialog
{
public:
			uiAngleCompAdvParsDlg(uiParent*,
					      PreStack::AngleCompParams&,
					      bool dooffset=false,
					      bool isformute=true);

    void		updateFromParams();

protected :

    bool		acceptOK(CallBacker*);
    bool		isSmoothTypeTimeAverage();
    void		smoothTypeSel(CallBacker*);
    void		smoothWindowSel(CallBacker*);

    uiRayTracerSel*	raytracerfld_;
    uiGenInput*		smoothtypefld_;
    uiGenInput*		smoothwindowfld_;
    uiGenInput*		smoothwinparamfld_;
    uiGenInput*		smoothwinlengthfld_;
    uiGenInput*		freqf3fld_;
    uiGenInput*		freqf4fld_;
    uiLabel*		smoothwinparamlbl_;
    uiLabel*		smoothwinlengthlbl_;
    uiLabel*		freqf3lbl_;
    uiLabel*		freqf4lbl_;

    bool		isformute_;
    PreStack::AngleCompParams&	params_;
};


mExpClass(uiPreStackProcessing) uiAngleMute : public uiDialog
{
public:

    static void		initClass();
			uiAngleMute(uiParent*,AngleMute*);

protected:

    bool		acceptOK(CallBacker*);
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

#endif

