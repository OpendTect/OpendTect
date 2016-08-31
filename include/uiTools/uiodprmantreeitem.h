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
#include "multiid.h"
#include "odpresentationmgr.h"

namespace OD { class PresentationManagedViewer; }

mExpClass(uiTools) uiODPrManagedParentTreeItem : public uiODTreeItem
{ mODTextTranslationClass(uiODPrManagedParentTreeItem)
public:
			uiODPrManagedParentTreeItem(const uiString&);
    virtual		~uiODPrManagedParentTreeItem();
    void		setPRManagedViewer(OD::PresentationManagedViewer&);

    void		getLoadedChildren(TypeSet<MultiID>&) const;
    void		showHideChildren(const MultiID&,bool);
    void		removeChildren(const MultiID&);
    void		addChildren(const TypeSet<MultiID>&);
    bool		selectChild(const MultiID&);
    void		emitChildPRRequest(const MultiID&,
					   OD::PresentationRequestType);

    virtual const char* childObjTypeKey() const			=0;
protected:
    virtual void	objAddedCB(CallBacker*);
    virtual void	objVanishedCB(CallBacker*);
    virtual void	objShownCB(CallBacker*);
    virtual void	objHiddenCB(CallBacker*);
    virtual void	objOrphanedCB(CallBacker*);

    virtual void	addChildItem(const MultiID&)		{}
};


mExpClass(uiTools) uiODPrManagedTreeItem : public uiODTreeItem
{ mODTextTranslationClass(uiODPrManagedTreeItem)
public:
			uiODPrManagedTreeItem(const uiString&);
    const MultiID&	storedID() const		{ return storedid_; }
    void		emitPRRequest(OD::PresentationRequestType);
    virtual void	handleItemCheck(bool triggerdispreq=true)	{}

protected:
    MultiID			storedid_;

    virtual OD::ViewerID	getViewerID() const		=0;
    virtual OD::ObjPresentationInfo* getObjPRInfo()		{ return 0; }
};

#endif
