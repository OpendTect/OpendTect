#ifndef uiodtreeitem_h
#define uiodtreeitem_h

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Kristofer Tingdahl
 Date:		4-11-2002
 RCS:		$Id: uiodtreeitem.h,v 1.1 2003-12-20 13:24:05 bert Exp $
________________________________________________________________________


-*/

#include "uitreeitemmanager.h"
#include "uiodscenemgr.h"

class uiPopUpMenu;
class uiListViewItem;
class uiParent;
class uiODApplMgr;
class uiSoViewer;

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
			uiODTreeTop(uiODSceneMgr::Scene*,uiODApplMgr*,
				    uiTreeFactorySet*);
			~uiODTreeTop();

    static const char*	sceneidkey;
    static const char*	viewerptr;
    static const char*	appmgrstr;
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


#endif

