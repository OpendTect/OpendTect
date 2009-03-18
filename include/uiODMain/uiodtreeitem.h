#ifndef uiodtreeitem_h
#define uiodtreeitem_h

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Kristofer Tingdahl
 Date:		4-11-2002
 RCS:		$Id: uiodtreeitem.h,v 1.30 2009-03-18 09:27:09 cvsranojay Exp $
________________________________________________________________________


-*/

#include "uitreeitemmanager.h"
#include "menuhandler.h"

class uiListView;
class uiODApplMgr;
class uiPopupMenu;
class uiSoViewer;


mClass uiODTreeItem : public uiTreeItem
{
public:
    			uiODTreeItem(const char*);
    bool		anyButtonClick(uiListViewItem*);

    int			sceneID() const;
protected:

    uiODApplMgr*	applMgr();
    uiSoViewer*		viewer();

    void		addStandardItems(uiPopupMenu&);
    void		handleStandardItems(int mnuid);
};


mClass uiODTreeTop : public uiTreeTopItem
{
public:
			uiODTreeTop(uiSoViewer*,uiListView*,
				    uiODApplMgr*,uiTreeFactorySet*);
			~uiODTreeTop();

    static const char*	sceneidkey();
    static const char*	viewerptr;
    static const char*	applmgrstr;
    static const char*	scenestr;

    int			sceneID() const;
    bool		select(int selkey);
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



mClass uiODTreeItemFactory : public uiTreeItemFactory
{
public:

    virtual uiTreeItem*	create(int visid,uiTreeItem*) const { return 0; }
    virtual uiTreeItem*	create(const MultiID&,uiTreeItem*) const { return 0; }

};


#define mShowMenu		bool showSubMenu();
#define mMenuOnAnyButton	bool anyButtonClick(uiListViewItem* lv) \
{ \
    if ( lv==uilistviewitem_ ) { showSubMenu(); return true; } \
    return inheritedClass::anyButtonClick( lv ); \
}
    

#define mDefineItem( type, inherited, parentitem, extrapublic ) \
mClass uiOD##type##TreeItem : public uiOD##inherited \
{ \
    typedef uiOD##inherited inheritedClass; \
public: \
    			uiOD##type##TreeItem(); \
    extrapublic;	\
protected: \
    const char*		parentType() const { return typeid(uiOD##parentitem).name();}\
};


#endif
