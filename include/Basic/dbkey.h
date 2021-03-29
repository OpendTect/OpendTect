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
class SurveyInfo;
class SurveyDiskLocation;


/*!\brief Key to an object in the OpendTect data store. The key is valid
  within the current survey.

  The OpendTect data store is a meta-datastore. It is defined only by interfaces
  to store and retreive the data. All objects have at least one default storage
  in the flat-file default data storage - for example seismic data can be
  stored in CBVS files. Just as well, it can be stored in other data stores.

  To identify the storage of objects, every object somewhere in a data store
  accessible for opendTect, you need its DBKey. If you need to handle objects in
  multiple surveys, you need to do stuff with the SurveyDiskLocation.

  Previously, the key was a string-based class called MultiID. The DBKey
  replaces it with the safer and more compact IDWithGroup.

*/

mExpClass(Basic) DBKey : public IDWithGroup<int,int>
{
public:

    typedef GroupID	DirID;
    typedef GroupNrType	DirNrType;

			DBKey()				{}
    explicit		DBKey( const char* s )		{ fromString(s); }
    explicit		DBKey( const OD::String& s )	{ fromString(s.str()); }
    explicit		DBKey( DirID dirid, ObjID oid=ObjID::getInvalid() )
			    : IDWithGroup<int,int>(dirid,oid)	{}
			DBKey( const DBKey& oth )	{ *this = oth; }
			~DBKey();

    DBKey&		operator =(const DBKey&);
    bool		operator ==(const DBKey&) const;
			mImplSimpleIneqOper(DBKey)

    static const DBKey&	getInvalid();
    static DBKey	get( DirNrType gnr, ObjNrType onr=-1 )
						{ return DBKey(gnr,onr); }
    static DBKey	getFromStr(const char*);
    static DBKey	getFromI64(od_int64);

    bool		isInvalid() const override { return groupnr_ < 0; }
    bool		isUsable() const;
    bool		isInCurrentSurvey() const;

			// aliases
    inline bool		hasValidDirID() const	{ return hasValidGroupID(); }
    inline DirID	dirID() const		{ return groupID(); }
    inline void		setDirID( DirID id )	{ setGroupID( id ); }

    BufferString	toString() const override
			{ return getString(true,false); }
    BufferString	getString(bool withsurvloc,bool forcesl=false) const;
    void		fromString(const char*) override;

    bool		hasSurveyLocation() const { return survloc_; }
    const SurveyDiskLocation& surveyDiskLocation() const;
    const SurveyInfo&	surveyInfo() const;
    BufferString	surveyName() const;

    void		setSurveyDiskLocation(const SurveyDiskLocation&);
    void		clearSurveyDiskLocation();
    DBKey		getLocal() const;

    uiString		toUiString() const;

    bool		hasAuxKey() const	{ return auxkey_; }
    BufferString	auxKey() const;
    void		setAuxKey(const char*);

    mDeprecated static	DBKey udf()		{ return getInvalid(); }
    mDeprecated bool	isEmpty() const		{ return isInvalid(); }
    mDeprecated bool	isUdf() const		{ return isInvalid(); }
    mDeprecated void	setEmpty()		{ setInvalid(); }

    inline BufferString	name() const;
    inline BufferString	mainFile() const;
    inline bool		implExists() const;
    inline IOObj*	getIOObj() const;

protected:

			DBKey( DirNrType dnr, ObjNrType onr=-1 )
			    : IDWithGroup<int,int>(dnr,onr)	{}

    BufferString*	auxkey_			= 0;
    SurveyDiskLocation*	survloc_		= 0;

    static void		doGetfromString(DBKey&,const char*);

};



/*!\brief Set of DBKey's */

mExpClass(Basic) DBKeySet : public OD::Set
{ mIsContainer( DBKeySet, ObjectSet<DBKey>, dbkys_ )
public:

    inline		DBKeySet()		{}
    inline		DBKeySet( const DBKeySet& oth )
				    { deepAppend( dbkys_, oth.dbkys_ ); }
    explicit		DBKeySet( const DBKey& dbky )
						{ add( dbky ); }
			~DBKeySet()		{ deepErase(dbkys_); }
    DBKeySet*		clone() const override	{ return new DBKeySet(*this); }

    inline DBKeySet&	operator =( const DBKeySet& oth )
			{ deepCopy( dbkys_, oth.dbkys_ ); return *this; }
    bool		operator ==(const DBKeySet&) const;
    bool		operator !=(const DBKeySet&) const;

    inline size_type	size() const		{ return dbkys_.size(); }
    inline bool		isEmpty() const		{ return dbkys_.isEmpty(); }
    bool		validIdx( od_int64 i ) const override
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
    DBKeySet&		add( const DBKey& ky )	{ return add(new DBKey(ky)); }
    bool		addIfNew(const DBKey&);
    void		append(const DBKeySet&,bool allowduplicates=true);
    void		insert(idx_type,const DBKey&);
    void		useIndexes( const idx_type* idxs )
						{ dbkys_.useIndexes(idxs); }

    void		setEmpty()		{ deepErase( dbkys_ ); }
    void		erase() override	{ setEmpty(); }
    DBKeySet&		removeSingle(idx_type);
    DBKeySet&		removeRange(idx_type,idx_type);
    DBKeySet&		remove(const DBKey&);
    DBKeySet&		removeUnusable(); // i.e. that do not lead to an IOObj

    void		swap( idx_type i1, idx_type i2 )
						{ dbkys_.swap( i1, i2 ); }

    inline DBKeySet&	operator +=( DBKey* k )		{ return add(k); }
    inline DBKeySet&	operator +=( const DBKey& k )	{ return add(k); }
    inline DBKeySet&	operator -=( const DBKey& k )	{ return remove(k); }

    void		addTo(BufferStringSet&) const;

    // remainder of OD::Set interface

    od_int64		nrItems() const override	{ return size(); }
    void		swapItems( od_int64 i1, od_int64 i2 ) override
			{ swap( (idx_type)i1, (idx_type)i2 ); }
    void		reverse() override		{ dbkys_.reverse(); }

};

mDefContainerSwapFunction( Basic, DBKeySet )

inline BufferString toString( const DBKey& ky )	{ return ky.toString(); }
inline uiString toUiString( const DBKey& ky )	{ return ky.toUiString(); }

// These functions are implemented in dbman.cc in General, so they only work
// if you also link (and init) the General lib

mGlobal(Basic) BufferString	nameOf(const DBKey&);
mGlobal(Basic) BufferString	mainFileOf(const DBKey&);
mGlobal(Basic) bool		implExists(const DBKey&);
mGlobal(Basic) IOObj*		getIOObj(const DBKey&);
mGlobal(Basic) void		delIOObj(IOObj*);

inline BufferString	DBKey::name() const	{ return ::nameOf(*this); }
inline BufferString	DBKey::mainFile() const	{ return ::mainFileOf(*this); }
inline bool		DBKey::implExists() const { return ::implExists(*this);}
inline IOObj*		DBKey::getIOObj() const	{ return ::getIOObj(*this); }
