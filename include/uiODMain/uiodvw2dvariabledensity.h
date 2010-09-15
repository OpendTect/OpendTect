#ifndef uiodvw2dvariabledensity_h
#define uiodvw2dvariabledensity_h

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Umesh Sinha
 Date:		June 2010
 RCS:		$Id: uiodvw2dvariabledensity.h,v 1.3 2010-09-15 10:14:00 cvsumesh Exp $
________________________________________________________________________

-*/

#include "uiodvw2dtreeitem.h"

#include "datapack.h"

class VW2DSeis;
namespace ColTab { class Sequence; };


mClass uiODVW2DVariableDensityTreeItem : public uiODVw2DTreeItem
{
public:
    				uiODVW2DVariableDensityTreeItem();
				~uiODVW2DVariableDensityTreeItem();

    bool                	select();

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
