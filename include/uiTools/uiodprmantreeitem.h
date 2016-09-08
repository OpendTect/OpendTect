#ifndef uiprmantreeitem_h
#define uiprmantreeitem_h

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

    void		getLoadedChildren(DBKeySet&) const;
    void		showHideChildren(const DBKey&,bool);
    void		removeChildren(const DBKey&);
    void		addChildren(const DBKeySet&);
    bool		selectChild(const DBKey&);
    void		emitChildPRRequest(const DBKey&,
					   OD::PresentationRequestType);

    virtual const char* childObjTypeKey() const			=0;
protected:
    virtual void	objAddedCB(CallBacker*);
    virtual void	objVanishedCB(CallBacker*);
    virtual void	objShownCB(CallBacker*);
    virtual void	objHiddenCB(CallBacker*);
    virtual void	objOrphanedCB(CallBacker*);

    virtual void	addChildItem(const DBKey&)		{}
};


mExpClass(uiTools) uiODPrManagedTreeItem : public uiODTreeItem
{ mODTextTranslationClass(uiODPrManagedTreeItem)
public:
			uiODPrManagedTreeItem(const uiString&);
    const DBKey&	storedID() const		{ return storedid_; }
    void		emitPRRequest(OD::PresentationRequestType);
    virtual void	handleItemCheck(bool triggerdispreq=true)	{}

protected:
    DBKey			storedid_;

    virtual OD::ViewerID	getViewerID() const		=0;
    virtual OD::ObjPresentationInfo* getObjPRInfo()		{ return 0; }
};

#endif
