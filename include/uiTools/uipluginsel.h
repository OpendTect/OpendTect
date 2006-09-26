#ifndef uipluginsel_h
#define uipluginsel_h

/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        A.H. Bril
 Date:          Feb 2006
 RCS:           $Id: uipluginsel.h,v 1.2 2006-09-26 18:53:32 cvskris Exp $
________________________________________________________________________

-*/

#include "uidialog.h"
#include "bufstringset.h"
class uiCheckBox;

class uiPluginSel : public uiDialog
{
public:
    				uiPluginSel(uiParent*);
    int				nrPlugins() const { return pluginnms_.size(); }

    static const char*		sKeyDoAtStartup;

protected:

    BufferStringSet		pluginnms_;
    ObjectSet<uiCheckBox>	cbs_;
    bool			rejectOK(CallBacker*);

};


#endif
