#ifndef uiodbodydisplaytreeitem_h
#define uiodbodydisplaytreeitem_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Kristofer Tingdahl
 Date:		May 2006
 RCS:		$Id$
________________________________________________________________________


-*/

#include "uioddisplaytreeitem.h"

#include "emposid.h"


namespace visSurvey { class MarchingCubesDisplay; class PolygonBodyDisplay; 
		      class RandomPosBodyDisplay; }


mDefineItem( BodyDisplayParent, TreeItem, TreeTop, mShowMenu mMenuOnAnyButton );


mClass uiODBodyDisplayTreeItemFactory : public uiODTreeItemFactory
{
public:
    const char*		name() const { return typeid(*this).name(); }
    uiTreeItem*		create() const
    			{ return new uiODBodyDisplayParentTreeItem; }
    virtual uiTreeItem*	createForVis(int visid,uiTreeItem*) const;
};


mClass uiODBodyDisplayTreeItem : public uiODDisplayTreeItem
{
public:
    			uiODBodyDisplayTreeItem(int,bool dummy);
    			uiODBodyDisplayTreeItem(const EM::ObjectID&);
    			~uiODBodyDisplayTreeItem();

    EM::ObjectID	emObjectID() const	{ return emid_; }

protected:
    void		prepareForShutdown();
    bool		askContinueAndSaveIfNeeded(bool withcancel);
    void		createMenuCB(CallBacker*);
    void		handleMenuCB(CallBacker*);
    void		colorChCB(CallBacker*);

    bool		init();
    const char*		parentType() const
			{return typeid(uiODBodyDisplayParentTreeItem).name();}

    EM::ObjectID			emid_;
    MenuItem				savemnuitem_;
    MenuItem				saveasmnuitem_;
    MenuItem				displaybodymnuitem_;
    MenuItem				displaypolygonmnuitem_;
    MenuItem				displayintersectionmnuitem_;
    MenuItem				singlecolormnuitem_;
    visSurvey::MarchingCubesDisplay*	mcd_;
    visSurvey::PolygonBodyDisplay*	plg_;
    visSurvey::RandomPosBodyDisplay*	rpb_;
};


#endif
