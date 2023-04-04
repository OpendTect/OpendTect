#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uiattributesmod.h"
#include "uiattrdesced.h"

#include "attribdescid.h"
#include "ranges.h"

class uiAttrSel;
class uiGenInput;
class uiLabeledSpinBox;
class uiPushButton;
class uiGDPositionDlg;
class GapDeconACorrView;


/*! \brief GapDecon Attribute description editor */

mClass(uiAttributes) uiGapDeconAttrib : public uiAttrDescEd
{ mODTextTranslationClass(uiGapDeconAttrib);
public:
			uiGapDeconAttrib(uiParent*,bool);
			~uiGapDeconAttrib();

    void		getEvalParams(TypeSet<EvalParam>&) const override;
    static const char*	sKeyOnInlineYN();
    static const char*	sKeyLineName();

protected:

    uiAttrSel*		inpfld_;
    uiGenInput*		gatefld_;
    uiGenInput*		lagfld_;
    uiGenInput*		gapfld_;
    uiGenInput*		noiselvlfld_;
    uiGenInput*		isinpzerophasefld_;
    uiGenInput*		isoutzerophasefld_;
    uiGenInput*		wantmixfld_;
    uiLabeledSpinBox*	stepoutfld_;
    uiPushButton*	exambut_;
    uiPushButton*	qcbut_;

    IOPar		par_;
    uiGDPositionDlg*	positiondlg_;
    GapDeconACorrView*	acorrview_;
    bool		setParameters(const Attrib::Desc&) override;
    bool		setInput(const Attrib::Desc&) override;

    bool		getParameters(Attrib::Desc&) override;
    bool		getInput(Attrib::Desc&) override;

    void		finalizeCB(CallBacker*);
    void                examPush(CallBacker*);
    void                qCPush(CallBacker*);
    void                mixSel(CallBacker*);
    bool		passStdCheck(const Attrib::Desc*,const char*,int,int,
				     Attrib::DescID);
    bool		passVolStatsCheck(const Attrib::Desc*,BinID,
					  Interval<float>);
    Attrib::Desc*	createNewDesc(Attrib::DescSet*,Attrib::DescID,
				      const char*,int,int,BufferString);
    Attrib::DescID	createVolStatsDesc(Attrib::Desc&,int);
    void		createHilbertDesc(Attrib::Desc&,Attrib::DescID&);
    Attrib::DescID	createGapDeconDesc(Attrib::DescID&,Attrib::DescID,
					   DescSet*,bool);
    void		prepareInputDescs(Attrib::DescID&,Attrib::DescID&,
					  Attrib::DescSet*);
    void		fillInGDDescParams(Attrib::Desc*);
    void		getInputMID(MultiID&) const;

			mDeclReqAttribUIFns
};
