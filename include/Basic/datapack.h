#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	A.H.Bril
 Date:		Jan 2007
________________________________________________________________________

-*/

#include "basicmod.h"
#include "groupedid.h"
#include "integerid.h"
#include "manobjectset.h"
#include "multiid.h"
#include "ptrman.h"
#include "sharedobject.h"
#include "threadlock.h"
#include "od_iosfwd.h"

class DataPackMgr;


/*!
\brief A data packet: data+positioning and more that needs to be shared.

  The 'category' is meant like:
  'Prestack gather'
  'Wavelet'
  'Fault surface'
*/

mExpClass(Basic) DataPack : public SharedObject
{
public:

    mExpClass(Basic) FullID : public GroupedID
    {
    public:

	typedef GroupID		MgrID;
	typedef ObjID		PackID;

				FullID()		{}
				FullID( const MultiID& mid )
				    : FullID(MgrID(mid.groupID()),
					     PackID(mid.objectID())) {}
				FullID( MgrID mgrid, PackID packid )
				    : GroupedID(mgrid,packid) {}
	bool			isValid() const { return groupID().isValid()
						      && objID().isValid(); }
	MultiID			asMultiID() const
				{ return MultiID(mgrID().asInt(),
							    packID().asInt()); }
	static FullID		getFromString(const char*);
	static FullID		getInvalid();

				// aliases
	MgrID			mgrID() const		{ return groupID(); }
	PackID			packID() const		{ return objID(); }
    };

    typedef FullID::PackID	ID;
    typedef FullID::MgrID	MgrID;

    inline static ID	getID( const FullID& fid )	{ return fid.packID(); }

			DataPack(const char* categry);
			DataPack(const DataPack&);
    virtual		~DataPack();

    ID			id() const		{ return id_; }
    FullID		fullID( MgrID mgrid ) const
			{ return FullID(mgrid,id()); }
    virtual const char*	category() const	{ return category_.buf(); }

    virtual float	nrKBytes() const	= 0;
    virtual void	dumpInfo(IOPar&) const;

    static const char*	sKeyCategory();
    static ID		cNoID()		{ return ID(); }
    mDeprecated("Use DataPack::ID::getInvalid()")
    static ID		cUdfID()	{ return DataPack::ID::getInvalid(); }

    virtual bool	isOK() const		{ return true; }

    Threads::Lock&	updateLock() const	{ return updatelock_; }

			mDeclInstanceCreatedNotifierAccess(DataPack);

protected:

    void			setManager(const DataPackMgr*);
    const ID			id_;
    const BufferString		category_;
    mutable Threads::Lock	updatelock_;
    const DataPackMgr*		manager_ = nullptr;

    static ID		getNewID();	//!< ensures a global data pack ID
    static float	sKb2MbFac();	//!< 1 / 1024

    void		setCategory( const char* c )
			{ *const_cast<BufferString*>(&category_) = c; }

    friend class	DataPackMgr;
public:
    mDeprecatedDef void		release();
    mDeprecatedDef DataPack*	obtain();
};


/*!
\brief Simple DataPack based on an unstructured char array buffer.
*/

mExpClass(Basic) BufferDataPack : public DataPack
{
public:

			BufferDataPack( char* b=nullptr, od_int64 sz=0,
					const char* catgry="Buffer" );
			BufferDataPack(const BufferDataPack&);
			~BufferDataPack();

    char*		buf()			{ return buf_; }
    char const*		buf() const		{ return buf_; }
    od_int64		size() const		{ return sz_; }
    void		setBuf( char*, od_int64);

    float		nrKBytes() const override { return sz_*sKb2MbFac(); }

protected:

    char*		buf_ = nullptr;
    od_int64		sz_;

};


/*!
\brief Manages DataPacks.

  DataPacks will be managed with everything in it. If you add a Pack, you
  will get the ID of the pack.

  When you obtain the data for looking at it, you can choose to 'only observe'.
  In that case, you'd better use the packToBeRemoved notifier, as the data may
  be deleted at any time. Normally, you want to obtain a reference whilst
  making sure the data is not thrown away.

  This means you *must* release the data pack once you no longer use it, but
 *NEVER* release a pack when you used the 'observing_only' option.

 You can get an appropriate DataPackMgr from the DPM() function.
*/

