#ifndef uipluginsel_h
#define uipluginsel_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        A.H. Bril
 Date:          Feb 2006
 RCS:           $Id$
________________________________________________________________________

-*/

#include "uitoolsmod.h"
#include "uidialog.h"
#include "bufstringset.h"
#include "separstr.h"

class uiCheckBox;

mExpClass(uiTools) uiPluginSel : public uiDialog
{
public:
    				uiPluginSel(uiParent*);
    int				nrPlugins() const { return pluginnms_.size(); }

    static const char*		sKeyDoAtStartup();

protected:

    BufferStringSet		pluginnms_;
    ObjectSet<uiCheckBox>	cbs_;
    bool			rejectOK(CallBacker*);

    void			checkCB(CallBacker*);
    int				getPluginIndex(const char*,
						ObjectSet<uiCheckBox>&);
    void			setChecked(const char*, bool);
    const char*			getLibName(const char*,
					    const TypeSet<SeparString>&) const;
    void			makeList(BufferStringSet&);
};


#endif

