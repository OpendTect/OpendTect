#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Bert
 Date:		Jan 2007
________________________________________________________________________

-*/

#include "basicmod.h"
#include "sharedobject.h"
#include "manobjectset.h"
#include "dbkey.h"
#include "ptrman.h"
#include "od_iosfwd.h"

class DataPackMgr;
template <class T> class ArrayND;


/*!\brief A data packet: data+positioning and more that needs to be shared.

  The 'category' is meant like:
  'Prestack gather'
  'Wavelet'
  'Fault surface'

  We have fixed the type of the data that is held to float. Moreover, the data
  has to be stored in one or more ArrayND<float> instances.

  A DataPack may be tied to a stored object. If so, the optional dbKey() is
  valid.

*/

mExpClass(Basic) DataPack : public SharedObject
{
public:

    typedef float		value_type;
    typedef ArrayND<value_type>	arrnd_type;
    typedef int			idx_type;
    typedef int			size_type;
    typedef od_int64		total_size_type;
    typedef float		kb_size_type;

    mExpClass(Basic) FullID : public GroupedID
    {
    public:

	typedef GroupID		MgrID;
	typedef ObjID		PackID;

				FullID()		{}
				FullID( MgrID mgrid, PackID packid )
				    : GroupedID(mgrid,packid) {}
	bool			isValid() const { return groupID().isValid()
						      && objID().isValid(); }
	static FullID		getFromString(const char*);
	static FullID		getInvalid();
	static bool		isInDBKey(const DBKey&);
	static FullID		getFromDBKey(const DBKey&);
	void			putInDBKey(DBKey&) const;

				// aliases
	MgrID			mgrID() const		{ return groupID(); }
	PackID			packID() const		{ return objID(); }
    };

    typedef FullID::PackID	ID;
    typedef FullID::MgrID	MgrID;

				mDeclInstanceCreatedNotifierAccess(DataPack);
				mDeclAbstractMonitorableAssignment(DataPack);

    bool			isEmpty() const		{ return gtIsEmpty(); }

    ID				id() const		{ return id_; }
    FullID			fullID( MgrID mgrid ) const
						{ return FullID(mgrid,id()); }
    const char*			category() const	{ return gtCategory(); }
    kb_size_type		nrKBytes() const	{ return gtNrKBytes(); }
    void			dumpInfo( IOPar& p ) const { doDumpInfo(p); }

    static const char*		sKeyCategory();
    static ID			cNoID()			{ return ID(); }


    mImplSimpleMonitoredGetSet( inline, dbKey, setDBKey, DBKey, dbkey_,
				cDBKeyChg() )
    static ChangeType		cDBKeyChg()		{ return 2; }

    size_type			nrArrays() const	{ return gtNrArrays(); }
    const arrnd_type*		arrayData( idx_type iarr ) const
				{ return gtArrayData(iarr); }

protected:

				DataPack(const char* categry);
    virtual			~DataPack();

    virtual const char*		gtCategory() const	{ return category_; }
    virtual bool		gtIsEmpty() const	= 0;
    virtual kb_size_type	gtNrKBytes() const	= 0;
    virtual void		doDumpInfo(IOPar&) const = 0;
    virtual size_type		gtNrArrays() const	{ return 0; }
    virtual const arrnd_type*	gtArrayData(idx_type) const { return 0; }

    void			setManager(const DataPackMgr*);
    const ID			id_;
    const BufferString		category_;
    DBKey			dbkey_;

    const DataPackMgr*		manager_;

    static ID			getNewID();  //!< ensures a global data pack ID
    static kb_size_type		sKb2MbFac(); //!< 1 / 1024

    void			setCategory( const char* c )
					{ mNonConst(category_) = c; }

    friend class		DataPackMgr;

public:

    mDeprecated void		release();
    mDeprecated DataPack*	obtain();

};


/*!\brief Simple DataPack based on an unstructured char array buffer. */

