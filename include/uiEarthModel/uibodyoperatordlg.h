#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Yuancheng Liu
 Date:		Feb 2009
________________________________________________________________________


-*/

#include "uiearthmodelmod.h"
#include "multiid.h"
#include "uidialog.h"

class IOObj;
class uiBodySel;
class uiGenInput;
class uiLabeledComboBox;
class uiPushButton;
class uiToolButton;
class uiTreeView;
class uiTreeViewItem;

namespace EM { class BodyOperator; }


mExpClass(uiEarthModel) uiBodyOperatorDlg : public uiDialog
{  mODTextTranslationClass(uiBodyOperatorDlg);
public:
    			uiBodyOperatorDlg(uiParent*);
 			~uiBodyOperatorDlg();

    MultiID		getBodyMid() const;

protected:

    void		finalizeCB(CallBacker*);
    bool		acceptOK(CallBacker*);
    void		itemClick(CallBacker*);
    void		bodySel(CallBacker*);
    void		oprSel(CallBacker*);
    void		typeSel(CallBacker*);
    void		turnOffAll();
    void		deleteAllChildInfo(uiTreeViewItem*);
    void		setOperator(uiTreeViewItem* lv,EM::BodyOperator& opt);
    void		displayAction(char item,int curidx);

    static char		sKeyUnion()	{ return 0; }
    static char		sKeyIntSect()	{ return 1; }
    static char		sKeyMinus()	{ return 2; }
    static char		sKeyUdf()	{ return -1; }

    mStruct(uiEarthModel) BodyOperand
    {
			BodyOperand();

	bool		operator==(const BodyOperand&) const;
	bool		isOK() const;

	MultiID		mid_;
	char 		act_;
	bool		defined_;
    };

    uiLabeledComboBox*		oprselfld_;

    uiLabeledComboBox*		typefld_;
    uiGenInput*			bodyselfld_;
    uiPushButton*		bodyselbut_;

    uiTreeView*			tree_;
    TypeSet<BodyOperand>	listinfo_;
    ObjectSet<uiTreeViewItem>	listsaved_;

    uiBodySel*			outputfld_;
};


mExpClass(uiEarthModel) uiImplicitBodyValueSwitchDlg : public uiDialog
{ mODTextTranslationClass(uiImplicitBodyValueSwitchDlg);
public:
    			uiImplicitBodyValueSwitchDlg(uiParent*,const IOObj*);
			~uiImplicitBodyValueSwitchDlg();

    MultiID		getBodyMid() const;

protected:

    bool		acceptOK(CallBacker*);
    const IOObj*	getIfMCSurfaceObj() const;
			/* For bodies made in older version
			   Translator group name : MarchingCubesSurface */

    uiBodySel*		inputfld_;
    uiBodySel*		outputfld_;
};