mExpClass(Basic) DataPackMgr : public CallBacker
{
public:
			// You can, but normally should not, construct
			// a manager. In general, leave it to DPM() - see below.

    typedef DataPack::MgrID	MgrID;
    typedef DataPack::ID	PackID;

    inline static MgrID getID( const DataPack::FullID& fid )
						{ return fid.mgrID(); }

    bool		isPresent(PackID) const;
    inline bool		isPresent( const DataPack::FullID& fid ) const
		    { return id() == fid.mgrID() && isPresent(fid.packID()); }

    // add() functions check isPresent(). If so, the pack is NOT added.
    template <class T>
    inline bool		add( T& p )		{ return doAdd(&p); }
    template <class T>
    inline bool		add( const T& p )	{ return doAdd(&p); }
    template <class T>
    inline bool		add( T* p )		{ return doAdd(p); }
    template <class T>
    inline bool		add( const T* p )	{ return doAdd(p); }
    template <class T>
    inline bool		add( RefMan<T>& p )	{ return add( (T*)p.ptr() ); }
    template <class T>
    inline bool		add( ConstRefMan<T>& p ) { return add( (T*)p.ptr() ); }

    template <class T>
    inline RefMan<T>	get(PackID);
    template <class T>
    inline ConstRefMan<T> get(PackID) const;

    template <class T>
    inline WeakPtr<T>	observe(PackID) const;
			//!<Dynamic casts to T and returns results

    bool		ref(PackID);
			//Convenience. Will ref if it is found
    bool		unRef(PackID);
			//Convenience. Will ref if it is found

    Notifier<DataPackMgr> newPack;		//!< Passed CallBacker* = Pack
    Notifier<DataPackMgr> packToBeRemoved;	//!< Passed CallBacker* = Pack

			// Standard mgr IDs take the low integer numbers
    static MgrID	BufID();	//!< Simple data buffer: 1
    static MgrID	PointID();	//!< Sets of 'unconnected' points: 2
    static MgrID	SeisID();	//!< Cube/Block (N1xN2xN3) data: 3
    static MgrID	FlatID();	//!< Flat (N1xN2) data: 4
    static MgrID	SurfID();	//!< Surface (triangulated) data: 5

			// Convenience to get info without any obtain()
    const char*		nameOf(PackID) const;
    static const char*	nameOf(const DataPack::FullID&);
    const char*		categoryOf(PackID) const;
    static const char*	categoryOf(const DataPack::FullID&);
    virtual float	nrKBytesOf(PackID) const;
    virtual void	dumpInfoFor(PackID,IOPar&) const;

    MgrID		id() const		{ return id_; }
    mDeprecated("Use DataPack::MgrID::getInvalid()")
    static MgrID	cUdfID()	{return DataPack::MgrID::getInvalid();}

    void		dumpInfo(od_ostream&) const;
    float		nrKBytes() const;

    void		getPackIDs(TypeSet<PackID>&) const;

protected:
    MgrID				id_;
    mutable WeakPtrSet<DataPack>	packs_;

    bool				doAdd(const DataPack*);

    static Threads::Lock	mgrlistlock_;
    static ManagedObjectSet<DataPackMgr> mgrs_;

    mDeprecatedDef DataPack*	doObtain(MgrID,bool) const;
    mDeprecatedDef int		indexOf(MgrID) const;

public:

			DataPackMgr(MgrID);
			//!< You can, but normally should not, construct
			//!< a manager. In general, leave it to DPM().
			~DataPackMgr();
			//!< Delete a DataPackMgr only when you have
			//!< created it with the constructor.

    static DataPackMgr& DPM(MgrID);
    static DataPackMgr& DPM(const DataPack::FullID& fid)
			{ return DPM(fid.mgrID()); }
    static DataPackMgr* gtDPM(MgrID,bool);
    static void		dumpDPMs(od_ostream&);

    RefMan<DataPack>	getDP(PackID);
    ConstRefMan<DataPack> getDP(PackID) const;
    WeakPtr<DataPack>	observeDP(PackID) const;

    mDeprecatedDef DataPack* addAndObtain(DataPack*);
    mDeprecatedDef DataPack* obtain(PackID);
    mDeprecatedDef const DataPack* obtain(PackID) const;
    mDeprecatedDef void release(PackID);
    mDeprecatedDef void release( const DataPack* dp )
					{ if ( dp ) unRef( dp->id() ); }
    mDeprecatedDef void releaseAll(bool donotify);
    mDeprecated("Use isPresent")
    bool	haveID( PackID pid ) const { return isPresent(pid); }
    mDeprecated("Use isPresent")
    bool	haveID( const DataPack::FullID& fid ) const
			{ return isPresent( fid ); }

    mDeprecatedDef const ObjectSet<const DataPack>& packs() const;

};

