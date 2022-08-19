#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uiattributesmod.h"
#include "uidialog.h"
#include "bufstringset.h"
#include "attribdescset.h"

class AttribParamGroup;
class uiAttribDescSetEd;
class uiCheckBox;
class uiLabel;
class uiLabeledSpinBox;
class uiListBox;
class uiPushButton;
class uiSlider;

mExpClass(uiAttributes) uiCrossAttrEvaluateDlg : public uiDialog
{ mODTextTranslationClass(uiCrossAttrEvaluateDlg);
public:
				uiCrossAttrEvaluateDlg(uiParent*,
					uiAttribDescSetEd&,bool enabstore=true);
				~uiCrossAttrEvaluateDlg();

    Attrib::Desc*		getAttribDesc() const   { return seldesc_; }
    void			getEvalSpecs(TypeSet<Attrib::SelSpec>&) const;
    Attrib::DescSet*		getEvalSet() const	{ return &attrset_; }
    bool			storeSlices() const;
    bool			evaluationPossible() const { return haspars_; }
    const TypeSet<Attrib::DescID>& evaluateChildIds() const
				{ return seldeschildids_; }
    BufferString		acceptedDefStr() const;

    Notifier<uiCrossAttrEvaluateDlg>		calccb;
    CNotifier<uiCrossAttrEvaluateDlg,int>	showslicecb;

protected:

    uiListBox*			paramsfld_;
    uiListBox*			attrnmsfld_;

    uiPushButton*		calcbut;
    uiSlider*			sliderfld;
    uiLabeledSpinBox*		nrstepsfld;
    uiLabel*			displaylbl;
    uiCheckBox*			storefld;

    void			parameterSel(CallBacker*);
    void			calcPush(CallBacker*);
    void			sliderMove(CallBacker*);
    void			doFinalize(CallBacker*);
    void			getSelDescIDs(
					TypeSet<TypeSet<Attrib::DescID> >&,
					TypeSet<TypeSet<int> >&);

    bool			acceptOK(CallBacker*) override;

    Attrib::Desc*		seldesc_;
    Attrib::DescID		srcid_;
    Attrib::DescSet&		attrset_;
    TypeSet<BufferStringSet>	userattnms_;//per parameter

    IOPar&			initpar_;
    ObjectSet<AttribParamGroup>	grps_;

    TypeSet<Attrib::DescID>	srcspecids_;
    TypeSet<Attrib::DescID>	seldeschildids_;
    BufferStringSet		lbls_; //size is nr of steps
    TypeSet<Attrib::SelSpec>	specs_;//size is nr of steps

    BufferStringSet		defstr_;
    bool			enabstore_;
    bool			haspars_;
};
