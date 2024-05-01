#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uiodmainmod.h"
#include "uiodtreeitem.h"
#include "factory.h"

class uiFKSpectrum;
class uiMenuHandler;
class uiSeisAmplSpectrum;
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
    VisID			displayID() const;
    int				attribNr() const;

    static int			cPixmapWidth()		{ return 16; }
    static int			cPixmapHeight()		{ return 10; }

				mDefineFactory2ParamInClass(uiODDataTreeItem,
					const Attrib::SelSpec&,const char*,
					factory )

				/*!<Adds custom create function for create
				    function. */

    void			prepareForShutdown() override;

protected:
				uiODDataTreeItem(const char* parenttype);
				~uiODDataTreeItem();

    int				uiTreeViewItemType() const override;
    bool			init() override;

    virtual void		checkCB(CallBacker*);
    void			keyPressCB(CallBacker*);
    bool			shouldSelect(int) const override;

    virtual bool		hasTransparencyMenu() const { return true; }

    uiODApplMgr*		applMgr() const;
    uiVisPartServer*		visserv_;
    SceneID			sceneID() const;
    bool			isSelectable() const override { return true; }
    bool			isExpandable() const override { return false; }
    const char*			parentType() const override
				{ return parenttype_; }

    bool			showSubMenu() override;

    virtual void		createMenu(MenuHandler*,bool istoolbar);
    void			addToToolBarCB(CallBacker*);
    void			createMenuCB(CallBacker*);
    virtual void		handleMenuCB(CallBacker*);
    void			updateColumnText(int col) override;
    virtual uiString		createDisplayName() const		= 0;

    void			displayMiniCtab( const ColTab::Sequence* );

    uiMenuHandler*		menu_;
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
    MenuItem			view2dwvaitem_;
    MenuItem			view2dvditem_;
    const char*			parenttype_;

    uiStatsDisplayWin*		statswin_;
    uiSeisAmplSpectrum*		ampspectrumwin_;
    uiFKSpectrum*		fkspectrumwin_;
};
