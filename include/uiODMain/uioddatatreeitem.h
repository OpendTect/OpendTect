#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Kristofer Tingdahl
 Date:		May 2006
________________________________________________________________________


-*/

#include "uiodmainmod.h"
#include "uiodscenetreeitem.h"
#include "bufstring.h"
#include "factory.h"

class ProbeLayer;
class uiColTabSelTool;
class uiFKSpectrum;
class uiMenuHandler;
class uiSeisAmplSpectrum;
class uiVisPartServer;
class uiODApplMgr;
class uiODDataTreeItem;
namespace Attrib { class SelSpec; }
namespace ColTab { class Sequence; }


/*!Base class for a data treeitem. */

mExpClass(uiODMain) uiODDataTreeItemFactory
{
public:

    typedef uiODDataTreeItem*	(*CreateFunc)(ProbeLayer&);

    void			addCreateFunc(CreateFunc,
					      const char* probelayertype,
					      const char* probetype);
    uiODDataTreeItem*		create(ProbeLayer&);

protected:

    TypeSet< TypeSet<CreateFunc> >	createfuncsset_;
    BufferStringSet			probetypes_;
    TypeSet< BufferStringSet >		probelayertypesset_;

};


mExpClass(uiODMain) uiODDataTreeItem : public uiODSceneTreeItem
{ mODTextTranslationClass(uiODDataTreeItem)
public:
				~uiODDataTreeItem();

    virtual bool		select();
    int				displayID() const;
    int				attribNr() const;

    static int			cPixmapWidth()		{ return 16; }
    static int			cPixmapHeight()		{ return 10; }

    static uiODDataTreeItemFactory& fac();
				//TODO PrIMPL remove
				mDefineFactory2ParamInClass(uiODDataTreeItem,
					const Attrib::SelSpec&,const char*,
					factory )

				/*!<Adds custom create function for create
				    function. */

    virtual void		prepareForShutdown();
    virtual void		setProbeLayer(ProbeLayer*);
    virtual void		updateDisplay()		{}

protected:
				uiODDataTreeItem(const char* parenttype);

    int				uiTreeViewItemType() const;
    virtual bool		init();

    virtual void		checkCB(CallBacker*);
    bool			shouldSelect(int) const;

    virtual bool		hasTransparencyMenu() const { return true; }

    uiVisPartServer*		visserv_;
    bool			isSelectable() const	{ return true; }
    bool			isExpandable() const	{ return false; }
    const char*			parentType() const	{ return parenttype_; }
    bool			showSubMenu();

    virtual void		createMenu(MenuHandler*,bool istoolbar);
    void			addToToolBarCB(CallBacker*);
    void			createMenuCB(CallBacker*);
    virtual void		handleMenuCB(CallBacker*);
    void			probeLayerChangedCB(CallBacker*);
    void			probeChangedCB(CallBacker*);

    void			updateColumnText(int col);
    virtual uiString		createDisplayName() const		= 0;

    void			displayMiniCtab( const ColTab::Sequence* );

    void			colSeqChgCB(CallBacker*);
    virtual void		colSeqChg(const ColTab::Sequence&) {}

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
    BufferString		parenttype_;

    uiSeisAmplSpectrum*		ampspectrumwin_;
    uiFKSpectrum*		fkspectrumwin_;

    RefMan<ProbeLayer>		probelayer_;
    uiColTabSelTool&		coltabsel_;
};
