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

#include "uiodmainmod.h"
#include "uiodattribtreeitem.h"
#include "uioddisplaytreeitem.h"

#include "emposid.h"


namespace visSurvey { class MarchingCubesDisplay; class PolygonBodyDisplay; 
		      class RandomPosBodyDisplay; }


mDefineItem( BodyDisplayParent, TreeItem, TreeTop,
	     mShowMenu mMenuOnAnyButton void loadBodies(); );


mExpClass(uiODMain) uiODBodyDisplayTreeItemFactory : public uiODTreeItemFactory
{
public:
    const char*		name() const { return typeid(*this).name(); }
    uiTreeItem*		create() const
    			{ return new uiODBodyDisplayParentTreeItem; }
    virtual uiTreeItem*	createForVis(int visid,uiTreeItem*) const;
};


mExpClass(uiODMain) uiODBodyDisplayTreeItem : public uiODDisplayTreeItem
{
public:
    			uiODBodyDisplayTreeItem(int,bool dummy);
    			uiODBodyDisplayTreeItem(const EM::ObjectID&);
    			~uiODBodyDisplayTreeItem();

    EM::ObjectID	emObjectID() const	{ return emid_; }

protected:
    void		prepareForShutdown();
    bool		askContinueAndSaveIfNeeded(bool withcancel);
    virtual void	createMenu(MenuHandler*,bool istb);
    void		handleMenuCB(CallBacker*);
    void		colorChCB(CallBacker*);
    uiODDataTreeItem*	createAttribItem(const Attrib::SelSpec*) const;
    bool		createUiVisObj();

    bool		init();
    const char*		parentType() const
			{return typeid(uiODBodyDisplayParentTreeItem).name();}

    EM::ObjectID			emid_;
    visSurvey::MarchingCubesDisplay*	mcd_;
    visSurvey::PolygonBodyDisplay*	plg_;
    visSurvey::RandomPosBodyDisplay*	rpb_;
    
    MenuItem				savemnuitem_;
    MenuItem				saveasmnuitem_;
    MenuItem				displaybodymnuitem_;
    MenuItem				displaypolygonmnuitem_;
    MenuItem				displayintersectionmnuitem_;
    MenuItem				singlecolormnuitem_;
    MenuItem				volcalmnuitem_;
};


mExpClass(uiODMain) uiODBodyDisplayDataTreeItem : public uiODAttribTreeItem
{
public:
    			uiODBodyDisplayDataTreeItem(const char* parenttype);
protected:
    void		createMenu(MenuHandler*,bool istb);
    void		handleMenuCB(CallBacker*);
    BufferString	createDisplayName() const;
    
    MenuItem		depthattribmnuitem_;
    MenuItem		isopachmnuitem_;
};



#endif

