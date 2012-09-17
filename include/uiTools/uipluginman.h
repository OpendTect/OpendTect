#ifndef uipluginman_h
#define uipluginman_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        A.H. Bril
 Date:          Oct 2003
 RCS:           $Id: uipluginman.h,v 1.5 2009/07/22 16:01:23 cvsbert Exp $
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
