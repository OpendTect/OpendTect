#ifndef uipluginman_h
#define uipluginman_h

/*+
________________________________________________________________________

 CopyRight:     (C) de Groot-Bril Earth Sciences B.V.
 Author:        A.H. Bril
 Date:          Oct 2003
 RCS:           $Id: uipluginman.h,v 1.1 2003-10-20 15:38:03 bert Exp $
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
