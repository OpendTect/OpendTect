#ifndef uiodvw2dvariabledensity_h
#define uiodvw2dvariabledensity_h

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Umesh Sinha
 Date:		June 2010
 RCS:		$Id$
________________________________________________________________________

-*/

#include "uiodvw2dtreeitem.h"

#include "datapack.h"
#include "menuhandler.h"

class uiMenuHandler;
class VW2DSeis;
namespace ColTab { class Sequence; };


mClass uiODVW2DVariableDensityTreeItem : public uiODVw2DTreeItem
{
public:
    				uiODVW2DVariableDensityTreeItem();
				~uiODVW2DVariableDensityTreeItem();

    bool                	select();
    bool                        showSubMenu();

    static const int		cPixmapWidth()			{ return 16; }
    static const int		cPixmapHeight()			{ return 10; }

protected:

    bool			init();
    void			displayMiniCtab(const ColTab::Sequence*);
    const char*			parentType() const
				{ return typeid(uiODVw2DTreeTop).name(); }
    bool			isSelectable() const            { return true; }

    DataPack::ID		dpid_;
    VW2DSeis*			dummyview_;
    
    uiMenuHandler*		menu_;
    MenuItem			selattrmnuitem_;
   
    void			createSelMenu(MenuItem&);
    bool    			handleSelMenu(int mnuid);

    void			checkCB(CallBacker*);
    void			dataChangedCB(CallBacker*);
    void			createMenuCB(CallBacker*);
    void			handleMenuCB(CallBacker*);
};


mClass uiODVW2DVariableDensityTreeItemFactory : public uiODVw2DTreeItemFactory
{
public:
    const char*		name() const		{ return typeid(*this).name(); }
    uiTreeItem*		create() const
			{ return new uiODVW2DVariableDensityTreeItem(); }
    uiTreeItem*         createForVis(const uiODViewer2D&,int visid) const;
};


#endif
