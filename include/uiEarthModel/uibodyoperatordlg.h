#ifndef uibodyoperatordlg_h
#define uibodyoperatordlg_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Yuancheng Liu
 Date:		Feb 2009
 RCS:		$Id: uibodyoperatordlg.h,v 1.2 2009-07-22 16:01:21 cvsbert Exp $
________________________________________________________________________


-*/

#include "uidialog.h"
#include "multiid.h"

class uiListView;
class uiListViewItem;
class uiPushButton;
class uiToolButton;
class uiLabeledComboBox;
class uiGenInput;

namespace EM { class BodyOperator; }


mClass uiBodyOperatorDlg : public uiDialog
{
public:
    			uiBodyOperatorDlg(uiParent*,EM::BodyOperator&);
 			~uiBodyOperatorDlg();   
    bool		acceptOK(CallBacker*);

protected:

    void			itemClick(CallBacker*);
    void			bodySel(CallBacker*);
    void			oprSel(CallBacker*);
    void			typeSel(CallBacker*);
    void			turnOffAll();
    void			deleteAllChildInfo(uiListViewItem*);
    void			setOprator(uiListViewItem* lv, 
					   EM::BodyOperator& opt);

    static char			sKeyUnion()	{ return 0; }
    static char			sKeyIntSect()	{ return 1; }
    static char			sKeyMinus()	{ return 2; }
    static char			sKeyUdf()	{ return -1; }

    mStruct bodyOprand
    {
				bodyOprand();
	bool			operator==(const bodyOprand&) const;
	MultiID			mid;
	char 			act;
	bool			defined;
    };

    EM::BodyOperator&		oprt_;

    uiLabeledComboBox*		oprselfld_;
    uiToolButton*		unionbut_;
    uiToolButton*		intersectbut_;
    uiToolButton*		minusbut_;

    uiLabeledComboBox*		typefld_;
    uiGenInput*			bodyselfld_;
    uiPushButton*		bodyselbut_;

    uiListView*			tree_;
    TypeSet<bodyOprand>	listinfo_;
    ObjectSet<uiListViewItem>	listsaved_;
};


#endif
