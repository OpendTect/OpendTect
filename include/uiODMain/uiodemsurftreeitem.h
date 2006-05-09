#ifndef uiodemsurftreeitem_h
#define uiodemsurftreeitem_h

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Kristofer Tingdahl
 Date:		May 2006
 RCS:		$Id: uiodemsurftreeitem.h,v 1.1 2006-05-09 11:00:53 cvsbert Exp $
________________________________________________________________________


-*/

#include "uiodattribtreeitem.h"
#include "uioddisplaytreeitem.h"
#include "emposid.h"
class uiVisEMObject;
class uiODDataTreeItem;


class uiODEarthModelSurfaceTreeItem : public uiODDisplayTreeItem
{
public:

    uiVisEMObject*	visEMObject() const	{ return uivisemobj; }

protected:
    			uiODEarthModelSurfaceTreeItem(const EM::ObjectID&);
    			~uiODEarthModelSurfaceTreeItem();
    void		createMenuCB(CallBacker*);
    void		handleMenuCB(CallBacker*);

    uiODDataTreeItem*	createAttribItem(const Attrib::SelSpec*) const;

    void		finishedEditingCB(CallBacker*);
    void		prepareForShutdown();

    EM::ObjectID	emid;
    uiVisEMObject*	uivisemobj;

private:
    bool		init();
    virtual void	initNotify() {};

    virtual void	checkCB(CallBacker*);

    bool		prevtrackstatus;

    MenuItem		savemnuitem_;
    MenuItem		saveasmnuitem_;
    MenuItem		enabletrackingmnuitem_;
    MenuItem		changesetupmnuitem_;
    MenuItem		reloadmnuitem_;
    MenuItem		relationsmnuitem_;
    MenuItem		starttrackmnuitem_;
};


class uiODEarthModelSurfaceDataTreeItem : public uiODAttribTreeItem
{
public:
    			uiODEarthModelSurfaceDataTreeItem( EM::ObjectID,
				       uiVisEMObject*, const char* parenttype );
protected:
    void		createMenuCB(CallBacker*);
    void		handleMenuCB(CallBacker*);
    BufferString	createDisplayName() const;

    MenuItem		depthattribmnuitem_;
    MenuItem		savesurfacedatamnuitem_;
    MenuItem		loadsurfacedatamnuitem_;
    EM::ObjectID	emid;
    uiVisEMObject*	uivisemobj;
};


#endif
