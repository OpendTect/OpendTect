#pragma once

/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : Nageswara
 * DATE     : Mar 2008
-*/

#include "uiemattribmod.h"
#include "uidialog.h"

class CtxtIOObj;
class TrcKeySampling;
class uiAttrSel;
class uiBatchJobDispatcherSel;
class uiGenInput;
class uiIOObjSel;
class uiLabeledComboBox;
class uiPosSubSel;


mClass(uiEMAttrib) uiStratAmpCalc : public uiDialog
{ mODTextTranslationClass(uiStratAmpCalc)
public:
			uiStratAmpCalc(uiParent*);
			~uiStratAmpCalc();

    void		init();

protected:
    void		inpSel(CallBacker*);
    void		horSel(CallBacker*);
    void		choiceSel(CallBacker*);
    void		setParFileNameCB(CallBacker*);
    void		getAvailableRange(TrcKeySampling&);
    bool		prepareProcessing();
    bool		checkInpFlds();
    bool		fillPar();
    void		setParFileName();
    bool		acceptOK(CallBacker*) override;

    CtxtIOObj&		horctio1_;
    CtxtIOObj&		horctio2_;

    uiGenInput*		winoption_;
    uiGenInput*		zoffset_;
    uiGenInput*		tophorshiftfld_;
    uiGenInput*		bothorshiftfld_;
    uiGenInput*		selfld_;
    uiGenInput*		foldfld_;
    uiGenInput*		attribnamefld_;
    uiGenInput*		classfld_;
    uiAttrSel*		inpfld_;
    uiIOObjSel*		horfld1_;
    uiIOObjSel*		horfld2_;
    uiPosSubSel*	rangefld_;
    uiLabeledComboBox*	ampoptionfld_;
    bool		usesingle_;
    bool		isoverwrite_;
    uiBatchJobDispatcherSel*	batchfld_;
};
