#ifndef odpresentationmgr_h
#define odpresentationmgr_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Satyaki Maitra
 Date:		August 2016
________________________________________________________________________

-*/

#include "bufstringset.h"
#include "generalmod.h"
#include "idxpair.h"
#include "commondefs.h"
#include "callback.h"
#include "notify.h"
#include "typeset.h"
#include "objectset.h"
#include "multiid.h"
#include "groupedid.h"

namespace OD
{

    typedef short int 				GroupNrType;
    typedef int 				ObjNrType;
    enum PresentationRequestType		{ Add, Vanish, Show, Hide };
    typedef GroupedID<GroupNrType,ObjNrType>	ViewerGroupedID;
    typedef ViewerGroupedID::GroupIDType 	ViewerTypeID; 
    typedef ViewerGroupedID::ObjIDType 		ViewerObjID; 


mExpClass(General) ViewerID : public ViewerGroupedID
{
public:
				ViewerID()
				: ViewerGroupedID(ViewerGroupedID::getInvalid())
				{}

				ViewerID( ViewerTypeID vwrtypeid,
					  ViewerObjID vwrobjid )
				: ViewerGroupedID(ViewerGroupedID::getInvalid())
				{
				    setGroupID( vwrtypeid );
				    setObjID( vwrobjid );
				}
				ViewerID(GroupNrType vwrtypeid, ObjNrType vwrid)
				: ViewerGroupedID(
				    ViewerGroupedID::get(vwrtypeid,vwrid) )
				{}
    ViewerTypeID		viewerTypeID()	{ return getGroupID(); }
    ViewerObjID			viewerObjID()	{ return getObjID(); }

};


mExpClass(General) ObjPresentationInfo
{
public:

    virtual				~ObjPresentationInfo()	{}
    virtual void			fillPar(IOPar&) const;
    virtual bool			usePar(const IOPar&);
    void				setStoredID(const MultiID& id)
					{ storedid_ = id; }
    const MultiID&			storedID() const
					{ return storedid_; }
    const char*				objTypeKey() const
					{ return objtypekey_; }
protected:
    MultiID				storedid_;
    BufferString			objtypekey_;
};



mExpClass(General) ObjPresentationInfoFactory
{
public:
    typedef ObjPresentationInfo* (*CreateFunc)( const IOPar& );

    void			addCreateFunc(CreateFunc, const char* key);
    ObjPresentationInfo*	create(const IOPar&);
protected:
    TypeSet<CreateFunc>		createfuncs_;
    BufferStringSet		keys_;
};


mGlobal(General) ObjPresentationInfoFactory& PRIFac();


mExpClass(General) PresentationManagedViewer : public CallBacker
{
public:
				PresentationManagedViewer();
    virtual			~PresentationManagedViewer();
    virtual ViewerTypeID	viewerTypeID() const			=0;
    ViewerObjID			viewerObjID() const	{ return viewerobjid_; }
    void			setViewerObjID(ViewerObjID id)
				{ viewerobjid_ = id; }

    CNotifier<PresentationManagedViewer,IOPar>	ObjAdded;
    CNotifier<PresentationManagedViewer,IOPar>	ObjOrphaned;
    CNotifier<PresentationManagedViewer,IOPar>	UnsavedObjLastCall;
    CNotifier<PresentationManagedViewer,IOPar>	ShowRequested;
    CNotifier<PresentationManagedViewer,IOPar>	HideRequested;
    CNotifier<PresentationManagedViewer,IOPar>	VanishRequested;

protected:
    ViewerObjID			viewerobjid_;
};



mExpClass(General) VwrTypePresentationMgr : public CallBacker
{
public:
    virtual ViewerTypeID	viewerTypeID()		=0;
    void			request(PresentationRequestType,
					const IOPar&,
				    ViewerObjID skipvwrid
				    =ViewerObjID::get(-1));
					// -1= do not skip any
    void			addViewer( PresentationManagedViewer* vwr )
				{ viewers_ += vwr; }
protected:
    ObjectSet<PresentationManagedViewer> viewers_;
};

mExpClass(General) PresentationManager
{
public:

    struct SyncInfo
    {
				SyncInfo( ViewerTypeID vwrtypeid,
					  bool sync )
				    : vwrtypeid_(vwrtypeid)
				    , issynced_(sync)	{}
	ViewerTypeID	vwrtypeid_;
	bool			issynced_;
	bool operator==( const SyncInfo& rhs ) const
	{
	    return vwrtypeid_==rhs.vwrtypeid_ && issynced_==rhs.issynced_;
	}
    };

				PresentationManager();

    VwrTypePresentationMgr*	getViewerTypeMgr(ViewerTypeID vwrtypeid);
    void			request(ViewerID id,
					PresentationRequestType,
					const IOPar&);
    void			syncAllViewerTypes();
    void			addViewerTypeManager(VwrTypePresentationMgr*);
    bool			areViewerTypesSynced(ViewerTypeID,
						    ViewerTypeID) const;
protected:
    ObjectSet<VwrTypePresentationMgr>	vwrtypemanagers_;
    TypeSet<SyncInfo>			vwrtypesyncinfos_;

    int				syncInfoIdx(ViewerTypeID) const;
};

mGlobal(General) PresentationManager& PrMan();

}
//namedpace OD

#endif
