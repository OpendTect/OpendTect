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


mExpClass(uiTools) uiPresManagedTreeItem : public uiODTreeItem
{ mODTextTranslationClass(uiPresManagedTreeItem)
public:
			uiPresManagedTreeItem(const uiString&);
    virtual		~uiPresManagedTreeItem();

    virtual OD::ObjPresentationInfo* getObjPRInfo() const	{ return 0; }
    const DBKey&	storedID() const		{ return storedid_; }

    void		setDataObj(SharedObject*);
    void		emitPRRequest(OD::PresentationRequestType);
    virtual void	handleItemCheck(bool triggerdispreq=true)	{}
    virtual void	prepareForShutdown();
    virtual OD::ViewerID getViewerID() const				=0;

protected:

    DBKey		storedid_;

    inline RefMan<SharedObject>& dataObj()		{ return dataobj_; }
    inline const RefMan<SharedObject>& dataObj() const	{ return dataobj_; }

    typedef Monitorable::ChangeData ChangeData;
    void		objChangedCB(CallBacker*);
    virtual void	handleObjChanged(const ChangeData&)		{}

private:

    RefMan<SharedObject> dataobj_;

};


mExpClass(uiTools) uiPresManagedParentTreeItem : public uiODTreeItem
{ mODTextTranslationClass(uiPresManagedParentTreeItem)
public:
			uiPresManagedParentTreeItem(const uiString&);
    virtual		~uiPresManagedParentTreeItem();
    void		setPRManagedViewer(OD::PresentationManagedViewer&);

    void		getLoadedChildren(OD::ObjPresentationInfoSet&) const;
    void		showHideChildren(const OD::ObjPresentationInfo&,bool);
    void		removeChildren(const OD::ObjPresentationInfo&);
    void		addChildren(const OD::ObjPresentationInfoSet&);
    bool		selectChild(const OD::ObjPresentationInfo&);
    void		emitChildPRRequest(const OD::ObjPresentationInfo&,
					   OD::PresentationRequestType);
    virtual uiPresManagedTreeItem*
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
