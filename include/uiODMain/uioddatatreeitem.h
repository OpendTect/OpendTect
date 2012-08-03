#ifndef uioddatatreeitem_h
#define uioddatatreeitem_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Kristofer Tingdahl
 Date:		May 2006
 RCS:		$Id: uioddatatreeitem.h,v 1.26 2012-08-03 13:01:03 cvskris Exp $
________________________________________________________________________


-*/

#include "uiodmainmod.h"
#include "uiodtreeitem.h"
#include "factory.h"

class uiMenuHandler;
namespace Attrib { class SelSpec; };
namespace ColTab { class Sequence; };


/*!Base class for a data treeitem. */

mClass(uiODMain) uiODDataTreeItem : public uiTreeItem
{
public:
				uiODDataTreeItem(const char* parenttype);
				~uiODDataTreeItem();

    virtual bool		select();
    int				displayID() const;
    int				attribNr() const;

    static const int		cPixmapWidth()		{ return 16; }
    static const int		cPixmapHeight()		{ return 10; }

				mDefineFactory2ParamInClass(uiODDataTreeItem,
					const Attrib::SelSpec&,const char*,
					factory );

    				/*!<Adds custom create function for create
				    function. */

    void			prepareForShutdown();				

protected:

    int				uiListViewItemType() const;
    virtual bool		init();

    virtual void		checkCB(CallBacker*);
    bool			shouldSelect(int) const;

    virtual bool		hasTransparencyMenu() const { return true; }

    uiODApplMgr*		applMgr() const;
    int				sceneID() const;
    bool			isSelectable() const	{ return true; }
    bool			isExpandable() const	{ return false; }
    const char*			parentType() const	{ return parenttype_; }
    bool			showSubMenu();

    virtual void		createMenu(MenuHandler*,bool istoolbar);
    void			addToToolBarCB(CallBacker*);
    void			createMenuCB(CallBacker*);
    virtual void		handleMenuCB(CallBacker*);
    void			updateColumnText(int col);
    virtual BufferString	createDisplayName() const		= 0;

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
    MenuItem                    statisticsitem_;
    MenuItem			amplspectrumitem_;
    MenuItem			fkspectrumitem_;
    MenuItem			view2dwvaitem_;
    MenuItem			view2dvditem_;
    const char*			parenttype_;
};

#endif

