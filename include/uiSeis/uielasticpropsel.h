#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uiseismod.h"

#include "multiid.h"
#include "elasticpropsel.h"
#include "uistring.h"
#include "uigroup.h"
#include "uidialog.h"

class CtxtIOObj;
class IOObj;
class PropertyRefSelection;

class uiLabeledComboBox;
class uiComboBox;
class uiGenInput;
class uiSeparator;
class uiTabStack;


mExpClass(uiSeis) uiElasticPropSelGrp : public uiGroup
{ mODTextTranslationClass(uiElasticPropSelGrp);
public:
				uiElasticPropSelGrp(uiParent*,
					const PropertyRefSelection& prs,
					ElasticPropertyRef&,
				     const ObjectSet<const ElasticFormula>&);
				mDeprecated("Use PropertyRefSelection")
				uiElasticPropSelGrp(uiParent*,
				     const BufferStringSet&,
				     ElasticPropertyRef&,
				     const ObjectSet<const ElasticFormula>&);
				~uiElasticPropSelGrp();

    mDeprecatedDef
    void			setPropRef( const ElasticPropertyRef& pr )
				{ elpropref_ = pr; }

    void			getFromScreen();
    bool			getFromScreen_();
    void			putToScreen();

    mDeprecatedDef
    const char*			quantityName() const;
    bool			isDefinedQuantity() const;

    mDeprecatedDef
    void			updateRefPropNames();

protected:

    uiGenInput*                 formfld_;
    uiLabeledComboBox*		selmathfld_;
    const BufferStringSet&	propnms_; //deprecated
    const PropertyRefSelection& prs_() const;

    ElasticPropertyRef&		elpropref_;
    const ObjectSet<const ElasticFormula> availableformulas_;

    mExpClass(uiSeis) uiSelInpGrp : public uiGroup
    { mODTextTranslationClass(uiSelInpGrp);
    public:
				uiSelInpGrp(uiParent*,
					    const PropertyRefSelection& prs,
					    int);
				mDeprecated("Use PropertyRefSelection")
				uiSelInpGrp(uiParent*,const BufferStringSet&,
					    int);
				~uiSelInpGrp();

	const char*		textOfVariable() const;
	void			setConstant(double val);
	void			setVariable(const char*);

	mDeprecatedDef
	bool			isActive()	{ return isactive_; }
	void			use(const Math::Formula&);
	void			set(Math::Formula&) const;

	void			fillList();
    protected:
	int			idx_;
	bool			isactive_;
	bool			isconstant_;
	const BufferStringSet&	propnms_; //deprecated
	const PropertyRefSelection& prs_() const;

	uiComboBox*		inpfld_;
	uiGenInput*		varnmfld_;
	uiGenInput*		ctefld_;

	void			initGrp(CallBacker*);
	void			selVarCB(CallBacker*);
    };
    ObjectSet<uiSelInpGrp>	inpgrps_;
    uiLabeledComboBox*		singleinpfld_;

    uiGenInput*			storenamefld_; //deprecated
    uiSeparator*		storenamesep_; //deprecated

    void			initGrpCB(CallBacker*);
    void			selFormulaChgCB(CallBacker*);
    void			selComputeFldChgCB(CallBacker*);
};


mExpClass(uiSeis) uiElasticPropSelDlg : public uiDialog
{ mODTextTranslationClass(uiElasticPropSelDlg);
public:
				uiElasticPropSelDlg(uiParent*,
					const PropertyRefSelection&,
					ElasticPropSelection&);
				~uiElasticPropSelDlg();

    const ElasticPropSelection&	elasticSel() const	{ return elpropsel_; }

protected:

    BufferStringSet		orgpropnms_; //deprecated
    BufferStringSet		propnms_; //deprecated

    ObjectSet<uiElasticPropSelGrp> propflds_;
    uiTabStack*			ts_;
    CtxtIOObj&			ctio_;

    const PropertyRefSelection& prs_;
    ElasticPropSelection&	elpropsel_;
    ElasticPropSelection	orgelpropsel_;

    mDeprecatedDef
    bool			doRead(const MultiID&);
    bool			doRead(const IOObj&);
    bool			doStore(const IOObj&);

    mDeprecatedDef
    void			initDlg(CallBacker*);
    void			updateFields();
    bool			openPropSel();
    void			openPropSelCB(CallBacker*) { openPropSel(); }
    bool			savePropSel();
    void			savePropSelCB(CallBacker*) { savePropSel(); }
    bool			acceptOK(CallBacker*) override;
    bool			rejectOK(CallBacker*) override;
    void			elasticPropSelectionChanged(CallBacker*);
    bool			screenSelectionChanged();
    void			screenSelectionChanged(CallBacker*);
};
