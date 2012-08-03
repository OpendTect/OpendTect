#ifndef uiodeditattribcolordlg_h
#define uiodeditattribcolordlg_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	R. K. Singh
 Date:		Feb 2008
 RCS:		$Id: uiodeditattribcolordlg.h,v 1.7 2012-08-03 13:01:03 cvskris Exp $
________________________________________________________________________


-*/

#include "uiodmainmod.h"
#include "uidialog.h"

class uiColorTable;
class uiTreeItem;

mClass(uiODMain) uiODEditAttribColorDlg : public uiDialog
{
public:
    				uiODEditAttribColorDlg(uiParent*,
						       ObjectSet<uiTreeItem>&,
						       const char* attrnm);
protected:
	
	uiColorTable*		uicoltab_;
	ObjectSet<uiTreeItem>&	items_;

	void			seqChg(CallBacker*);
	void			mapperChg(CallBacker*);
	bool			acceptOK(CallBacker*);
};

#endif

