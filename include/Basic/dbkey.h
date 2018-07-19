#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Bert
 Date:		Sep 2016
________________________________________________________________________

-*/

#include "groupedid.h"
#include "bufstring.h"
#include "uistring.h"
#include "objectset.h"

class BufferStringSet;
class IOObj;
class SurveyDiskLocation;


/*!\brief Key to an object in the OpendTect data store. The key is valid
  within the current survey.

  The OpendTect data store is a meta-datastore. It is defined only by interfaces
  to store and retreive the data. All objects have at least one default storage
  in the flat-file default data storage - for example seismic data can be
  stored in CBVS files. Just as well, it can be stored in other data stores.

  To identify the storage of objects, every object somewhere in a data store
  accessible for opendTect, you need its DBKey. If you need to handle objects in
  multiple surveys, you need the 'FullDBKey'.

  Previously, the key was a string-based class called MultiID. The DBKey
  replaces it with the safer and more compact IDWithGroup.

*/

mExpClass(Basic) DBKey : public IDWithGroup<int,int>
{
public:

    typedef GroupID	DirID;
    typedef GroupNrType	DirNrType;

			DBKey()
			    : auxkey_(0)	{}
			DBKey( const DBKey& oth )
			    : auxkey_(0)	{ *this = oth; }
			DBKey( DirID dirid, ObjID oid=ObjID::getInvalid() )
			    : IDWithGroup<int,int>(dirid,oid)
			    , auxkey_(0)	{}
    virtual		~DBKey();

    virtual DBKey*	clone() const		{ return new DBKey(*this); }
    static DBKey*	getFromString(const char*);

    DBKey&		operator =(const DBKey&);
    bool		operator ==(const DBKey&) const;
			mImplSimpleIneqOper(DBKey)

    static DBKey	getInvalid()		{ return DBKey(-1,-1); }
    static DBKey	get( DirNrType gnr, ObjNrType onr=-1 )
						{ return DBKey(gnr,onr); }
    static DBKey	getFromStr(const char*);
    static DBKey	getFromI64(od_int64);

    virtual bool	isInvalid() const	{ return groupnr_ < 0; }
    virtual bool	isInCurrentSurvey() const { return true; }

			// aliases
    inline bool		hasValidDirID() const	{ return hasValidGroupID(); }
    inline DirID	dirID() const		{ return groupID(); }
    inline void		setDirID( DirID id )	{ setGroupID( id ); }

    virtual BufferString toString() const;
    virtual void	fromString(const char*);
    virtual const SurveyDiskLocation& surveyDiskLocation() const;

    uiString		toUiString() const;

    bool		hasAuxKey() const	{ return auxkey_; }
    BufferString	auxKey() const;
    void		setAuxKey(const char*);

    mDeprecated		DBKey(const char*);
    mDeprecated static	DBKey udf()		{ return getInvalid(); }
    mDeprecated bool	isEmpty() const		{ return isInvalid(); }
    mDeprecated bool	isUdf() const		{ return isInvalid(); }
    mDeprecated void	setEmpty()		{ setInvalid(); }

protected:

			DBKey( DirNrType dnr, ObjNrType onr=-1 )
			    : IDWithGroup<int,int>(dnr,onr)
			    , auxkey_(0)	{}

    BufferString*	auxkey_;

    static void		doGetfromString(DBKey&,const char*);

};



/*!\brief Set of DBKey's. Preserves FullDBKey's */

mExpClass(Basic) DBKeySet : public OD::Set
{ mIsContainer( DBKeySet, ObjectSet<DBKey>, dbkys_ )
public:

    inline		DBKeySet()		{}
    inline		DBKeySet( const DBKeySet& oth )
				    { deepAppendClone( dbkys_, oth.dbkys_ ); }
    explicit		DBKeySet( const DBKey& dbky )
						{ add( dbky ); }
			~DBKeySet()		{ deepErase(dbkys_); }
    virtual DBKeySet*	clone() const		{ return new DBKeySet(*this); }

    inline DBKeySet&	operator =( const DBKeySet& oth )
			{ deepCopyClone( dbkys_, oth.dbkys_ ); return *this; }
    bool		operator ==(const DBKeySet&) const;

    inline size_type	size() const		{ return dbkys_.size(); }
    inline bool		isEmpty() const		{ return dbkys_.isEmpty(); }
    virtual bool	validIdx( od_int64 i ) const
						{ return dbkys_.validIdx(i); }
    idx_type		indexOf(const DBKey&) const;
    inline bool		isPresent( const DBKey& dbky )
						{ return indexOf(dbky) >= 0;}
    DBKey&		get( idx_type idx )	{ return *dbkys_.get(idx); }
    const DBKey&	get( idx_type idx ) const { return *dbkys_.get(idx); }
    DBKey&		operator [](idx_type idx) { return *dbkys_[idx]; }
    const DBKey&	operator [](idx_type idx) const { return *dbkys_[idx]; }
    DBKey&		first()			{ return *dbkys_.first(); }
    const DBKey&	first() const		{ return *dbkys_.first(); }
    DBKey&		last()			{ return *dbkys_.last(); }
    const DBKey&	last() const		{ return *dbkys_.last(); }

    DBKeySet&		add( DBKey* ky )	{ dbkys_.add(ky); return *this;}
    DBKeySet&		add( const DBKey& ky )	{ return add( ky.clone() ); }
    bool		addIfNew(const DBKey&);
    void		append(const DBKeySet&,bool allowduplicates=true);
    void		insert(idx_type,const DBKey&);
    void		useIndexes( const idx_type* idxs )
						{ dbkys_.useIndexes(idxs); }

    void		setEmpty()		{ deepErase( dbkys_ ); }
    virtual void	erase()			{ setEmpty(); }
    DBKeySet&		removeSingle(idx_type);
    DBKeySet&		removeRange(idx_type,idx_type);
    DBKeySet&		remove(const DBKey&);

    void		swap( idx_type i1, idx_type i2 )
						{ dbkys_.swap( i1, i2 ); }

    inline DBKeySet&	operator +=( DBKey* k )		{ return add(k); }
    inline DBKeySet&	operator +=( const DBKey& k )	{ return add(k); }
    inline DBKeySet&	operator -=( const DBKey& k )	{ return remove(k); }

    void		addTo(BufferStringSet&) const;

    // remainder of OD::Set interface

    virtual od_int64	nrItems() const		{ return size(); }
    virtual void	swapItems( od_int64 i1, od_int64 i2 )
			{ swap( (idx_type)i1, (idx_type)i2 ); }
    virtual void	reverse()		{ dbkys_.reverse(); }

};

mDefContainerSwapFunction( Basic, DBKeySet )


mGlobal(Basic) inline BufferString toString( const DBKey& ky )
{ return ky.toString(); }
mGlobal(Basic) inline uiString toUiString( const DBKey& ky )
{ return ky.toUiString(); }

// These functions are implemented in dbman.cc in General, so they only work
// if you also link (and init) the General lib

mGlobal(Basic) BufferString	nameOf(const DBKey&);
mGlobal(Basic) IOObj*		getIOObj(const DBKey&);
