#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uiseismod.h"

#include "elasticpropsel.h"
#include "multiid.h"
#include "uidialog.h"
#include "uigroup.h"
#include "uistring.h"

class CtxtIOObj;
class IOObj;
class PropertyRefSelection;

class uiComboBox;
class uiGenInput;
class uiLabeledComboBox;
class uiTabStack;


mExpClass(uiSeis) uiElasticPropSelGrp : public uiGroup
{ mODTextTranslationClass(uiElasticPropSelGrp);
public:
				uiElasticPropSelGrp(uiParent*,
					const PropertyRefSelection& prs,
					ElasticPropertyRef&,
				     const ObjectSet<const ElasticFormula>&);
				~uiElasticPropSelGrp();

    bool			isDefinedQuantity() const;

    void			putToScreen();
    bool			getFromScreen();

private:

    uiGenInput*                 formfld_;
    uiLabeledComboBox*		selmathfld_;
    const PropertyRefSelection& prs_;

    ElasticPropertyRef&		elpropref_;
    const ObjectSet<const ElasticFormula> availableformulas_;

    mExpClass(uiSeis) uiSelInpGrp : public uiGroup
    { mODTextTranslationClass(uiSelInpGrp);
    public:
				uiSelInpGrp(uiParent*,
					    const PropertyRefSelection& prs,
					    int);
				~uiSelInpGrp();

	void			use(const Math::Formula&);
	void			set(Math::Formula&) const;

    private:

	void			initGrp(CallBacker*);
	void			selVarCB(CallBacker*);
	const char*		textOfVariable() const;
	void			setConstant(double val);
	void			setVariable(const char*);

	int			idx_;
	bool			isactive_ = false;
	bool			isconstant_;
	const PropertyRefSelection& prs_;

	uiComboBox*		inpfld_;
	uiGenInput*		varnmfld_;
	uiGenInput*		ctefld_;
    };

    ObjectSet<uiSelInpGrp>	inpgrps_;
    uiLabeledComboBox*		singleinpfld_;

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

    ObjectSet<uiElasticPropSelGrp> propflds_;
    uiTabStack*			ts_			= nullptr;
    CtxtIOObj&			ctio_;

    const PropertyRefSelection& prs_;
    ElasticPropSelection&	elpropsel_;
    ElasticPropSelection	orgelpropsel_;

    bool			doRead(const IOObj&);
    bool			doStore(const IOObj&);

    void			updateFields();
    bool			openPropSel();
    void			openPropSelCB(CallBacker*) { openPropSel(); }
    bool			savePropSel();
    void			savePropSelCB(CallBacker*) { savePropSel(); }
    bool			acceptOK(CallBacker*) override;
    bool			rejectOK(CallBacker*) override;
    void			elasticPropSelectionChanged(CallBacker*);
    bool			screenSelectionChanged();
    void			screenSelectionChangedCB(CallBacker*);
};