mExpClass(Basic) BufferDataPack : public DataPack
{
public:

			BufferDataPack(char* b=0,total_size_type s=0,
					const char* catgry="Buffer");
			mDeclMonitorableAssignment(BufferDataPack);

    char*		buf()			{ return buf_; }
    char const*		buf() const		{ return buf_; }
    total_size_type	size() const		{ return sz_; }
    void		setBuf(char*,total_size_type);
    bool		mkNewBuf(total_size_type);

    static char*	createBuf(total_size_type);

protected:

			~BufferDataPack();

    char*		buf_;
    total_size_type	sz_;

    virtual bool	gtIsEmpty() const override
						{ return sz_ < 1; }
    virtual kb_size_type gtNrKBytes() const override
						{ return sz_*sKb2MbFac(); }
    virtual void	doDumpInfo( IOPar& p ) const override
						{ DataPack::doDumpInfo(p);}

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

    typedef DataPack::FullID::MgrID	ID;
    typedef DataPack::ID		PackID;
    typedef WeakPtrSet<DataPack>	PackSet;
    mUseType( PackSet,			idx_type );
    mUseType( PackSet,			size_type );
    mUseType( DataPack,			kb_size_type );

    inline static ID	getID( const DataPack::FullID& fid )
						{ return fid.groupID(); }

    bool		isPresent(PackID) const;
    inline bool		isPresent( const DataPack::FullID& f ) const
			{ return f.groupID() == id() && isPresent(f.objID()); }

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
    static ID		BufID();	//!< Simple data buffer: 1
    static ID		PointID();	//!< Sets of 'unconnected' points: 2
    static ID		SeisID();	//!< Cube/Block (N1xN2xN3) data: 3
    static ID		FlatID();	//!< Flat (N1xN2) data: 4
    static ID		SurfID();	//!< Surface (triangulated) data: 5

			// Convenience to get info without any obtain()
    const char*		nameOf(PackID) const;
    static const char*	nameOf(const DataPack::FullID&);
    const char*		categoryOf(PackID) const;
    static const char*	categoryOf(const DataPack::FullID&);
    virtual kb_size_type nrKBytesOf(PackID) const;
    virtual void	dumpInfoFor(PackID,IOPar&) const;

    ID			id() const		{ return id_; }
    void		dumpInfo(od_ostream&) const;
    kb_size_type	nrKBytes() const;

    void		getPackIDs(TypeSet<PackID>&) const;

protected:

    bool				doAdd(const DataPack*);

    ID					id_;
    mutable PackSet			packs_;

    static Threads::Lock		mgrlistlock_;
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

    RefMan<DataPack>	getDP(PackID);
    ConstRefMan<DataPack> getDP(PackID) const;
    WeakPtr<DataPack>	observeDP(PackID) const;

    mDeprecated DataPack* addAndObtain(DataPack*);
    mDeprecated DataPack* obtain(PackID);
    mDeprecated const DataPack* obtain(PackID) const;
    mDeprecated void	release(PackID);
    mDeprecated void	release( const DataPack* dp )
					{ if ( dp ) unRef( dp->id() ); }
    mDeprecated void	releaseAll(bool donotify);
    mDeprecated bool	haveID( PackID pid ) const { return isPresent(pid); }
    mDeprecated bool	haveID( const DataPack::FullID& fid ) const
			{ return isPresent( fid ); }

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

//Implementations

template <class T> inline
RefMan<T> DataPackMgr::get( PackID dpid )
{
    auto pack = getDP( dpid );
    mDynamicCastGet( T*, casted, pack.ptr() );
    return RefMan<T>( casted );
}

template <class T> inline
ConstRefMan<T> DataPackMgr::get( PackID dpid ) const
{
    auto pack = getDP( dpid );
    mDynamicCastGet( const T*, casted, pack.ptr() );
    return ConstRefMan<T>( casted );
}


template <class T> inline
WeakPtr<T> DataPackMgr::observe( PackID dpid ) const
{
    auto pack = getDP( dpid );
    pack.setNoDelete( true );

    mDynamicCastGet( const T*, casted, pack.ptr() );
    return WeakPtr<T>( const_cast<T*>(casted) );
}


template <class T> inline
ConstDataPackRef<T>::ConstDataPackRef( const DataPack* dp )
    : dp_(const_cast<DataPack*>(dp) )
    , ptr_( 0 )
{
    mDynamicCast(T*, ptr_, dp_ );
}


template <class T> inline
ConstDataPackRef<T>::ConstDataPackRef( const ConstDataPackRef<T>& dpr )
    : ptr_( dpr.ptr_ )
    , dp_( dpr.dp_ )
{
    if ( dp_ ) dp_->ref();
}


template <class T> inline
ConstDataPackRef<T>&
ConstDataPackRef<T>::operator=( const ConstDataPackRef<T>& dpr )
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
DataPackRef<T>::DataPackRef( DataPack* p )
    : ConstDataPackRef<T>( p )
{ }


template <class T> inline
DataPackRef<T>::DataPackRef( const DataPackRef<T>& dpr )
    : ConstDataPackRef<T>( dpr )
{ }


template <class T> inline
DataPackRef<T>& DataPackRef<T>::operator=( const DataPackRef<T>& dpr )
{
    ConstDataPackRef<T>::operator=( dpr );
    return *this;
}
