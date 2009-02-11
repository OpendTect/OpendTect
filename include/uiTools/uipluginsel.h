#ifndef uipluginsel_h
#define uipluginsel_h

/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        A.H. Bril
 Date:          Feb 2006
 RCS:           $Id: uipluginsel.h,v 1.4 2009-02-11 10:35:50 cvsranojay Exp $
________________________________________________________________________

-*/

#include "uidialog.h"
#include "bufstringset.h"
class uiCheckBox;

mClass uiPluginSel : public uiDialog
{
public:
    				uiPluginSel(uiParent*);
    int				nrPlugins() const { return pluginnms_.size(); }

    static const char*		sKeyDoAtStartup();

protected:

    BufferStringSet		pluginnms_;
    ObjectSet<uiCheckBox>	cbs_;
    bool			rejectOK(CallBacker*);

};


#endif
