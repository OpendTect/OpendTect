#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uiodmainmod.h"

#include "uiodattribtreeitem.h"
#include "uioddisplaytreeitem.h"
#include "emposid.h"

class DataPointSet;
class uiVisEMObject;
class uiODDataTreeItem;


mExpClass(uiODMain) uiODEarthModelSurfaceTreeItem : public uiODDisplayTreeItem
{ mODTextTranslationClass(uiODEarthModelSurfaceTreeItem)
public:

    uiVisEMObject*	visEMObject() const	{ return uivisemobj_; }
    EM::ObjectID	emObjectID() const	{ return emid_; }
    virtual VisID	reloadEMObject();	// Return new display id.

    void		setOnlyAtSectionsDisplay(bool) override;
    bool		isOnlyAtSections() const override;

    bool		askSave();

protected:
			uiODEarthModelSurfaceTreeItem(const EM::ObjectID&);
			~uiODEarthModelSurfaceTreeItem();

    void		createMenu(MenuHandler*,bool istb) override;
    void		handleMenuCB(CallBacker*) override;

    uiODDataTreeItem*	createAttribItem(const Attrib::SelSpec*) const override;
    void		addAuxDataItems();

    void		finishedEditingCB(CallBacker*);

    bool		doSave();
    void		askSaveCB(CallBacker*);
    void		saveCB(CallBacker*);

    EM::ObjectID	emid_;
    uiVisEMObject*	uivisemobj_	= nullptr;

    MenuItem		createflatscenemnuitem_;

protected:
    bool		init() override;
    virtual void	initNotify() {}
    bool		createUiVisObj();

    void		checkCB(CallBacker*) override;
    void		selChg(CallBacker*);
    void		keyPressCB(CallBacker*) override;

    void		updateTrackingState();
    bool		istrackingallowed_	= true;

    BufferString	timelastmodified_;
    MenuItem		savemnuitem_;
    MenuItem		saveasmnuitem_;
    MenuItem		enabletrackingmnuitem_;
    MenuItem		changesetupmnuitem_;
    MenuItem		reloadmnuitem_;
    MenuItem		trackmenuitem_;
    MenuItem		starttrackmnuitem_;

    mDeprecated("Use askSave")
    bool		isHorReady(EM::ObjectID&)	{ return askSave(); }
};


mExpClass(uiODMain) uiODEarthModelSurfaceDataTreeItem
						: public uiODAttribTreeItem
{ mODTextTranslationClass(uiODEarthModelSurfaceDataTreeItem)
public:
			uiODEarthModelSurfaceDataTreeItem(const EM::ObjectID&,
				       uiVisEMObject*,const char* parenttype);

    void		setDataPointSet(const DataPointSet&);
    void		selectAndLoadAuxData();

protected:
			~uiODEarthModelSurfaceDataTreeItem();

    void		createMenu(MenuHandler*,bool istb) override;
    void		handleMenuCB(CallBacker*) override;
    uiString		createDisplayName() const override;

    MenuItem		depthattribmnuitem_;
    MenuItem		savesurfacedatamnuitem_;
    MenuItem		loadsurfacedatamnuitem_;
    MenuItem		algomnuitem_;
    MenuItem		fillholesmnuitem_;
    MenuItem		filtermnuitem_;
    MenuItem		horvariogrammnuitem_;
    MenuItem		attr2geommnuitm_;

    bool		changed_	= false;
    EM::ObjectID	emid_;
    uiVisEMObject*	uivisemobj_;
};
