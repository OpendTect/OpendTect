#ifndef uiodvw2dwigglevararea_h
#define uiodvw2dwigglevararea_h

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Umesh Sinha
 Date:		June 2010
 RCS:		$Id: uiodvw2dwigglevararea.h,v 1.3 2010-09-27 09:26:30 cvsumesh Exp $
________________________________________________________________________

-*/

#include "uiodvw2dtreeitem.h"

#include "datapack.h"
#include "menuhandler.h"

class uiMenuHandler;
class VW2DSeis;


mClass uiODVW2DWiggleVarAreaTreeItem : public uiODVw2DTreeItem
{
public:
    				uiODVW2DWiggleVarAreaTreeItem();
				~uiODVW2DWiggleVarAreaTreeItem();

    bool			select();
    bool                        showSubMenu();

protected:
	
    bool			init();
    const char*			parentType() const
				{ return typeid(uiODVw2DTreeTop).name(); }
    bool                        isSelectable() const            { return true; }
    
    DataPack::ID		dpid_;
    VW2DSeis*			dummyview_;

    uiMenuHandler*              menu_;
    MenuItem                    selattrmnuitem_;

    void                        createSelMenu(MenuItem&);
    bool			handleSelMenu(int mnuid);

    void			checkCB(CallBacker*);
    void			dataChangedCB(CallBacker*);
    void                        createMenuCB(CallBacker*);
    void                        handleMenuCB(CallBacker*);
};


mClass uiODVW2DWiggleVarAreaTreeItemFactory : public uiTreeItemFactory
{
public:
    const char*		name() const		{ return typeid(*this).name(); }
    uiTreeItem*		create() const
			{ return new uiODVW2DWiggleVarAreaTreeItem(); }
};


#endif
