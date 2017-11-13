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
#include "sharedobject.h"
#include "odpresentationmgr.h"

class SharedObject;
namespace Presentation { class ManagedViewer; }


mExpClass(uiTools) uiPresManagedTreeItem : public uiODTreeItem
{ mODTextTranslationClass(uiPresManagedTreeItem)
public:

    typedef Presentation::ObjInfo	PresInfo;
    typedef Presentation::RequestType	ReqType;
    typedef Presentation::ViewerID	ViewerID;

			uiPresManagedTreeItem(const uiString&);
    virtual		~uiPresManagedTreeItem();

    virtual PresInfo*	getObjPrInfo() const	{ return 0; }
    const DBKey&	storedID() const	{ return storedid_; }

    void		setDataObj(SharedObject*);
    virtual void	handleItemCheck(bool triggerdispreq=true)	{}
    virtual void	prepareForShutdown();
    virtual ViewerID	viewerID() const				= 0;

protected:

    DBKey		storedid_;

    inline RefMan<SharedObject>& dataObj()		{ return dataobj_; }
    inline const RefMan<SharedObject>& dataObj() const	{ return dataobj_; }

    typedef Monitorable::ChangeData ChangeData;
    void		objChangedCB(CallBacker*);
    virtual void	handleObjChanged(const ChangeData&)		{}

private:

    friend class	uiPresManagedParentTreeItem;
    RefMan<SharedObject> dataobj_;

public:

			// Usually done by parent trees only
    void		emitPrRequest(ReqType);

};


mExpClass(uiTools) uiPresManagedParentTreeItem : public uiODTreeItem
{ mODTextTranslationClass(uiPresManagedParentTreeItem)
public:

    typedef Presentation::ObjInfo	PresInfo;
    typedef Presentation::ObjInfoSet	PresInfoSet;
    typedef Presentation::RequestType	ReqType;
    typedef Presentation::ViewerID	ViewerID;

			uiPresManagedParentTreeItem(const uiString&);
    virtual		~uiPresManagedParentTreeItem();
    void		setPrManagedViewer(Presentation::ManagedViewer&);

    void		getLoadedChildren(PresInfoSet&) const;
    void		showHideChildren(const PresInfo&,bool);
    void		removeChildren(const PresInfo&);
    void		addChildren(const PresInfo&);
    void		addChildren(const PresInfoSet&);
    bool		selectChild(const PresInfo&);

    virtual uiPresManagedTreeItem*
			addChildItem(const PresInfo&)
			{ return 0; }

    virtual ViewerID	viewerID() const			= 0;
    virtual const char* childObjTypeKey() const			= 0;

protected:

    virtual void	objAddedCB(CallBacker*);
    virtual void	objVanishReqCB(CallBacker*);
    virtual void	objShowReqCB(CallBacker*);
    virtual void	objHideReqCB(CallBacker*);
    virtual void	objOrphanedCB(CallBacker*);

    void		emitChildPrRequest(const PresInfo&,ReqType);

};
