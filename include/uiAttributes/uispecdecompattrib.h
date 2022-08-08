#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Nanne Hemstra
 Date:          January 2003
________________________________________________________________________

-*/

#include "uiattributesmod.h"
#include "multiid.h"
#include "uiattrdesced.h"
#include "uiattribpanel.h"
#include "iopar.h"

namespace Attrib { class Desc; }

class uiGenInput;
class uiImagAttrSel;
class uiLabeledSpinBox;
class uiPushButton;
class uiSpecDecompPanel;
class uiTrcPositionDlg;

/*! \brief Spectral Decomposition Attribute description editor */

mExpClass(uiAttributes) uiSpecDecompAttrib : public uiAttrDescEd
{ mODTextTranslationClass(uiSpecDecompAttrib)
public:

			uiSpecDecompAttrib(uiParent*,bool);
			~uiSpecDecompAttrib();

    void		getEvalParams(TypeSet<EvalParam>&) const override;
    int			getOutputIdx(float) const override;
    float		getOutputValue(int) const override;
protected:

    uiImagAttrSel*	inpfld_;
    uiGenInput*		typefld_;
    uiGenInput*		gatefld_;
    uiLabeledSpinBox*	outpfld_;
    uiLabeledSpinBox*	stepfld_;
    uiGenInput*		waveletfld_;

    bool		setParameters(const Attrib::Desc&) override;
    bool		setInput(const Attrib::Desc&) override;
    bool		setOutput(const Attrib::Desc&) override;

    bool		getParameters(Attrib::Desc&) override;
    bool		getInput(Attrib::Desc&) override;
    bool		getOutput(Attrib::Desc&) override;

    void		inputSel(CallBacker*);
    void		typeSel(CallBacker*);
    void		stepChg(CallBacker*);
    void		panelTFPush(CallBacker*);

    void		checkOutValSnapped() const;
    void		getInputMID(MultiID&) const;
    Attrib::DescID	createSpecDecompDesc(Attrib::DescSet*) const;
    void		createHilbertDesc(Attrib::DescSet*,
					  Attrib::DescID&) const;
    Attrib::Desc*	createNewDesc(Attrib::DescSet*,Attrib::DescID,
				      const char*,int,int,BufferString) const;
    Attrib::Desc*	createNewDescFromDP(Attrib::DescSet*,const char* atrnm,
					    const char* userefstr) const;
    void		fillInSDDescParams(Attrib::Desc*) const;
    bool		passStdCheck(const Attrib::Desc*,const char*,
				     int seloutidx,int inpidx,
				     Attrib::DescID inpid) const;
    void		viewPanalCB(CallBacker*);
    void		setPrevSel();
    void		getPrevSel();
    static const char* sKeyBinID();
    static const char* sKeyLineName();
    static const char* sKeyTrcNr();

    float		nyqfreq_;
    int			nrsamples_; //!< Nr of samples in selected data
    float		ds_; //!< Sample spacing of selected data

    uiPushButton*	tfpanelbut_;
    uiSpecDecompPanel*	panelview_;	//!< Time Frequency panel
    uiTrcPositionDlg*	positiondlg_;
    IOPar		prevpar_;

			mDeclReqAttribUIFns
};


mClass(uiAttributes) uiSpecDecompPanel	: public uiAttribPanel
{ mODTextTranslationClass(uiSpecDecompPanel)
public:
				uiSpecDecompPanel( uiParent* p )
				    : uiAttribPanel( p )		{}

protected:
    const char*		getProcName() override;
    const char*		getPackName() override;
    const char*		getPanelName() override;

};

