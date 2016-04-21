#ifndef datapack_h
#define datapack_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	A.H.Bril
 Date:		Jan 2007
________________________________________________________________________

-*/

#include "basicmod.h"
#include "namedobj.h"
#include "manobjectset.h"
#include "multiid.h"
#include "notify.h"
#include "od_iosfwd.h"
#include "ptrman.h"

class DataPackMgr;


/*!
\brief A data packet: data+positioning and more that needs to be shared.

  The 'category' is meant like:
  'Prestack gather'
  'Wavelet'
  'Fault surface'
*/

mExpClass(Basic) DataPack : public NamedMonitorable
			  , public RefCount::Referenced
{
public:
    typedef int				ID;
    typedef MultiID			FullID;

    inline static ID	getID( const FullID& fid )	{ return fid.ID(1); }

			DataPack( const char* categry )
			    : NamedMonitorable("<?>")
			    , category_(categry)
			    , manager_( 0 )
			    , id_(getNewID())	{}
			DataPack( const DataPack& dp )
			    : NamedMonitorable( dp )
			    , category_( dp.category_ )
			    , manager_( 0 )
			    , id_(getNewID())	{}
    virtual		~DataPack()		{ sendDelNotif(); }

    ID			id() const		{ return id_; }
    FullID		fullID( int mgrid ) const { return FullID(mgrid,id()); }
    virtual const char*	category() const	{ return category_.buf(); }

    virtual float	nrKBytes() const	= 0;
    virtual void	dumpInfo(IOPar&) const;

    static const char*	sKeyCategory();
    static ID		cNoID()			{ return 0; }

    virtual bool	isOK() const		{ return true; }

    Threads::Lock&	updateLock() const	{ return updatelock_; }

protected:

    void			setManager(const DataPackMgr*);
    const ID			id_;
    const BufferString		category_;

    mutable Threads::Lock	updatelock_;
    const DataPackMgr*		manager_;

    static ID			getNewID();  //!< ensures a global data pack ID
    static float		sKb2MbFac(); //!< 1 / 1024

    void			setCategory( const char* c )
				{ *const_cast<BufferString*>(&category_) = c; }

    friend class		DataPackMgr;

public:

    mDeprecated void		release();
    mDeprecated DataPack*	obtain();

};


/*!
\brief Simple DataPack based on an unstructured char array buffer.
*/

mExpClass(Basic) BufferDataPack : public DataPack
{
public:

			BufferDataPack( char* b=0, od_int64 s=0,
					const char* catgry="Buffer" )
			    : DataPack(catgry)
			    , buf_(b)
			    , sz_(s)		{}

			~BufferDataPack()	{ delete [] buf_; }

    char*		buf()			{ return buf_; }
    char const*		buf() const		{ return buf_; }
    od_int64		size() const		{ return sz_; }
    void		setBuf( char* b, od_int64 s )
			{ delete [] buf_; sz_ = s; buf_ = b; }

    virtual float	nrKBytes() const	{ return sz_*sKb2MbFac(); }

protected:

    char*		buf_;
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

    typedef int		ID;		//!< Each Mgr has its own ID
    inline static ID	getID( const DataPack::FullID& fid )
						{ return fid.ID(0); }

    bool		haveID(DataPack::ID) const;
    inline bool		haveID( const DataPack::FullID& fid ) const
			{ return id() == fid.ID(0) && haveID( fid.ID(1) ); }

    template <class T>
    inline T*		add(T* p)		{ doAdd(p); return p; }
    template <class T>
    inline T*		add(RefMan<T>& p)	{ doAdd(p.ptr()); return p; }
    RefMan<DataPack>	get(DataPack::ID dpid) const;

    template <class T>
    inline RefMan<T>	getAndCast(DataPack::ID dpid) const;
			//!<Dynamic casts to T and returns results

    WeakPtr<DataPack>	observe(DataPack::ID dpid) const;

    template <class T>
    inline WeakPtr<T>	observeAndCast(DataPack::ID dpid) const;
			//!<Dynamic casts to T and returns results

    bool		ref(DataPack::ID dpid);
			//Convenience. Will ref if it is found
    bool		unRef(DataPack::ID dpid);
			//Convenience. Will ref if it is found

    Notifier<DataPackMgr> newPack;		//!< Passed CallBacker* = Pack
    Notifier<DataPackMgr> packToBeRemoved;	//!< Passed CallBacker* = Pack

			// Standard mgr IDs take the low integer numbers
    static ID		BufID();	//!< Simple data buffer: 1
    static ID		PointID();	//!< Sets of 'unconnected' points: 2
    static ID		SeisID();	//!< Cube/Block (N1xN2xN3) data: 3
    static ID		FlatID();	//!< Flat (N1xN2) data: 4
    static ID		SurfID();	//!< Surface (triangulated) data: 5

			// Convenience to get info without any obtain()
    const char*		nameOf(DataPack::ID) const;
    static const char*	nameOf(const DataPack::FullID&);
    const char*		categoryOf(DataPack::ID) const;
    static const char*	categoryOf(const DataPack::FullID&);
    virtual float	nrKBytesOf(DataPack::ID) const;
    virtual void	dumpInfoFor(DataPack::ID,IOPar&) const;

    ID			id() const		{ return id_; }
    void		dumpInfo(od_ostream&) const;
    float		nrKBytes() const;

    void		getPackIDs(TypeSet<DataPack::ID>&) const;

protected:
    void				doAdd(DataPack*);

    mutable Threads::Atomic<int>	nrnull_;
    mutable Threads::SpinRWLock		packslock_;
    TypeSet<WeakPtr<DataPack> >		packs_;

    ID					id_;

    DataPack*				doObtain(ID,bool) const;
    int					indexOf(ID) const;
					//!<Object should be readlocked

    static Threads::Lock	mgrlistlock_;
    static ManagedObjectSet<DataPackMgr> mgrs_;

public:

			DataPackMgr(ID);
			//!< You can, but normally should not, construct
			//!< a manager. In general, leave it to DPM().
			~DataPackMgr();
			//!< Delete a DataPackMgr only when you have
			//!< created it with the constructor.

    static DataPackMgr&	DPM(ID);
    static DataPackMgr*	gtDPM(ID,bool);
    static void		dumpDPMs(od_ostream&);

    /*mDeprecated*/ DataPack*		addAndObtain(DataPack*);
					/*!< The pack becomes mines. Pack is
					    obtained during the lock, i.e.
					    threadsafe. */

    /*mDeprecated*/ DataPack*		obtain( DataPack::ID dpid )
					{ return doObtain(dpid,false); }
    /*mDeprecated*/ const DataPack*	obtain( DataPack::ID dpid ) const
					{ return doObtain(dpid,false); }

    /*mDeprecated*/ void		release(DataPack::ID);
    /*mDeprecated*/ void		release( const DataPack* dp )
					{ if ( dp ) release( dp->id() ); }
    /*mDeprecated*/ void		releaseAll(bool donotify);
};



