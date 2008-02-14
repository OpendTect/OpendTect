#ifndef uiodeditattribcolordlg_h
#define uiodeditattribcolordlg_h

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	R. K. Singh
 Date:		Feb 2008
 RCS:		$Id: uiodeditattribcolordlg.h,v 1.2 2008-02-14 07:01:29 cvsraman Exp $
________________________________________________________________________


-*/

#include "uidialog.h"

class ColorTableEditor;
class uiTreeItem;


class uiODEditAttribColorDlg : public uiDialog
{
public:
    				uiODEditAttribColorDlg(uiParent*,
						       ObjectSet<uiTreeItem>&,
						       const char*);

protected:
	
	ColorTableEditor*	coltabed_;
	ObjectSet<uiTreeItem>&	items_;
	int			itemusedineditor_;

	void			initApplyButton(CallBacker*);
	void			doApply(CallBacker*);
	bool			acceptOK(CallBacker*);
};


#endif
