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
#include "plugins.h"
#include "bufstringset.h"

class uiCheckBox;
struct PluginProduct;

mExpClass(uiTools) uiPluginSel : public uiDialog
{mODTextTranslationClass(uiPluginSel);
public:
				uiPluginSel(uiParent*);
				~uiPluginSel();

    int				nrPlugins() const { return products_.size(); }


    static const char*		sKeyDoAtStartup();

protected:

    void			makeProductList(
					const ObjectSet<PluginManager::Data>&);
    void			createUI();

    int				getProductIndex(const char* prodnm) const;
    int				getProductIndexForLib(const char* libnm) const;

    bool			acceptOK(CallBacker*);

    int				maxpluginname_;
    ObjectSet<uiCheckBox>	cbs_;
    ObjectSet<PluginProduct>	products_;
};

#endif

