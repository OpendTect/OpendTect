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

class uiTreeView;
class uiODApplMgr;
class uiPopupMenu;
class ui3DViewer;


mExpClass(uiODMain) uiODTreeItem : public uiTreeItem
{
public:
    			uiODTreeItem(const char*);
    bool		anyButtonClick(uiTreeViewItem*);

    int			sceneID() const;
protected:

    uiODApplMgr*	applMgr();
    ui3DViewer*		viewer();

    void		addStandardItems(uiPopupMenu&);
    void		handleStandardItems(int mnuid);
};


mExpClass(uiODMain) uiODTreeTop : public uiTreeTopItem
{
public:
			uiODTreeTop(ui3DViewer*,uiTreeView*,
				    uiODApplMgr*,uiTreeFactorySet*);
			~uiODTreeTop();

    static const char*	sceneidkey();
    static const char*	viewerptr();
    static const char*	applmgrstr();
    static const char*	scenestr();

    int			sceneID() const;
    bool		selectWithKey(int selkey);
    TypeSet<int>	getDisplayIds(int&, bool);
    void		loopOverChildrenIds( TypeSet<int>&, int&, bool, 
	    				     const ObjectSet<uiTreeItem>& );

protected:

    void		addFactoryCB(CallBacker*);
    void		removeFactoryCB(CallBacker*);

    virtual const char*	parentType() const { return 0; } 
    uiODApplMgr*	applMgr();

    uiTreeFactorySet*	tfs;
};



mExpClass(uiODMain) uiODTreeItemFactory : public uiTreeItemFactory
{
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
    

#define mDefineItem( type, inherited, parentitem, extrapublic ) \
mExpClass(uiODMain) uiOD##type##TreeItem : public uiOD##inherited \
{ \
    typedef uiOD##inherited inheritedClass; \
public: \
    			uiOD##type##TreeItem(); \
    extrapublic;	\
protected: \
    const char*		parentType() const { return typeid(uiOD##parentitem).name();}\
};


#endif

