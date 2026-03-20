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

namespace Attrib { class CurrentSel; class DescID; class DescSet; }

mExpClass(uiEMAttrib) uiStratAmpCalc : public uiDialog
{ mODTextTranslationClass(uiStratAmpCalc)
public:

    mExpClass( uiEMAttrib ) Setup : public uiDialog::Setup
    {
	public:
			Setup(const char* defmapnm="Stratal Amplitude",
			      bool allowattributes=true);
			~Setup();

			mDefSetupMemb(const char*,defmapname)
			mDefSetupMemb(bool,allowattributes)
    };

			uiStratAmpCalc(uiParent*,const Setup& =Setup());
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
    Attrib::DescSet*	getFromInpFld(TypeSet<Attrib::DescID>&,int&);
    bool		acceptOK(CallBacker*) override;

    Attrib::CurrentSel& sel_;
    TypeSet<int>	seloutputs_;
    BufferStringSet	seloutnms_;

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
    bool		allowattributes_;
    uiBatchJobDispatcherSel*	batchfld_;

};
