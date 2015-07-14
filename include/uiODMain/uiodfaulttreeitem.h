#ifndef uiodfaulttreeitem_h
#define uiodfaulttreeitem_h

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

class DataPointSet;


namespace visSurvey { class FaultDisplay; class FaultStickSetDisplay; }


mExpClass(uiODMain) uiODFaultParentTreeItem : public uiODTreeItem
{ mODTextTranslationClass(uiODFaultParentTreeItem)
    typedef uiODTreeItem inheritedClass;
public:
			uiODFaultParentTreeItem();
			~uiODFaultParentTreeItem();

protected:
			mMenuOnAnyButton
    bool		showSubMenu();
    const char* 	parentType() const
			{ return typeid(uiODTreeTop).name(); }

    void		keyPressCB(CallBacker*);
};


mExpClass(uiODMain) uiODFaultTreeItemFactory : public uiODTreeItemFactory
{ mODTextTranslationClass(uiODFaultTreeItemFactory)
public:
    const char*		name() const { return typeid(*this).name(); }
    uiTreeItem*		create() const
			{ return new uiODFaultParentTreeItem; }
    uiTreeItem*		createForVis(int visid,uiTreeItem*) const;
};


mExpClass(uiODMain) uiODFaultTreeItem : public uiODDisplayTreeItem
{ mODTextTranslationClass(uiODFaultTreeItem)
public:
			uiODFaultTreeItem(int,bool dummy);
			uiODFaultTreeItem(const EM::ObjectID&);
			~uiODFaultTreeItem();

    EM::ObjectID	emObjectID() const	{ return emid_; }

    void		setOnlyAtSectionsDisplay(bool);
    bool		isOnlyAtSections() const;

protected:
    bool		askContinueAndSaveIfNeeded(bool withcancel);
    void		prepareForShutdown();
    virtual void	createMenu(MenuHandler*,bool istb);
    void		handleMenuCB(CallBacker*);
    void		colorChCB(CallBacker*);

    uiODDataTreeItem*	createAttribItem(const Attrib::SelSpec*) const;

    bool		init();
    const char*		parentType() const
			{return typeid(uiODFaultParentTreeItem).name();}

    EM::ObjectID		emid_;

    MenuItem			savemnuitem_;
    MenuItem			saveasmnuitem_;
    MenuItem			displayplanemnuitem_;
    MenuItem			displaystickmnuitem_;
    MenuItem			displayintersectionmnuitem_;
    MenuItem			displayintersecthorizonmnuitem_;
    MenuItem			singlecolmnuitem_;
    visSurvey::FaultDisplay*	faultdisplay_;
};


mDefineItem( FaultStickSetParent, TreeItem, TreeTop,mShowMenu mMenuOnAnyButton);


mExpClass(uiODMain) uiODFaultStickSetTreeItemFactory
    : public uiODTreeItemFactory
{ mODTextTranslationClass(uiODFaultStickSetTreeItemFactory)
public:
    const char*		name() const { return typeid(*this).name(); }
    uiTreeItem*		create() const
			{ return new uiODFaultStickSetParentTreeItem; }
    uiTreeItem*		createForVis(int visid,uiTreeItem*) const;
};


mExpClass(uiODMain) uiODFaultStickSetTreeItem : public uiODDisplayTreeItem
{ mODTextTranslationClass(uiODFaultStickSetTreeItem)
public:
			uiODFaultStickSetTreeItem(int,bool dummy);
			uiODFaultStickSetTreeItem(const EM::ObjectID&);
			~uiODFaultStickSetTreeItem();

    EM::ObjectID	emObjectID() const	{ return emid_; }

protected:
    bool		askContinueAndSaveIfNeeded( bool withcancel );
    void		prepareForShutdown();
    void		createMenu(MenuHandler*,bool istb);
    void		handleMenuCB(CallBacker*);
    void		colorChCB(CallBacker*);

    bool		init();
    const char*		parentType() const
			{return typeid(uiODFaultStickSetParentTreeItem).name();}


    EM::ObjectID			emid_;
    MenuItem				onlyatsectmnuitem_;
    MenuItem				savemnuitem_;
    MenuItem				saveasmnuitem_;
    visSurvey::FaultStickSetDisplay*	faultsticksetdisplay_;
};


mExpClass(uiODMain) uiODFaultSurfaceDataTreeItem : public uiODAttribTreeItem
{ mODTextTranslationClass(uiODFaultSurfaceDataTreeItem)
public:
			uiODFaultSurfaceDataTreeItem(EM::ObjectID,
				const char* parenttype);

    void		setDataPointSet(const DataPointSet&);

protected:

    void		createMenu(MenuHandler*,bool istb);
    void		handleMenuCB(CallBacker*);
    BufferString	createDisplayName() const;

    MenuItem		depthattribmnuitem_;
    MenuItem		savesurfacedatamnuitem_;
    MenuItem		loadsurfacedatamnuitem_;
    MenuItem		algomnuitem_;

    bool		changed_;
    EM::ObjectID	emid_;
};

#endif

