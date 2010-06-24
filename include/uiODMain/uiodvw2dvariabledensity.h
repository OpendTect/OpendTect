#ifndef uiodvw2dvariabledensity_h
#define uiodvw2dvariabledensity_h

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Umesh Sinha
 Date:		June 2010
 RCS:		$Id: uiodvw2dvariabledensity.h,v 1.1 2010-06-24 08:54:11 cvsumesh Exp $
________________________________________________________________________

-*/

#include "uiodvw2dtreeitem.h"

#include "datapack.h"

class VW2DSeis;


mClass uiODVW2DVariableDensityTreeItem : public uiODVw2DTreeItem
{
public:
    				uiODVW2DVariableDensityTreeItem();
				~uiODVW2DVariableDensityTreeItem();

    bool                	select();

protected:

    bool			init();
    const char*			parentType() const
				{ return typeid(uiODVw2DTreeTop).name(); }
    bool			isSelectable() const            { return true; }

    DataPack::ID		dpid_;
    bool			viachkbox_;
    VW2DSeis*			dummyview_;
    
    void			checkCB(CallBacker*);
    void			dataChangedCB(CallBacker*);
};


mClass uiODVW2DVariableDensityTreeItemFactory : public uiTreeItemFactory
{
public:
    const char*		name() const		{ return typeid(*this).name(); }
    uiTreeItem*		create() const
			{ return new uiODVW2DVariableDensityTreeItem(); }
};


#endif
