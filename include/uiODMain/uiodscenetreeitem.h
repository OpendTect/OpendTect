#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Kristofer Tingdahl
 Date:		4-11-2002
________________________________________________________________________


-*/

#include "uiodmainmod.h"
#include "uiodprmantreeitem.h"

class uiTreeView;
class uiODApplMgr;
class uiMenu;
class ui3DViewer;


mExpClass(uiODMain) uiODSceneTreeItem : public uiODPrManagedTreeItem
{ mODTextTranslationClass(uiODSceneTreeItem);
public:
			uiODSceneTreeItem(const uiString&);
    bool		anyButtonClick(uiTreeViewItem*);

    int			sceneID() const;
    void		prepareForShutdown();
protected:

    uiODApplMgr*	applMgr();
    ui3DViewer*		viewer();
    virtual OD::ViewerID getViewerID() const;

    void		removeAllItems();

    void		setMoreObjectsToDoHint(bool yn);
    bool		getMoreObjectsToDoHint() const;
};


mExpClass(uiODMain) uiODSceneTreeTop : public uiTreeTopItem
{ mODTextTranslationClass(uiODSceneTreeTop);
public:
			uiODSceneTreeTop(ui3DViewer*,uiTreeView*,
				    uiODApplMgr*,uiTreeFactorySet*);
			~uiODSceneTreeTop();

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

    virtual const char* parentType() const { return 0; }
    uiODApplMgr*	applMgr();

    uiTreeFactorySet*	tfs;
};



mExpClass(uiODMain) uiODSceneTreeItemFactory : public uiTreeItemFactory
{ mODTextTranslationClass(uiODSceneTreeItemFactory);
public:

    virtual uiTreeItem* createForVis(int visid,uiTreeItem*) const
				{ return 0; }

};


#define mShowMenu		bool showSubMenu();
#define mMenuOnAnyButton	bool anyButtonClick(uiTreeViewItem* lv) \
{ \
    if ( lv==uitreeviewitem_ ) { select(); showSubMenu(); return true; } \
    return inheritedClass::anyButtonClick( lv ); \
}

#define mDefineItemMembers( type, inherited, parentitem ) \
    typedef uiOD##inherited inheritedClass; \
protected: \
virtual const char*	iconName() const; \
const char*	parentType() const \
{ return typeid(uiOD##parentitem).name();} \
public: \
			uiOD##type##TreeItem()


