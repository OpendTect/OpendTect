#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uiemattribmod.h"
#include "uidialog.h"

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
    bool		usesingle_		= true;
    bool		isoverwrite_		= false;
    uiBatchJobDispatcherSel*	batchfld_;
};
