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
				     const BufferStringSet&,
				     ElasticPropertyRef&,
				     const ObjectSet<const ElasticFormula>&);
				~uiElasticPropSelGrp();

    void			setPropRef( const ElasticPropertyRef& pr )
				{ elpropref_ = pr; }

    void			getFromScreen();
    void			putToScreen();

    const char*			quantityName() const;
    bool			isDefinedQuantity() const;

    void			updateRefPropNames();

protected:

    uiGenInput*                 formfld_;
    uiLabeledComboBox*		selmathfld_;
    const BufferStringSet&	propnms_;

    ElasticPropertyRef&		elpropref_;
    const ObjectSet<const ElasticFormula> availableformulas_;

    mExpClass(uiSeis) uiSelInpGrp : public uiGroup
    { mODTextTranslationClass(uiSelInpGrp);
    public:
				uiSelInpGrp(uiParent*,const BufferStringSet&,
					    int);
				~uiSelInpGrp();

	const char*		textOfVariable() const;
	void			setConstant(double val);
	void			setVariable(const char*);

	bool			isActive()	{ return isactive_; }
	void			use(const Math::Formula&);

	void			fillList();
    protected:
	int			idx_;
	bool			isactive_;
	bool			isconstant_;
	const BufferStringSet&	propnms_;

	uiComboBox*		inpfld_;
	uiGenInput*		varnmfld_;
	uiGenInput*		ctefld_;

	void			initGrp(CallBacker*);
	void			selVarCB(CallBacker*);
    };
    ObjectSet<uiSelInpGrp>	inpgrps_;
    uiLabeledComboBox*		singleinpfld_;

    uiGenInput*			storenamefld_;
    uiSeparator*		storenamesep_;

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

    BufferStringSet		orgpropnms_;
    BufferStringSet		propnms_;

    ObjectSet<uiElasticPropSelGrp> propflds_;
    uiTabStack*			ts_			= nullptr;
    CtxtIOObj&			ctio_;

    const PropertyRefSelection& prs_;
    ElasticPropSelection&	elpropsel_;
    ElasticPropSelection	orgelpropsel_;

    bool			doRead(const MultiID&);
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
    void			screenSelectionChanged(CallBacker*);
};
