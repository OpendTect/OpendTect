#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2026 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uiodmainmod.h"

#include "factory.h"
#include "uiodtreeitem.h"

class uiAmplSpectrum;
class uiFKSpectrum;
class uiMenuHandler;
class uiStatsDisplayWin;
class uiVisPartServer;
namespace Attrib { class SelSpec; }
namespace ColTab { class Sequence; }


/*!Base class for a data treeitem. */

mExpClass(uiODMain) uiODDataTreeItem : public uiTreeItem
{ mODTextTranslationClass(uiODDataTreeItem)
public:

    virtual void		show(bool yn)		{}
    bool			select() override;
    [[nodiscard]] VisID		displayID() const;
    [[nodiscard]] int		attribNr() const;

    [[nodiscard]] static int	cPixmapWidth()		{ return 16; }
    [[nodiscard]] static int	cPixmapHeight()		{ return 10; }

				mDefineFactory2ParamInClass(uiODDataTreeItem,
					const Attrib::SelSpec&,const char*,
					factory )

				/*!<Adds custom create function for create
				    function. */

    void			prepareForShutdown() override;

protected:
				uiODDataTreeItem(const char* parenttype);
				~uiODDataTreeItem();

    [[nodiscard]] int		uiTreeViewItemType() const override;
    bool			init() override;

    virtual void		checkCB(CallBacker*);
    void			keyPressCB(CallBacker*);
    [[nodiscard]] bool		shouldSelect(int) const override;

    [[nodiscard]] virtual bool	hasTransparencyMenu() const { return true; }

    [[nodiscard]] uiODApplMgr*	applMgr() const;
    uiVisPartServer*		visserv_;
    [[nodiscard]] SceneID	sceneID() const;
    [[nodiscard]] bool		isSelectable() const override { return true; }
    [[nodiscard]] bool		isExpandable() const override { return false; }
    [[nodiscard]] const char*	parentType() const override
				{ return parenttype_; }

    bool			showSubMenu() override;

    virtual void		createMenu(MenuHandler*,bool istoolbar);
    void			addToToolBarCB(CallBacker*);
    void			createMenuCB(CallBacker*);
    virtual void		handleMenuCB(CallBacker*);
    void			updateColumnText(int col) override;
    [[nodiscard]] virtual uiString createDisplayName() const		= 0;

    void			displayMiniCtab( const ColTab::Sequence* );

    uiMenuHandler*		menu_				{nullptr};
    MenuItem			movemnuitem_;
    MenuItem			movetotopmnuitem_;
    MenuItem			movetobottommnuitem_;
    MenuItem			moveupmnuitem_;
    MenuItem			movedownmnuitem_;

    MenuItem			displaymnuitem_;
    MenuItem			removemnuitem_;
    MenuItem			changetransparencyitem_;
    MenuItem			statisticsitem_;
    MenuItem			amplspectrumitem_;
    MenuItem			fkspectrumitem_;
    //MenuItem			view2dwvaitem_;
    MenuItem			view2dvditem_;
    const char*			parenttype_;

    uiAmplSpectrum*		ampspectrumwin_			{nullptr};
    uiFKSpectrum*		fkspectrumwin_			{nullptr};
};
