#ifndef uiodeditattribcolordlg_h
#define uiodeditattribcolordlg_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	R. K. Singh
 Date:		Feb 2008
 RCS:		$Id$
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

#endif

