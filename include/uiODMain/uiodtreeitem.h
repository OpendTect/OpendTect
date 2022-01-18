#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Kristofer Tingdahl
 Date:		4-11-2002
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

    int			sceneID() const;

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

    int			sceneID() const;
    bool		selectWithKey(int selkey);
    TypeSet<int>	getDisplayIds(int&, bool);
    void		loopOverChildrenIds(TypeSet<int>&,int&,bool,
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

    virtual uiTreeItem* createForVis(int visid,uiTreeItem*) const
				{ return 0; }

};
