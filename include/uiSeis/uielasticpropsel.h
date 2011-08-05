#ifndef uielasticpropsel_h
#define uielasticpropsel_h
/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bruno
 Date:          April 2011
 RCS:           $Id: uielasticpropsel.h,v 1.1 2011-08-05 14:57:46 cvsbruno Exp $
________________________________________________________________________

-*/

#include "elasticpropsel.h"

#include "uigroup.h"
#include "uiobjfileman.h"
#include "uiobjectquicksel.h"
#include "uidialog.h"

class CtxtIOObj;
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
					ElasticPropSelection&);
				~uiElasticPropSelDlg();
protected:

    ObjectSet<uiElasticPropSelGrp> propflds_;
    uiTabStack*			ts_;
    ElasticPropSelection&	elpropsel_;
    CtxtIOObj&			ctio_;

    bool			openPropSel();
    bool			savePropSel(); 
    void                        openPropSelCB(CallBacker*) { openPropSel(); }
    void                        savePropSelCB(CallBacker*) { savePropSel(); }


    bool			acceptOK(CallBacker*);
    void			elasticPropSelectionChanged(CallBacker*);
};



#endif
