#ifndef uielasticpropsel_h
#define uielasticpropsel_h
/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bruno
 Date:          April 2011
 RCS:           $Id$
________________________________________________________________________

-*/

#include "uiseismod.h"
#include "multiid.h"
#include "elasticpropsel.h"

#include "uigroup.h"
#include "uidialog.h"

class CtxtIOObj;
class IOObj;
class MultiID;
class MathExpression;
class PropertyRefSelection;

class uiLabeledComboBox;
class uiComboBox;
class uiGenInput;
class uiSeparator;
class uiIOObjSel;
class uiTabStack;


mClass(uiSeis) uiElasticPropSelGrp : public uiGroup
{
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
    uiLabeledComboBox* 		selmathfld_;
    const BufferStringSet&	propnms_;

    ElasticPropertyRef&		elpropref_;
    ElasticFormula&		elformsel_;
    const TypeSet<ElasticFormula> availableformulas_;

    MathExpression*	   	expr_;

    mClass(uiSeis) uiSelInpGrp : public uiGroup
    {
    public:
			uiSelInpGrp(uiParent*,const BufferStringSet&,int);

	const char*		textOfVariable() const;
	void			setVariable(const char*,float val);

	bool			isActive() 	{ return isactive_; }
	void			use(MathExpression*);

	void			fillList();
    protected:
	int 			idx_;
	bool			isactive_;
	bool			isconstant_;
	const BufferStringSet&	propnms_;

	uiComboBox*		inpfld_;
	uiGenInput* 		varnmfld_;
	uiGenInput*		ctefld_;

	void			selVarCB(CallBacker*);
    };
    ObjectSet<uiSelInpGrp> 	inpgrps_;
    uiLabeledComboBox*		singleinpfld_;

    uiGenInput*			storenamefld_;
    uiSeparator*		storenamesep_;

    void			getMathExpr();
    void			selFormulaChgCB(CallBacker*);
    void			selComputeFldChgCB(CallBacker*);
};


mClass(uiSeis) uiElasticPropSelDlg : public uiDialog
{
public:
				uiElasticPropSelDlg(uiParent*,
					const PropertyRefSelection&,
					ElasticPropSelection&);
				~uiElasticPropSelDlg();

    const ElasticPropSelection&	elasticSel() const 	{ return elpropsel_; }
    const MultiID& 		storedKey() const 	{ return storedmid_; }
    bool			propSaved() const 	{ return propsaved_; }

protected:

    BufferStringSet		orgpropnms_;
    BufferStringSet		propnms_;

    ObjectSet<uiElasticPropSelGrp> propflds_;
    uiTabStack*			ts_;
    CtxtIOObj&			ctio_;

    ElasticPropSelection&	elpropsel_;
    ElasticPropSelection&	orgelpropsel_;
    MultiID			storedmid_;
    bool 			propsaved_;

    bool			doRead(const MultiID&);
    bool			doStore(const IOObj&);

    void			updateFields();
    bool			openPropSel();
    void                        openPropSelCB(CallBacker*) { openPropSel(); }
    bool			savePropSel(); 
    void                        savePropSelCB(CallBacker*) { savePropSel(); }
    bool			acceptOK(CallBacker*);
    bool			rejectOK(CallBacker*);
    void			elasticPropSelectionChanged(CallBacker*);
    bool			screenSelectionChanged(CallBacker*);
};



#endif

