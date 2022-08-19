#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uiodmainmod.h"
#include "uitreeitemmanager.h"
#include "menuhandler.h"
#include "uistring.h"

class uiTreeView;
class uiODApplMgr;
class uiMenu;
class ui3DViewer;


mExpClass(uiODMain) uiODTreeItem : public uiTreeItem
{
mODTextTranslationClass(uiODTreeItem)
public:
    virtual		~uiODTreeItem();
    bool		anyButtonClick(uiTreeViewItem*);

    SceneID		sceneID() const;

protected:
			uiODTreeItem(const uiString&);

    uiODApplMgr*	applMgr();
    ui3DViewer*		viewer();

    virtual bool	init();
    virtual const char*	iconName() const		{ return 0; }

    void		addStandardItems(uiMenu&);
    void		handleStandardItems(int mnuid);

    void		setMoreObjectsToDoHint(bool yn);
    bool		getMoreObjectsToDoHint() const;
};


mExpClass(uiODMain) uiODTreeTop : public uiTreeTopItem
{
mODTextTranslationClass(uiODTreeTop)
public:
			uiODTreeTop(ui3DViewer*,uiTreeView*,
				    uiODApplMgr*,uiTreeFactorySet*);
			~uiODTreeTop();

    static const char*	sceneidkey();
    static const char*	viewerptr();
    static const char*	applmgrstr();

    SceneID		sceneID() const;
    bool		selectWithKey(int selkey);
    TypeSet<VisID>	getDisplayIds(VisID&, bool);
    void		loopOverChildrenIds(TypeSet<VisID>&,VisID&,bool,
	    				    const ObjectSet<uiTreeItem>&);

protected:

    void		addFactoryCB(CallBacker*);
    void		removeFactoryCB(CallBacker*);

    virtual const char*	parentType() const { return 0; }
    uiODApplMgr*	applMgr();

    uiTreeFactorySet*	tfs;
};


mExpClass(uiODMain) uiODParentTreeItem : public uiODTreeItem
{
mODTextTranslationClass(uiODParentTreeItem)
public:
    virtual		~uiODParentTreeItem();

    virtual bool	showSubMenu() = 0;
    bool		anyButtonClick(uiTreeViewItem*);
    void		show(bool yn);

protected:
			uiODParentTreeItem(const uiString&);
    virtual bool	init();

    virtual const char* iconName() const = 0;
    const char*		parentType() const;
    virtual int		uiTreeViewItemType() const;
    virtual void	checkCB(CallBacker*);
};


mExpClass(uiODMain) uiODTreeItemFactory : public uiTreeItemFactory
{
mODTextTranslationClass(uiODTreeItemFactory)
public:

    virtual uiTreeItem* createForVis(VisID visid,uiTreeItem*) const
				{ return 0; }

};
