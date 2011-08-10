#ifndef uielasticpropsel_h
#define uielasticpropsel_h
/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bruno
 Date:          April 2011
 RCS:           $Id: uielasticpropsel.h,v 1.4 2011-08-10 15:03:51 cvsbruno Exp $
________________________________________________________________________

-*/

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


mClass uiElasticPropSelGrp : public uiGroup
{
public:
    				uiElasticPropSelGrp(uiParent*,
					 const BufferStringSet&,
					 ElasticPropertyRef&,
					 const TypeSet<ElasticFormula>&);

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

    mClass uiSelInpGrp : public uiGroup
    {
    public:
			uiSelInpGrp(uiParent*,const BufferStringSet&,int);

	const char*		textOfVariable() const;
	void			setVariable(const char*,float val);

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
    void			selChgCB(CallBacker*);
};


mClass uiElasticPropSelDlg : public uiDialog
{
public:
				uiElasticPropSelDlg(uiParent*,
					const PropertyRefSelection&,
					const MultiID& elpsel);
				~uiElasticPropSelDlg();

    const MultiID& 		storedKey() const { return storedmid_; }
protected:

    BufferStringSet		propnms_;
    BufferStringSet		tmpelasticnms_;
    ObjectSet<uiElasticPropSelGrp> propflds_;
    uiTabStack*			ts_;
    CtxtIOObj&			ctio_;

    ElasticPropSelection	elpropsel_;
    MultiID			storedmid_;

    bool			doSave(const IOObj&);

    bool			openPropSel();
    void                        openPropSelCB(CallBacker*) { openPropSel(); }
    bool			savePropSel(); 
    void                        savePropSelCB(CallBacker*) { savePropSel(); }
    bool			acceptOK(CallBacker*);
    void			elasticPropSelectionChanged(CallBacker*);
    bool			screenSelectionChanged(CallBacker*);
};



#endif
