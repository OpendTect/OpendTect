#ifndef uiodeditattribcolordlg_h
#define uiodeditattribcolordlg_h

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	R. K. Singh
 Date:		Feb 2008
 RCS:		$Id: uiodeditattribcolordlg.h,v 1.4 2009-01-08 10:47:25 cvsranojay Exp $
________________________________________________________________________


-*/

#include "uidialog.h"

class uiColorTable;
class uiTreeItem;


mClass uiODEditAttribColorDlg : public uiDialog
{
public:
    				uiODEditAttribColorDlg(uiParent*,
						       ObjectSet<uiTreeItem>&,
						       const char*);

protected:
	
	uiColorTable*		uicoltab_;
	ObjectSet<uiTreeItem>&	items_;
	int			itemusedineditor_;

	void			initApplyButton(CallBacker*);
	void			doApply(CallBacker*);
	bool			acceptOK(CallBacker*);
};


#endif
