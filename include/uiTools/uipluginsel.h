#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        A.H. Bril
 Date:          Feb 2006
________________________________________________________________________

-*/

#include "uitoolsmod.h"
#include "uidialog.h"
#include "uitreeview.h"
#include "plugins.h"
#include "bufstringset.h"

class uiButton;
class uiCheckBox;
class uiTreeView;
struct PluginProduct;
struct PluginVendor;
class IOPar;


mExpClass(uiTools) uiPluginSel : public uiDialog
{mODTextTranslationClass(uiPluginSel);
public:
				uiPluginSel(uiParent*);
				~uiPluginSel();

    int				nrPlugins() const { return products_.size(); }


    static const char*		sKeyDoAtStartup();
    static const char*		sKeyLicInstallExe();

protected:

    void			readPackageList();
    void			makeProductList(
					const ObjectSet<PluginManager::Data>&);
    void			createUI();
    int				getProductIndex(const char* prodnm) const;
    bool			isVendorSelected(int) const;
    int				getVendorIndex(const char*) const;
    void			readVendorList();

    void			finalizeCB(CallBacker*);
    void			startLicInstallCB(CallBacker*);
    void			showLicInstallCB(CallBacker*);
    void			licInstallDlgClosed(CallBacker*);

    bool			acceptOK(CallBacker*);

    ObjectSet<PluginProduct>	products_;
    ObjectSet<PluginVendor>	vendors_;
    uiTreeView*			treefld_;
};

