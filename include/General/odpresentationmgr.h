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
#include "integerid.h"
#include <utility>

typedef IntegerID<int>	ViewerSubID;
typedef std::pair< ViewerSubID, ViewerSubID > ViewerSubID_Pair;

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


mGlobal(General) ObjPresentationInfoFactory& ODIFac();


mExpClass(General) PresentationManagedViewer : public CallBacker
{
public:
				PresentationManagedViewer();
    virtual			~PresentationManagedViewer();
    virtual ViewerSubID		viewerTypeID() const			=0;
    ViewerSubID			viewerID() const	{ return viewerid_; }
    void			setViewerID(ViewerSubID id)
				{ viewerid_ = id; }

    CNotifier<PresentationManagedViewer,IOPar>	ObjAdded;
    CNotifier<PresentationManagedViewer,IOPar>	ObjOrphaned;
    CNotifier<PresentationManagedViewer,IOPar>	UnsavedObjLastCall;
    CNotifier<PresentationManagedViewer,IOPar>	ShowRequested;
    CNotifier<PresentationManagedViewer,IOPar>	HideRequested;
    CNotifier<PresentationManagedViewer,IOPar>	VanishRequested;

protected:
    ViewerSubID			viewerid_;
};


mExpClass(General) ODViewerID : public ViewerSubID_Pair
{
public:
				ODViewerID( ViewerSubID vwrtypeid,
					    ViewerSubID vwrid )
				    : ViewerSubID_Pair(vwrtypeid,vwrid) {}
    ViewerSubID			viewerTypeID()	{ return first; }
    ViewerSubID			viewerID()	{ return second; }
};


namespace OD
{
    enum PresentationRequestType	{ Add, Vanish, Show, Hide };
}


mExpClass(General) ODVwrTypePresentationMgr : public CallBacker
{
public:
    virtual ViewerSubID		viewerTypeID()		=0;
    void			request(OD::PresentationRequestType,
					const IOPar&,
				    ViewerSubID skipvwrid=ViewerSubID::get(-1));
					// -1= do not skip any
    void			addViewer( PresentationManagedViewer* vwr )
				{ viewers_ += vwr; }
protected:
    ObjectSet<PresentationManagedViewer> viewers_;
};

mExpClass(General) ODPresentationManager
{
public:

    struct SyncInfo
    {
				SyncInfo( ViewerSubID vwrtypeid, bool sync )
				    : vwrtypeid_(vwrtypeid)
				    , issynced_(sync)	{}
	ViewerSubID		vwrtypeid_;
	bool			issynced_;
	bool operator==( const SyncInfo& rhs ) const
	{
	    return vwrtypeid_==rhs.vwrtypeid_ && issynced_==rhs.issynced_;
	}
    };

				ODPresentationManager();

    ODVwrTypePresentationMgr*	getViewerTypeMgr(ViewerSubID dispdomainid);
    void			request(ODViewerID id,
					OD::PresentationRequestType,
					const IOPar&);
    void			syncAllViewerTypes();
    void			addViewerTypeManager(ODVwrTypePresentationMgr*);
    bool			areViewerTypesSynced(ViewerSubID vwrtypeid1,
						     ViewerSubID typeid2) const;
protected:
    ObjectSet<ODVwrTypePresentationMgr> vwrtypemanagers_;
    TypeSet<SyncInfo>			vwrtypesyncinfos_;

    int				syncInfoIdx(ViewerSubID dispdomainid) const;
};

mGlobal(General) ODPresentationManager& ODPrMan();


#endif
