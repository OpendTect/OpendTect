#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Satyaki Maitra
 Date:		August 2016
________________________________________________________________________


-*/

#include "uitoolsmod.h"
#include "uiodtreeitem.h"
#include "dbkey.h"
#include "odpresentationmgr.h"

class SharedObject;
namespace OD { class PresentationManagedViewer; }


mExpClass(uiTools) uiODPrManagedTreeItem : public uiODTreeItem
{ mODTextTranslationClass(uiODPrManagedTreeItem)
public:
			uiODPrManagedTreeItem(const uiString&);
    virtual		~uiODPrManagedTreeItem();

    virtual OD::ObjPresentationInfo* getObjPRInfo() const	{ return 0; }
    const DBKey&	storedID() const		{ return storedid_; }

    void		setDataObj(SharedObject*);
    void		emitPRRequest(OD::PresentationRequestType);
    virtual void	handleItemCheck(bool triggerdispreq=true)	{}
    virtual void	prepareForShutdown();
    virtual OD::ViewerID getViewerID() const				=0;

protected:

    RefMan<SharedObject> dataobj_;
    DBKey		storedid_;

    virtual void	objChangedCB(CallBacker*)	{}

};


mExpClass(uiTools) uiODPrManagedParentTreeItem : public uiODTreeItem
{ mODTextTranslationClass(uiODPrManagedParentTreeItem)
public:
			uiODPrManagedParentTreeItem(const uiString&);
    virtual		~uiODPrManagedParentTreeItem();
    void		setPRManagedViewer(OD::PresentationManagedViewer&);

    void		getLoadedChildren(OD::ObjPresentationInfoSet&) const;
    void		showHideChildren(const OD::ObjPresentationInfo&,bool);
    void		removeChildren(const OD::ObjPresentationInfo&);
    void		addChildren(const OD::ObjPresentationInfoSet&);
    bool		selectChild(const OD::ObjPresentationInfo&);
    void		emitChildPRRequest(const OD::ObjPresentationInfo&,
					   OD::PresentationRequestType);
    virtual uiODPrManagedTreeItem*
			addChildItem(const OD::ObjPresentationInfo&)
			{ return 0; }

    virtual const char* childObjTypeKey() const			=0;
protected:
    virtual void	objAddedCB(CallBacker*);
    virtual void	objVanishedCB(CallBacker*);
    virtual void	objShownCB(CallBacker*);
    virtual void	objHiddenCB(CallBacker*);
    virtual void	objOrphanedCB(CallBacker*);

    virtual OD::ViewerID getViewerID() const			=0;

};
