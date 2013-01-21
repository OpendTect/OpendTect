#ifndef uibodyoperatordlg_h
#define uibodyoperatordlg_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Yuancheng Liu
 Date:		Feb 2009
 RCS:		$Id$
________________________________________________________________________


-*/

#include "uiearthmodelmod.h"
#include "uiearthmodelmod.h"
#include "multiid.h"
#include "uidialog.h"
#include "uiioobjsel.h"

class uiGenInput;
class uiLabeledComboBox;
class uiTreeView;
class uiTreeViewItem;
class uiPushButton;
class uiToolButton;

namespace EM { class BodyOperator; }


mExpClass(uiEarthModel) uiBodyOperatorDlg : public uiDialog
{
public:
    			uiBodyOperatorDlg(uiParent*);
 			~uiBodyOperatorDlg();  

    const MultiID	getBodyMid() const { return outputfld_->key(); }			
protected:

    bool		acceptOK(CallBacker*);
    void		itemClick(CallBacker*);
    void		bodySel(CallBacker*);
    void		oprSel(CallBacker*);
    void		typeSel(CallBacker*);
    void		turnOffAll();
    void		deleteAllChildInfo(uiTreeViewItem*);
    void		setOprator(uiTreeViewItem* lv,EM::BodyOperator& opt);

    static char		sKeyUnion()	{ return 0; }
    static char		sKeyIntSect()	{ return 1; }
    static char		sKeyMinus()	{ return 2; }
    static char		sKeyUdf()	{ return -1; }

    mStruct(uiEarthModel) bodyOprand
    {
			bodyOprand();
	bool		operator==(const bodyOprand&) const;
	MultiID		mid;
	char 		act;
	bool		defined;
    };

    uiLabeledComboBox*		oprselfld_;
    uiToolButton*		unionbut_;
    uiToolButton*		intersectbut_;
    uiToolButton*		minusbut_;

    uiLabeledComboBox*		typefld_;
    uiGenInput*			bodyselfld_;
    uiPushButton*		bodyselbut_;

    uiTreeView*			tree_;
    TypeSet<bodyOprand>		listinfo_;
    ObjectSet<uiTreeViewItem>	listsaved_;

    uiIOObjSel*			outputfld_;
};


#endif


