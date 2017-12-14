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

class uiButton;
class uiCheckBox;
struct PluginPackage;
struct PluginProvider;
class IOPar;


mExpClass(uiTools) uiPluginSel : public uiDialog
{mODTextTranslationClass(uiPluginSel);
public:
				uiPluginSel(uiParent*);
				~uiPluginSel();

    int				nrPackages() const { return packages_.size(); }

    static const char*		sKeyDoAtStartup();

protected:

    void			readPackageList();
    void			makePackageList();
    void			createUI();
    int				getPackageIndex(const char*) const;
    bool			isProviderSelected(int) const;
    int				getProviderIndex(const char*) const;
    void			readProviderList();

    bool			acceptOK();

    ObjectSet<PluginPackage>	packages_;
    ObjectSet<PluginProvider>	providers_;

};
