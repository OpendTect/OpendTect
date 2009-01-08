#ifndef uipluginman_h
#define uipluginman_h

/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        A.H. Bril
 Date:          Oct 2003
 RCS:           $Id: uipluginman.h,v 1.4 2009-01-08 07:07:01 cvsranojay Exp $
________________________________________________________________________

-*/

#include "uidialog.h"
class uiListBox;
class uiTextEdit;
class uiCheckBox;


/*!\brief Shows loaded plugins and allows adding */

mClass uiPluginMan : public uiDialog
{ 	
public:
			uiPluginMan(uiParent*);

protected:

    uiListBox*		listfld;
    uiTextEdit*		infofld;
    uiCheckBox*		selatstartfld;

    bool		rejectOK(CallBacker*);
    void		fillList();
    void		selChg(CallBacker*);
    void		loadPush(CallBacker*);

};


#endif
