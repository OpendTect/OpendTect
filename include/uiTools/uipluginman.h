#ifndef uipluginman_h
#define uipluginman_h

/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        A.H. Bril
 Date:          Oct 2003
 RCS:           $Id: uipluginman.h,v 1.2 2003-11-07 12:21:54 bert Exp $
________________________________________________________________________

-*/

#include "uidialog.h"
class uiListBox;
class uiTextEdit;


/*!\brief Shows loaded plugins and allows adding */

class uiPluginMan : public uiDialog
{ 	
public:
			uiPluginMan(uiParent*);

protected:

    uiListBox*		listfld;
    uiTextEdit*		infofld;

    void		fillList();
    void		selChg(CallBacker*);
    void		loadPush(CallBacker*);

};


#endif
