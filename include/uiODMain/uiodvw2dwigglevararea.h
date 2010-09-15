#ifndef uiodvw2dwigglevararea_h
#define uiodvw2dwigglevararea_h

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Umesh Sinha
 Date:		June 2010
 RCS:		$Id: uiodvw2dwigglevararea.h,v 1.2 2010-09-15 10:14:00 cvsumesh Exp $
________________________________________________________________________

-*/

#include "uiodvw2dtreeitem.h"

#include "datapack.h"

class VW2DSeis;


mClass uiODVW2DWiggleVarAreaTreeItem : public uiODVw2DTreeItem
{
public:
    				uiODVW2DWiggleVarAreaTreeItem();
				~uiODVW2DWiggleVarAreaTreeItem();

    bool			select();

protected:
	
    bool			init();
    const char*			parentType() const
				{ return typeid(uiODVw2DTreeTop).name(); }
    bool                        isSelectable() const            { return true; }
    
    DataPack::ID		dpid_;
    VW2DSeis*			dummyview_;

    void			checkCB(CallBacker*);
    void			dataChangedCB(CallBacker*);
};


mClass uiODVW2DWiggleVarAreaTreeItemFactory : public uiTreeItemFactory
{
public:
    const char*		name() const		{ return typeid(*this).name(); }
    uiTreeItem*		create() const
			{ return new uiODVW2DWiggleVarAreaTreeItem(); }
};


#endif
