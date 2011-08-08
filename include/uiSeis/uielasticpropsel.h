#ifndef uielasticpropsel_h
#define uielasticpropsel_h
/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bruno
 Date:          April 2011
 RCS:           $Id: uielasticpropsel.h,v 1.3 2011-08-08 13:59:22 cvsbruno Exp $
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
class uiIOObjSel;
class uiTabStack;


mClass uiElasticPropSelGrp : public uiGroup
{
public:
    				uiElasticPropSelGrp(uiParent*,
				 const PropertyRefSelection&,ElasticFormula&,
				 const TypeSet<ElasticFormula>&);

    void			getFromScreen();
    void			putToScreen();

protected:
    uiGenInput*                 formfld_;
    uiLabeledComboBox* 		selmathfld_;

    const PropertyRefSelection&	proprefsel_;
    ElasticFormula&		elformsel_;
    const TypeSet<ElasticFormula> availableformulas_;

    BufferStringSet		propnms_;
    MathExpression*	   	expr_;

    mClass uiSelInpGrp : public uiGroup
    {
    public:
			uiSelInpGrp(uiParent*,const BufferStringSet&,int);

	const char*		textOfVariable() const;
	void			setVariable(const char*,float val);

	void			use(MathExpression*);

    protected:
	int 			idx_;
	bool			isactive_;
	bool			isconstant_;
	BufferStringSet		propnms_;

	uiComboBox*		inpfld_;
	uiGenInput* 		varnmfld_;
	uiGenInput*		ctefld_;

	uiGenInput*		storenamefld_;

	void			selVarCB(CallBacker*);
    };
    ObjectSet<uiSelInpGrp> 	inpgrps_;
    uiLabeledComboBox*		singleinpfld_;

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
};



#endif
