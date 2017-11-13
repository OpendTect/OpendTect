#pragma once

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
#include "dbkey.h"
#include "groupedid.h"
#include "zdomain.h"
#include "zaxistransform.h"

namespace Presentation
{

    typedef GroupedID::GroupID		ViewerTypeID;
    typedef GroupedID::ObjID		ViewerObjID;

    enum RequestType			{ Add, Show, Hide, Vanish };

    mGlobal(General) const char*	sKeyObj();

    mExpClass(General) ViewerID : public ::GroupedID
    {
    public:
				ViewerID()
				    : ::GroupedID(GroupedID::getInvalid())  {}
				ViewerID( ViewerTypeID vwrtypeid,
					  ViewerObjID vwrobjid )
				    : ::GroupedID(vwrtypeid,vwrobjid)	    {}
	static inline ViewerID	get( GroupNrType gnr, ObjNrType onr )
				{ return ViewerID( gnr, onr ); }

	ViewerTypeID		viewerTypeID() const	{ return groupID(); }
	ViewerObjID		viewerObjID() const	{ return objID(); }

    protected:

				ViewerID( GroupNrType gnr, ObjNrType onr )
				    : GroupedID(gnr,onr)	{}

    };


    mExpClass(General) ObjInfo
    {
    public:

				ObjInfo()		{}
				ObjInfo(const DBKey& dbk)
				: storedid_(dbk)	{}
	virtual			~ObjInfo()		{}

	virtual uiString	getName() const;
	ObjInfo*		clone() const;
	virtual bool		isSaveable() const
					    { return !storedid_.isInvalid();}
	virtual void		fillPar(IOPar&) const;
	virtual bool		usePar(const IOPar&);
	void			setStoredID( const DBKey& id )
							{ storedid_ = id; }
	const DBKey&		storedID() const	{ return storedid_; }
	const char*		objTypeKey() const	{ return objtypekey_; }
	virtual bool		isSameObj(const ObjInfo&) const;

    protected:

	BufferString		objtypekey_;
	DBKey			storedid_;

    };

    mExpClass(General) ObjInfoSet
    {
    public:
				~ObjInfoSet()	{ deepErase( prinfoset_ ); }

	bool			isPresent(const ObjInfo&) const;
	int			size() const	{ return prinfoset_.size(); }
	ObjInfo*		remove(int);
	ObjInfo*		get(int);
	const ObjInfo*		get(int) const;
	bool			add(ObjInfo*);

    protected:

	ObjectSet<ObjInfo>	prinfoset_;

    };

    mExpClass(General) ObjInfoFactory
    {
    public:

	typedef ObjInfo* (*CreateFunc)(const IOPar&);

	void		addCreateFunc(CreateFunc,const char* key);
	ObjInfo*	create(const IOPar&);

    protected:

	TypeSet<CreateFunc>	createfuncs_;
	BufferStringSet		keys_;

    };

    mExpClass(General) ManagedViewer : public CallBacker
    {
    public:
				ManagedViewer();
	virtual			~ManagedViewer();

	inline ViewerID		viewerID() const
				    { return ViewerID(viewerTypeID(),
							viewerObjID()); }
	inline ViewerTypeID	viewerTypeID() const	{ return vwrTypeID(); }
	inline ViewerObjID	viewerObjID() const	{ return viewerobjid_; }
	void			setViewerObjID(ViewerObjID id)
				    { viewerobjid_ = id; }

	const ZAxisTransform*	getZAxisTransform() const
				    { return datatransform_.ptr(); }
	void			setZAxisTransform(ZAxisTransform*);
	bool			hasZAxisTransform() const
				    { return datatransform_; }
	const ZDomain::Info&	zDomain() const		{return *zdomaininfo_; }

	CNotifier<ManagedViewer,IOPar>	ObjAdded;
	CNotifier<ManagedViewer,IOPar>	ObjOrphaned;
	CNotifier<ManagedViewer,IOPar>	UnsavedObjLastCall;
	CNotifier<ManagedViewer,IOPar>	ShowRequested;
	CNotifier<ManagedViewer,IOPar>	HideRequested;
	CNotifier<ManagedViewer,IOPar>	VanishRequested;

    protected:

	ViewerObjID		viewerobjid_;
	RefMan<ZAxisTransform>	datatransform_;
	ZDomain::Info*		zdomaininfo_;

	virtual ViewerTypeID	vwrTypeID() const			= 0;

    };



    mExpClass(General) VwrTypeMgr : public CallBacker
    {
    public:

	virtual ViewerTypeID	viewerTypeID() const		= 0;
	virtual void		handleRequest(ViewerID originivwrid,
						RequestType,const IOPar&);
	void			addViewer( ManagedViewer* vwr )
				    { viewers_ += vwr; }
	ManagedViewer*		getViewer(ViewerObjID);
	const ManagedViewer*	getViewer(ViewerObjID) const;

    protected:

	ObjectSet<ManagedViewer> viewers_;

    };

    mExpClass(General) Manager
    {
    public:

	struct SyncInfo
	{
				SyncInfo( ViewerTypeID vwrtypeid,
					      bool sync )
				    : vwrtypeid_(vwrtypeid)
				    , issynced_(sync)	{}

	    bool		operator==( const SyncInfo& rhs ) const
				{
				    return vwrtypeid_==rhs.vwrtypeid_
					&& issynced_==rhs.issynced_;
				}

	    ViewerTypeID	vwrtypeid_;
	    bool		issynced_;

	};

				Manager();

	VwrTypeMgr*		getViewerTypeMgr(ViewerTypeID vwrtypeid);
	const VwrTypeMgr*	getViewerTypeMgr(ViewerTypeID vwrtypeid) const;
	ManagedViewer*		getViewer(ViewerID vwrid);
	const ManagedViewer*	getViewer(ViewerID vwrid) const;
	void			handleRequest(ViewerID id,RequestType,
					      const IOPar&);
	void			syncAllViewerTypes();
	void			addViewerTypeManager(VwrTypeMgr*);
	bool			canViewerBeSynced(ViewerID,
						      ViewerID) const;
	bool			areViewerTypesSynced(ViewerTypeID,
							ViewerTypeID) const;
    protected:

	ObjectSet<VwrTypeMgr>	vwrtypemanagers_;
	TypeSet<SyncInfo>	vwrtypesyncinfos_;

	int			syncInfoIdx(ViewerTypeID) const;

    };

} // namespace Presentation


namespace OD
{

    mGlobal(General) Presentation::Manager&		PrMan();
    mGlobal(General) Presentation::ObjInfoFactory&	PrIFac();

} // namespace OD