/*! This class is obsolete and will be removed. Use ConstRefMan<T> instead.

*/
template <class T>
mClass(Basic) ConstDataPackRef
{
public:
    mDeprecated("Use ConstRefMan")	ConstDataPackRef(const DataPack* p);
				//!<Assumes p is obtained
    mDeprecated("Use ConstRefMan")
				ConstDataPackRef(const ConstDataPackRef<T>&);
    virtual			~ConstDataPackRef()		{ releaseNow();}

    ConstDataPackRef<T>&	operator=(const ConstDataPackRef<T>&);

    void			releaseNow();

				operator bool() const		{ return ptr_; }
    const T*			operator->() const		{ return ptr_; }
    const T&			operator*() const		{ return *ptr_;}
    const T*			ptr() const			{ return ptr_; }
    const DataPack*		dataPack() const		{ return dp_; }

protected:

    DataPack*			dp_ = nullptr;
    T*				ptr_ = nullptr;
};

/*! This class is obsolete and will be removed. Use RefMan<T> instead.

  Obtains the pack, and releases it when it goes out of scope. Typically used
  to hold a datapack as a local variable. Will also work when there are
  multiple return points.

 Example of usage:

 \code
     DataPackRef<FlatDataPack> fdp = DPM(DataPackMgr::FlatID).obtain( id );

     if ( fdp )
     {
	if ( fdp->info().getTotalSize()== 0 )
	    return; //release is called automatically;
	if ( mIsUdf(fdp->get(0,0) )
	    return; //release is called automatically;

	fdp->set(0,0,0);
     }
     ConstDataPackRef<FlatDataPack> cdp = DPM(DataPackMgr::FlatID).obtain( id );

     if ( cdp )
     {
	if ( cdp->info().getTotalSize()== 0 )
	    return; //release is called automatically;
	if ( mIsUdf(cdp->get(0,0) )
	    return; //release is called automatically;
     }
 \endcode
 */

template <class T>
mClass(Basic) DataPackRef : public ConstDataPackRef<T>
{
public:
    mDeprecated("Use RefMan")	DataPackRef(DataPack* p);
			//!<Assumes p is obtained
    mDeprecated("Use RefMan")	DataPackRef(const DataPackRef<T>&);

    DataPackRef<T>&	operator=(const DataPackRef<T>&);

    void		releaseNow();

    T*			operator->()			{ return this->ptr_; }
    T&			operator*()			{ return *this->ptr_;}
    T*			ptr()				{ return this->ptr_; }
    DataPack*		dataPack()			{ return this->dp_; }
};


mGlobal(Basic) DataPackMgr& DPM(DataPackMgr::MgrID);
		//!< will create a new mgr if needed
mGlobal(Basic) DataPackMgr& DPM(const DataPack::FullID&);
		//!< will return empty dummy mgr if mgr ID not found

//Implementations
template <class T> inline
RefMan<T> DataPackMgr::get( PackID dpid )
{
    RefMan<DataPack> pack = getDP( dpid );
    mDynamicCastGet( T*, casted, pack.ptr() );
    return RefMan<T>( casted );
}

template <class T> inline
ConstRefMan<T> DataPackMgr::get( PackID dpid ) const
{
    ConstRefMan<DataPack> pack = getDP( dpid );
    mDynamicCastGet( const T*, casted, pack.ptr() );
    return ConstRefMan<T>( casted );
}


template <class T> inline
WeakPtr<T> DataPackMgr::observe( PackID dpid ) const
{
    ConstRefMan<DataPack> pack = getDP( dpid );
    pack.setNoDelete( true );

    mDynamicCastGet( const T*, casted, pack.ptr() );
    return WeakPtr<T>( const_cast<T*>(casted) );
}



template <class T> inline
ConstDataPackRef<T>::ConstDataPackRef(const DataPack* p)
    : dp_(const_cast<DataPack*>(p) )
{
    mDynamicCast(T*, ptr_, dp_ );
}


template <class T> inline
ConstDataPackRef<T>::ConstDataPackRef(const ConstDataPackRef<T>& dpr)
    : ptr_( dpr.ptr_ )
    , dp_( dpr.dp_ )
{
    if ( dp_ ) dp_->ref();
}


template <class T> inline
ConstDataPackRef<T>&
ConstDataPackRef<T>::operator=(const ConstDataPackRef<T>& dpr)
{
    releaseNow();
    ptr_ = dpr.ptr_;
    dp_ = dpr.dp_;

    if ( dp_ ) dp_->ref();
    return *this;
}


template <class T> inline
void ConstDataPackRef<T>::releaseNow()
{
    ptr_ = nullptr;
    if ( dp_ )
    {
	dp_->release();
	dp_ = nullptr;
    }
}


template <class T> inline
DataPackRef<T>::DataPackRef(DataPack* p)
    : ConstDataPackRef<T>( p )
{ }


template <class T> inline
DataPackRef<T>::DataPackRef(const DataPackRef<T>& dpr)
    : ConstDataPackRef<T>( dpr )
{ }


template <class T> inline
DataPackRef<T>& DataPackRef<T>::operator=(const DataPackRef<T>& dpr)
{
    ConstDataPackRef<T>::operator=(dpr);
    return *this;
}


