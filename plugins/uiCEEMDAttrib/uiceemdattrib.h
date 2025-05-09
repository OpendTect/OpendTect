#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "gendefs.h"

#include "attribdesc.h"
#include "attribdescid.h"
#include "uiattrdesced.h"
#include "uiattribpanel.h"

namespace Attrib { class Desc; }
class uiAttrSel;
class uiCEEMDPanel;
class uiGenInput;
class uiLabel;
class uiPushButton;
class uiSpecDecompPanel;
class uiTrcPositionDlg;
class uiLabeledSpinBox;

mClass(uiCEEMDAttrib) uiCEEMDAttrib : public uiAttrDescEd
{ mODTextTranslationClass(uiCEEMDAttrib)
public:

			uiCEEMDAttrib(uiParent*,bool);
			~uiCEEMDAttrib();

private:

    uiAttrSel*		inpfld_;
    uiGenInput*		methodfld_;
    uiGenInput*		maximffld_;
    uiGenInput*		stopimffld_;
    uiGenInput*		maxsiftfld_;
    uiGenInput*		stopsiftfld_;
    uiLabeledSpinBox*	outputfreqfld_;
    uiLabeledSpinBox*	stepoutfreqfld_;
    uiGenInput*		attriboutputfld_;
    uiGenInput*		outputcompfld_;

    uiPushButton*	tfpanelbut_;
    uiCEEMDPanel*	panelview_	= nullptr; //!< Time Frequency panel
    uiTrcPositionDlg*	positiondlg_	= nullptr;
    IOPar		prevpar_;

    void		initGrp(CallBacker*);
    void		panelTFPush(CallBacker*);
    void		outSel(CallBacker*);
    void		stepChg(CallBacker*);
    void		getInputMID(MultiID&) const;
    void		setPrevSel();
    void		getPrevSel();
    void		viewPanelCB(CallBacker*);
    void		inputSelCB(CallBacker*);
    DescID		createCEEMDDesc(DescSet*) const;
    RefMan<Desc>	createNewDesc(DescSet*,DescID,const char*,
				      int inpidx,BufferString) const;
    void		fillInCEEMDDescParams(Desc*) const;

    bool		setParameters(const Attrib::Desc&) override;
    bool		setInput(const Attrib::Desc&) override;
    bool		getParameters(Attrib::Desc&) override;
    bool		getInput(Attrib::Desc&) override;
    bool		getOutput(Attrib::Desc&) override;

    mDeclReqAttribUIFns
};


mClass(uiCEEMDAttrib) uiCEEMDPanel : public uiAttribPanel
{ mODTextTranslationClass(uiCEEMDPanel)
public:
				uiCEEMDPanel(uiParent*);
				~uiCEEMDPanel();

private:

    const char*			getProcName() const override;
    const char*			getPackName() const override;
    const char*			getPanelName() const override;

};
