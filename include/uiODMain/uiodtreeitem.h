#ifndef uiodtreeitem_h
#define uiodtreeitem_h

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Kristofer Tingdahl
 Date:		4-11-2002
 RCS:		$Id: uiodtreeitem.h,v 1.8 2005-03-07 10:58:26 cvskris Exp $
________________________________________________________________________


-*/

#include "uitreeitemmanager.h"
class uiParent;
class uiSoViewer;
class uiListView;
class uiPopUpMenu;
class uiODApplMgr;
class uiListViewItem;

class uiODTreeItem : public uiTreeItem
{
public:
    			uiODTreeItem(const char*);

protected:

    uiODApplMgr*	applMgr();
    uiSoViewer*		viewer();
    int			sceneID() const;

};


class uiODTreeTop : public uiTreeTopItem
{
public:
			uiODTreeTop(uiSoViewer*,uiListView*,
				    uiODApplMgr*,uiTreeFactorySet*);
			~uiODTreeTop();

    static const char*	sceneidkey;
    static const char*	viewerptr;
    static const char*	applmgrstr;
    static const char*	scenestr;

    int			sceneID() const;
    bool		select(int selkey);

protected:

    void		addFactoryCB(CallBacker*);
    void		removeFactoryCB(CallBacker*);

    virtual const char*	parentType() const { return 0; } 
    uiODApplMgr*	applMgr();

    uiTreeFactorySet*	tfs;
};



class uiODTreeItemFactory : public uiTreeItemFactory
{
public:

    virtual uiTreeItem*	create(int visid,uiTreeItem*) const { return 0; }
    virtual uiTreeItem*	create(const MultiID&,uiTreeItem*) const { return 0; }

};


#endif

