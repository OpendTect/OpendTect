#ifndef uiodemsurftreeitem_h
#define uiodemsurftreeitem_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Kristofer Tingdahl
 Date:		May 2006
 RCS:		$Id: uiodemsurftreeitem.h,v 1.18 2012/06/27 15:23:39 cvsjaap Exp $
________________________________________________________________________


-*/

#include "uiodattribtreeitem.h"
#include "uioddisplaytreeitem.h"
#include "emposid.h"

class DataPointSet;
class uiVisEMObject;
class uiODDataTreeItem;


mClass uiODEarthModelSurfaceTreeItem : public uiODDisplayTreeItem
{
public:

    uiVisEMObject*	visEMObject() const	{ return uivisemobj_; }
    EM::ObjectID	emObjectID() const	{ return emid_; }

protected:
    			uiODEarthModelSurfaceTreeItem(const EM::ObjectID&);
    			~uiODEarthModelSurfaceTreeItem();
    void		createMenuCB(CallBacker*);
    void		handleMenuCB(CallBacker*);

    uiODDataTreeItem*	createAttribItem(const Attrib::SelSpec*) const;

    void		finishedEditingCB(CallBacker*);
    void		prepareForShutdown();

    void		askSaveCB(CallBacker*);
    void		saveCB(CallBacker*);

    EM::ObjectID	emid_;
    uiVisEMObject*	uivisemobj_;

    MenuItem		createflatscenemnuitem_;

protected:
    bool		init();
    virtual void	initNotify() {};
    bool		createUiVisObj();

    virtual void	checkCB(CallBacker*);
    void		selChg(CallBacker*);

    bool		istrackingallowed_;
    bool		prevtrackstatus_;	// obsolete

    MenuItem		savemnuitem_;
    MenuItem		saveasmnuitem_;
    MenuItem		enabletrackingmnuitem_;
    MenuItem		changesetupmnuitem_;
    MenuItem		reloadmnuitem_;
    MenuItem		trackmenuitem_;
    MenuItem		starttrackmnuitem_;

    void		updateTrackingState();
};


mClass uiODEarthModelSurfaceDataTreeItem : public uiODAttribTreeItem
{
public:
    			uiODEarthModelSurfaceDataTreeItem(EM::ObjectID,
				       uiVisEMObject*,const char* parenttype);

    void		setDataPointSet(const DataPointSet&);

protected:
    void		createMenu(MenuHandler*,bool istb);
    void		handleMenuCB(CallBacker*);
    BufferString	createDisplayName() const;

    MenuItem		depthattribmnuitem_;
    MenuItem		savesurfacedatamnuitem_;
    MenuItem		loadsurfacedatamnuitem_;
    MenuItem		algomnuitem_;
    MenuItem		fillholesmnuitem_;
    MenuItem		filtermnuitem_;
    MenuItem		horvariogrammnuitem_;
    MenuItem		attr2geommnuitm_;

    bool		changed_;
    EM::ObjectID	emid_;
    uiVisEMObject*	uivisemobj_;
};


#endif
