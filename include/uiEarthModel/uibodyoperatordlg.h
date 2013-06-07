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

#include "multiid.h"
#include "uidialog.h"
#include "uiioobjsel.h"

class uiGenInput;
class uiLabeledComboBox;
class uiListView;
class uiListViewItem;
class uiPushButton;
class uiToolButton;

namespace EM { class BodyOperator; }


mClass uiBodyOperatorDlg : public uiDialog
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
    void		deleteAllChildInfo(uiListViewItem*);
    void		setOprator(uiListViewItem* lv,EM::BodyOperator& opt);

    static char		sKeyUnion()	{ return 0; }
    static char		sKeyIntSect()	{ return 1; }
    static char		sKeyMinus()	{ return 2; }
    static char		sKeyUdf()	{ return -1; }

    mStruct bodyOprand
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

    uiListView*			tree_;
    TypeSet<bodyOprand>		listinfo_;
    ObjectSet<uiListViewItem>	listsaved_;

    uiIOObjSel*			outputfld_;
};


#endif
