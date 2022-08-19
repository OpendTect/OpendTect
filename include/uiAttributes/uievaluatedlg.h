#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uiattributesmod.h"
#include "uidialog.h"
#include "uigroup.h"
#include "bufstringset.h"
#include "attribdescid.h"

namespace Attrib
{
    class Desc;
    class DescSet;
    class SelSpec;
    class ValParam;
}

class uiAttrDescEd;
class uiCheckBox;
class DataInpSpec;
class EvalParam;
class uiGenInput;
class uiLabel;
class uiLabeledSpinBox;
class uiPushButton;
class uiSlider;


mExpClass(uiAttributes) AttribParamGroup : public uiGroup
{ mODTextTranslationClass(AttribParamGroup);
public:
				AttribParamGroup(uiParent*,const uiAttrDescEd&,
						 const EvalParam&);
    void			updatePars(Attrib::Desc&,int);
    void			updateDesc(Attrib::Desc&,int);
    const char*			getLabel()		{ return evallbl_; }
    static uiString		sInit();
    static uiString		sIncr();
protected:

    void			createInputSpecs(const Attrib::ValParam*,
						 DataInpSpec*&,DataInpSpec*&);

    uiGenInput*			initfld;
    uiGenInput*			incrfld;
    BufferString		parlbl_;
    BufferString		evallbl_;

    BufferString		parstr1_;
    BufferString		parstr2_;
    int				pgidx_;
    bool			evaloutput_;
    const uiAttrDescEd&		desced_;
};


mExpClass(uiAttributes) uiEvaluateDlg : public uiDialog
{ mODTextTranslationClass(uiEvaluateDlg);
public:
				uiEvaluateDlg(uiParent*,uiAttrDescEd&,
					      bool enabstore=true);
				~uiEvaluateDlg();

    Attrib::Desc*		getAttribDesc() const	{ return seldesc_; }
    void			getEvalSpecs(TypeSet<Attrib::SelSpec>&) const;
    Attrib::DescSet*		getEvalSet() const	{ return attrset_; }
    bool			storeSlices() const;
    bool			evaluationPossible() const { return haspars_; }

    Notifier<uiEvaluateDlg>	calccb;
    CNotifier<uiEvaluateDlg,int> showslicecb;

protected:

    uiGenInput*			evalfld;
    uiPushButton*		calcbut;
    uiSlider*			sliderfld;
    uiLabeledSpinBox*		nrstepsfld;
    uiLabel*			displaylbl;
    uiCheckBox*			storefld;

    void			variableSel(CallBacker*);
    void			calcPush(CallBacker*);
    void			sliderMove(CallBacker*);
    void			doFinalize(CallBacker*);

    bool			acceptOK(CallBacker*) override;

    Attrib::Desc*		seldesc_;
    Attrib::DescSet*		attrset_;
    uiAttrDescEd&		desced_;
    Attrib::DescID		srcid_;

    IOPar&			initpar_;
    BufferStringSet		lbls_;
    ObjectSet<AttribParamGroup>	grps_;

    TypeSet<Attrib::SelSpec>	specs_;
    bool			enabstore_;
    bool			haspars_;
};
