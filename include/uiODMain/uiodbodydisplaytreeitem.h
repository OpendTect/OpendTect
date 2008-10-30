#ifndef uiodbodydisplaytreeitem_h
#define uiodbodydisplaytreeitem_h

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Kristofer Tingdahl
 Date:		May 2006
 RCS:		$Id: uiodbodydisplaytreeitem.h,v 1.2 2008-10-30 19:05:52 cvsyuancheng Exp $
________________________________________________________________________


-*/

#include "uioddisplaytreeitem.h"

#include "emposid.h"


namespace visSurvey { class MarchingCubesDisplay; class PolygonSurfDisplay; }


mDefineItem( BodyDisplayParent, TreeItem, TreeTop, mShowMenu );


class uiODBodyDisplayTreeItemFactory : public uiODTreeItemFactory
{
public:
    const char*		name() const { return typeid(*this).name(); }
    uiTreeItem*		create() const
    			{ return new uiODBodyDisplayParentTreeItem; }
    uiTreeItem*		create(int visid,uiTreeItem*) const;
};


class uiODBodyDisplayTreeItem : public uiODDisplayTreeItem
{
public:
    			uiODBodyDisplayTreeItem( int, bool dummy );
    			uiODBodyDisplayTreeItem( const EM::ObjectID& emid );
    			~uiODBodyDisplayTreeItem();

protected:
    void		prepareForShutdown();
    void		createMenuCB(CallBacker*);
    void		handleMenuCB(CallBacker*);
    void		colorChCB(CallBacker*);

    bool		init();
    const char*		parentType() const
			{return typeid(uiODBodyDisplayParentTreeItem).name();}

    EM::ObjectID			emid_;
    MenuItem				savemnuitem_;
    MenuItem				saveasmnuitem_;
    MenuItem				newellipsoidmnuitem_;
    MenuItem				displaybodymnuitem_;
    MenuItem				displaypolygonmnuitem_;
    MenuItem				displayintersectionmnuitem_;
    MenuItem				removeselectedmnuitem_;
    MenuItem				displaymnuitem_;
    visSurvey::MarchingCubesDisplay*	mcd_;
    visSurvey::PolygonSurfDisplay*	plg_;
};


#endif
