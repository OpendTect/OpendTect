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

namespace OD { class PresentationManagedViewer; }

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

    virtual const char* childObjTypeKey() const			=0;
protected:
    virtual void	objAddedCB(CallBacker*);
    virtual void	objVanishedCB(CallBacker*);
    virtual void	objShownCB(CallBacker*);
    virtual void	objHiddenCB(CallBacker*);
    virtual void	objOrphanedCB(CallBacker*);

    virtual void	addChildItem(const OD::ObjPresentationInfo&)	{}
};


mExpClass(uiTools) uiODPrManagedTreeItem : public uiODTreeItem
{ mODTextTranslationClass(uiODPrManagedTreeItem)
public:
			uiODPrManagedTreeItem(const uiString&);
    virtual		~uiODPrManagedTreeItem();

    virtual OD::ObjPresentationInfo* getObjPRInfo() const	{ return 0; }
    const DBKey&	storedID() const		{ return storedid_; }
    void		emitPRRequest(OD::PresentationRequestType);
    virtual void	handleItemCheck(bool triggerdispreq=true)	{}
    virtual void	prepareForShutdown();

protected:
    DBKey			storedid_;

    virtual OD::ViewerID	getViewerID() const		=0;
};
