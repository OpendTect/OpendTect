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
#include "objectset.h"

class uiMenu;
class uiThemeSel;
class uiTreeView;
class uiLanguageSel;
class PluginPackage;
class PluginProvider;


mExpClass(uiTools) uiPluginSel : public uiDialog
{mODTextTranslationClass(uiPluginSel);
public:
				uiPluginSel(uiParent*);
				~uiPluginSel();

    int				nrPackages() const { return packages_.size(); }

    static const char*		sKeyDoAtStartup();

protected:

    uiTreeView*			treefld_;
    uiThemeSel*			themesel_;
    uiLanguageSel*		languagesel_;
    uiMenu&			rightclickmenu_;

    void			readPackageList();
    void			makePackageList();
    void			createUI();
    int				getPackageIndex(const char*) const;
    bool			isProviderSelected(int) const;
    int				getProviderIndex(const char*) const;
    void			readProviderList();
    bool			fillRightClickMenu(const BufferString&,bool);
    void			launchURL(const char*);
    void			doPkgMnu(const PluginPackage&);
    void			doProvMnu(const PluginProvider&);

    bool			acceptOK();
    void			rightClickCB(CallBacker*);

    ObjectSet<PluginPackage>	packages_;
    ObjectSet<PluginProvider>	providers_;

};