template <class T>
mClass(Basic) ConstDataPackRef
{
public:
    mDeprecated			ConstDataPackRef(const DataPack* p);
				//!<Assumes p is obtained
    mDeprecated			ConstDataPackRef(const ConstDataPackRef<T>&);
    virtual			~ConstDataPackRef()		{ releaseNow();}

    ConstDataPackRef<T>&	operator=(const ConstDataPackRef<T>&);

    void			releaseNow();

				operator bool() const		{ return ptr_; }
    const T*			operator->() const		{ return ptr_; }
    const T&			operator*() const		{ return *ptr_;}
    const T*			ptr() const			{ return ptr_; }
    const DataPack*		dataPack() const		{ return dp_; }

protected:

    DataPack*			dp_;
    T*				ptr_;
};

/*! Provides safe&easy access to DataPack subclass.

This class is legacy, and will be removed in due time. Use RefMan<T> instead.

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
    mDeprecated		DataPackRef(DataPack* p);
			//!<Assumes p is obtained
    mDeprecated		DataPackRef(const DataPackRef<T>&);

    DataPackRef<T>&	operator=(const DataPackRef<T>&);

    void		releaseNow();

    T*			operator->()			{ return this->ptr_; }
    T&			operator*()			{ return *this->ptr_;}
    T*			ptr()				{ return this->ptr_; }
    DataPack*		dataPack()			{ return this->dp_; }
};


mGlobal(Basic) DataPackMgr& DPM(DataPackMgr::ID);
		//!< will create a new mgr if needed
mGlobal(Basic) DataPackMgr& DPM(const DataPack::FullID&);
		//!< will return empty dummy mgr if mgr ID not found

#define mObtainDataPack( var, type, mgrid, newid ) \
{ \
    unRefAndZeroPtr( var ); \
    \
    RefMan<DataPack> __dp = DPM( mgrid ).get( newid ); \
    mDynamicCastGet( type, __dummy, __dp.ptr() ); \
    var = (type) refPtr( __dummy ); \
}


#define mObtainDataPackToLocalVar( var, type, mgrid, newid ) \
type var = 0; \
mObtainDataPack( var, type, mgrid, newid ); \

//Implementations

template <class T> inline
RefMan<T> DataPackMgr::getAndCast(DataPack::ID dpid) const
{
    RefMan<DataPack> pack = get( dpid );
    mDynamicCastGet( T*, casted, pack.ptr() );
    return RefMan<T>( casted );
}


template <class T> inline
WeakPtr<T> DataPackMgr::observeAndCast( DataPack::ID dpid ) const
{
    RefMan<DataPack> pack = get( dpid );
    pack.setNoDelete( true );

    mDynamicCastGet( T*, casted, pack.ptr() );
    return WeakPtr<T>( casted );
}


template <class T> inline
ConstDataPackRef<T>::ConstDataPackRef(const DataPack* p)
    : dp_(const_cast<DataPack*>(p) )
    , ptr_( 0 )
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
    ptr_ = 0;
    if ( dp_ )
    {
	dp_->unRef();
	dp_ = 0;
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


#endif
