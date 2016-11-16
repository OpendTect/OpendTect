#pragma once
/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bruno
 Date:          April 2011
________________________________________________________________________

-*/

#include "uiseismod.h"
#include "dbkey.h"
#include "elasticpropsel.h"
#include "uistring.h"
#include "uigroup.h"
#include "uidialog.h"

class CtxtIOObj;
class IOObj;
class PropertyRefSelection;
namespace Math { class Expression; }

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
					 const TypeSet<ElasticFormula>&);

    void			setPropRef(const ElasticPropertyRef& pr)
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
    ElasticFormula&		elformsel_;
    const TypeSet<ElasticFormula> availableformulas_;

    Math::Expression*		expr_;

    mExpClass(uiSeis) uiSelInpGrp : public uiGroup
    { mODTextTranslationClass(uiSelInpGrp);
    public:
			uiSelInpGrp(uiParent*,const BufferStringSet&,int);

	const char*		textOfVariable() const;
	void			setVariable(const char*,float val);

	bool			isActive()	{ return isactive_; }
	void			use(Math::Expression*);

	void			fillList();
    protected:
	int			idx_;
	bool			isactive_;
	bool			isconstant_;
	const BufferStringSet&	propnms_;

	uiComboBox*		inpfld_;
	uiGenInput*		varnmfld_;
	uiGenInput*		ctefld_;

	void			selVarCB(CallBacker*);
    };
    ObjectSet<uiSelInpGrp>	inpgrps_;
    uiLabeledComboBox*		singleinpfld_;

    uiGenInput*			storenamefld_;
    uiSeparator*		storenamesep_;

    void			getMathExpr();
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
    const DBKey&		storedKey() const	{ return storedmid_; }
    bool			propSaved() const	{ return propsaved_; }

protected:

    BufferStringSet		orgpropnms_;
    BufferStringSet		propnms_;

    ObjectSet<uiElasticPropSelGrp> propflds_;
    uiTabStack*			ts_;
    CtxtIOObj&			ctio_;

    ElasticPropSelection&	elpropsel_;
    ElasticPropSelection&	orgelpropsel_;
    DBKey			storedmid_;
    bool			propsaved_;

    bool			doRead(const DBKey&);
    bool			doStore(const IOObj&);

    void			updateFields();
    bool			openPropSel();
    void                        openPropSelCB(CallBacker*) { openPropSel(); }
    bool			savePropSel();
    void                        savePropSelCB(CallBacker*) { savePropSel(); }
    bool			acceptOK();
    bool			rejectOK();
    void			elasticPropSelectionChanged(CallBacker*);
    void			screenSelectionChangedCB(CallBacker*);
    bool			screenSelectionChanged();
};
