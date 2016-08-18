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
#include "commondefs.h"
#include "callback.h"
#include "notify.h"
#include "typeset.h"
#include "objectset.h"
#include "multiid.h"


mExpClass(General) ObjPresentationInfo
{
public:

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


mExpClass(General) ODVwrTypePresentationMgr : public CallBacker
{
public:
						ODVwrTypePresentationMgr();
    virtual int					viewerTypeID()		=0;
    CNotifier<ODVwrTypePresentationMgr,IOPar>	ObjAdded;
    CNotifier<ODVwrTypePresentationMgr,IOPar>	ObjOrphaned;
    CNotifier<ODVwrTypePresentationMgr,IOPar>	UnsavedObjLastCall;
    CNotifier<ODVwrTypePresentationMgr,IOPar>	ShowRequested;
    CNotifier<ODVwrTypePresentationMgr,IOPar>	HideRequested;
    CNotifier<ODVwrTypePresentationMgr,IOPar>	VanishRequested;
};


mExpClass(General) ODPresentationManager
{
public:

    struct SyncInfo
    {
				SyncInfo( int did, bool sync )
				    : vwrtypeid_(did)
				    , issynced_(sync)	{}
	int			vwrtypeid_;
	bool			issynced_;
	bool operator==( const SyncInfo& rhs ) const
	{
	    return vwrtypeid_==rhs.vwrtypeid_ && issynced_==rhs.issynced_;
	}
    };

    enum RequestType		{ Add, Vanish, Show, Hide };

				ODPresentationManager();

    ODVwrTypePresentationMgr*	getViewerTypeMgr(int dispdomainid);
    void			request(int vwrtypeid,RequestType,const IOPar&);
    void			syncAllViewerTypes();
    void			addViewerTypeManager(ODVwrTypePresentationMgr*);
    bool			areViewerTypesSynced(int vwrtypeid1,
						     int vwrtypeid2) const;
protected:
    ObjectSet<ODVwrTypePresentationMgr> vwrtypemanagers_;
    TypeSet<SyncInfo>			vwrtypesyncinfos_;

    int				syncInfoIdx(int dispdomainid) const;
};

mGlobal(General) ODPresentationManager& ODPrMan();


#endif
