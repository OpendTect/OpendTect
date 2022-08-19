#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "gendefs.h"
#include "uiattrdesced.h"
#include "attribdescid.h"
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

class uiCEEMDAttrib : public uiAttrDescEd
{ mODTextTranslationClass(uiCEEMDAttrib)
public:

    uiCEEMDAttrib(uiParent*,bool);

protected:

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
    uiCEEMDPanel*	panelview_;	//!< Time Frequency panel
    uiTrcPositionDlg*	positiondlg_;
    IOPar		prevpar_;

    void		panelTFPush(CallBacker*);
    void		outSel(CallBacker*);
    void		stepChg(CallBacker*);
    void		getInputMID(MultiID&) const;
    void		setPrevSel();
    void		getPrevSel();
    void		viewPanelCB(CallBacker*);
    void		inputSelCB(CallBacker*);
    DescID		createCEEMDDesc(DescSet*) const;
    Desc*		createNewDesc(DescSet*,DescID,const char*,
				      int inpidx,BufferString) const;
    void		fillInCEEMDDescParams(Desc*) const;

    bool		setParameters(const Attrib::Desc&) override;
    bool		setInput(const Attrib::Desc&) override;
    bool		getParameters(Attrib::Desc&) override;
    bool		getInput(Attrib::Desc&) override;
    bool		getOutput(Attrib::Desc&) override;

    mDeclReqAttribUIFns
};


class uiCEEMDPanel	: public uiAttribPanel
{
public:
				uiCEEMDPanel( uiParent* p )
				    : uiAttribPanel( p )		{};

protected:
    virtual const char*		getProcName();
    virtual const char*		getPackName();
    virtual const char*		getPanelName();

};
