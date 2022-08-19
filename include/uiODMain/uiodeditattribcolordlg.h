#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uiodmainmod.h"
#include "uidialog.h"

class uiColorTableGroup;
class uiTreeItem;

mExpClass(uiODMain) uiODEditAttribColorDlg : public uiDialog
{ mODTextTranslationClass(uiODEditAttribColorDlg);
public:
    				uiODEditAttribColorDlg(uiParent*,
						       ObjectSet<uiTreeItem>&,
						       const char* attrnm);
protected:
	
	uiColorTableGroup*		uicoltab_;
	ObjectSet<uiTreeItem>&	items_;

	void			seqChg(CallBacker*);
	void			mapperChg(CallBacker*);
	bool			acceptOK(CallBacker*);
};
