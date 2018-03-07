#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
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


mExpClass(uiTools) uiODPreStart : public uiDialog
{mODTextTranslationClass(uiODPreStart);
public:
				uiODPreStart(uiParent*);
				~uiODPreStart();

    int				nrPackages() const { return packages_.size(); }

    static const char*		sKeyDoAtStartup();

protected:

    uiThemeSel*			themesel_;
    uiLanguageSel*		languagesel_;
    uiTreeView*			treefld_;
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
