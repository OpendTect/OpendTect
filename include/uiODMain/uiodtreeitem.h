#ifndef uiodtreeitem_h
#define uiodtreeitem_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Kristofer Tingdahl
 Date:		4-11-2002
 RCS:		$Id$
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
{ mODTextTranslationClass(uiODTreeItem);
public:
			uiODTreeItem(const uiString&);
    bool		anyButtonClick(uiTreeViewItem*);

    int			sceneID() const;
protected:

    uiODApplMgr*	applMgr();
    ui3DViewer*		viewer();

    virtual bool	init();
    virtual const char*	iconName() const		{ return 0; }

    void		addStandardItems(uiMenu&);
    void		handleStandardItems(int mnuid);
};


mExpClass(uiODMain) uiODTreeTop : public uiTreeTopItem
{ mODTextTranslationClass(uiODTreeTop);
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



mExpClass(uiODMain) uiODTreeItemFactory : public uiTreeItemFactory
{ mODTextTranslationClass(uiODTreeItemFactory);
public:

    virtual uiTreeItem*	createForVis(int visid,uiTreeItem*) const
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
    virtual const char* iconName() const; \
    const char*     	parentType() const \
			{ return typeid(uiOD##parentitem).name();} \
public: \
                        uiOD##type##TreeItem()

#define mDefineItem( type, inherited, parentitem, extrapublic ) \
mExpClass(uiODMain) uiOD##type##TreeItem : public uiOD##inherited \
{ mODTextTranslationClass(uiOD##type##TreeItem)\
    mDefineItemMembers( type, inherited, parentitem ); \
    extrapublic;	\
};




#endif
