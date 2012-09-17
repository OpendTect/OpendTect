#ifndef uipluginsel_h
#define uipluginsel_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        A.H. Bril
 Date:          Feb 2006
 RCS:           $Id: uipluginsel.h,v 1.5 2009/07/22 16:01:23 cvsbert Exp $
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
