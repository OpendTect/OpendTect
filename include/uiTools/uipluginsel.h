#ifndef uipluginsel_h
#define uipluginsel_h

/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        A.H. Bril
 Date:          Feb 2006
 RCS:           $Id: uipluginsel.h,v 1.1 2006-02-16 12:36:39 cvsbert Exp $
________________________________________________________________________

-*/

#include "uidialog.h"
#include "bufstringset.h"
class uiCheckBox;

class uiPluginSel : public uiDialog
{
public:
    				uiPluginSel(uiParent*);

    static const char*		sKeyDoAtStartup;

protected:

    BufferStringSet		pluginnms_;
    ObjectSet<uiCheckBox>	cbs_;
    bool			rejectOK(CallBacker*);

};


#endif
