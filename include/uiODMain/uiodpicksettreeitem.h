#ifndef uiodpicksettreeitem_h
#define uiodpicksettreeitem_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Kristofer Tingdahl
 Date:		May 2006
 RCS:		$Id$
________________________________________________________________________


-*/

#include "uiodmainmod.h"
#include "uioddisplaytreeitem.h"

namespace Pick		{ class Set; }


mDefineItem( PickSetParent, TreeItem, TreeTop, \
    ~uiODPickSetParentTreeItem(); \
    void addPickSet(Pick::Set*);\
    mShowMenu mMenuOnAnyButton )


mExpClass(uiODMain) uiODPickSetTreeItemFactory : public uiODTreeItemFactory
{ mODTextTranslationClass(uiODPickSetTreeItemFactory)
public:

    const char*		name() const { return typeid(*this).name(); }
    uiTreeItem*		create() const { return new uiODPickSetParentTreeItem; }
    uiTreeItem*		createForVis(int visid,uiTreeItem*) const;

};


mExpClass(uiODMain) uiODPickSetTreeItem : public uiODDisplayTreeItem
{ mODTextTranslationClass(uiODPickSetTreeItem)
public:
    			uiODPickSetTreeItem(int dispid,Pick::Set&);
    			~uiODPickSetTreeItem();
    virtual bool	actModeWhenSelected() const	{ return true; }
    Pick::Set&		getSet()			{ return set_; }
    const Pick::Set&	getSet() const			{ return set_; }

protected:

    bool		init();
    void		prepareForShutdown();
    bool		askContinueAndSaveIfNeeded(bool withcancel);
    void		setChg(CallBacker*);
    virtual void	createMenu(MenuHandler*,bool istb);
    void		handleMenuCB(CallBacker*);
    const char*		parentType() const
    			{ return typeid(uiODPickSetParentTreeItem).name(); }

    Pick::Set&		set_;

    MenuItem		storemnuitem_;
    MenuItem		storeasmnuitem_;
    MenuItem		dirmnuitem_;
    MenuItem		onlyatsectmnuitem_;
    MenuItem		convertbodymnuitem_;
    MenuItem		propertymnuitem_;
};


mDefineItem( PolygonParent, TreeItem, TreeTop, \
    ~uiODPolygonParentTreeItem(); \
    void addPolygon(Pick::Set*);\
    mShowMenu mMenuOnAnyButton )


mExpClass(uiODMain) uiODPolygonTreeItemFactory : public uiODTreeItemFactory
{ mODTextTranslationClass(uiODPolygonTreeItemFactory)
public:

    const char*		name() const { return typeid(*this).name(); }
    uiTreeItem*		create() const { return new uiODPolygonParentTreeItem; }
    uiTreeItem*		createForVis(int visid,uiTreeItem*) const;

};


mExpClass(uiODMain) uiODPolygonTreeItem : public uiODDisplayTreeItem
{ mODTextTranslationClass(uiODPolygonTreeItem)
public:
			uiODPolygonTreeItem(int dispid,Pick::Set&);
			~uiODPolygonTreeItem();
    virtual bool	actModeWhenSelected() const	{ return true; }
    Pick::Set&		getSet()			{ return set_; }
    const Pick::Set&	getSet() const			{ return set_; }

protected:

    bool		init();
    void		prepareForShutdown();
    bool		askContinueAndSaveIfNeeded(bool withcancel);
    void		setChg(CallBacker*);
    virtual void	createMenu(MenuHandler*,bool istb);
    void		handleMenuCB(CallBacker*);
    const char*		parentType() const
			{ return typeid(uiODPolygonParentTreeItem).name(); }

    Pick::Set&		set_;

    MenuItem		storemnuitem_;
    MenuItem		storeasmnuitem_;
    MenuItem		onlyatsectmnuitem_;
    MenuItem		propertymnuitem_;
    MenuItem		closepolyitem_;
};


#endif


