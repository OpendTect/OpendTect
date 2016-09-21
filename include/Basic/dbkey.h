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
#include "typeset.h"

class BufferStringSet;


/*!\brief Full key to any object in the OpendTect data store.

  The OpendTect data store is a meta-datastore. It is defined only by interfaces
  to store and retreive the data. All objects have at least one default storage
  in the flat-file default data storage - for example seismic data can be
  stored in CBVS files. Just as well, it can be stored in other data stores.

  To identify the storage of objects, every object somewhere in a data store
  accessible for opendTect, you need its DBKey.

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
			~DBKey();

    DBKey&		operator =(const DBKey&);
    bool		operator ==(const DBKey&) const;
    inline bool		operator !=( const DBKey& oth ) const
			{ return !(oth == *this); }

    static DBKey	getInvalid()		{ return DBKey(-1,-1); }
    static DBKey	get( DirNrType gnr, ObjNrType onr=-1 )
						{ return DBKey(gnr,onr); }
    static DBKey	getFromString(const char*);
    static DBKey	getFromInt64(od_int64);

    virtual bool	isInvalid() const	{ return groupnr_ < 0; }

			// aliases
    inline bool		hasValidDirID() const	{ return hasValidGroupID(); }
    inline DirID	dirID() const		{ return groupID(); }
    inline void		setDirID( DirID id )	{ setGroupID( id ); }

    virtual BufferString toString() const;
    virtual void	fromString(const char*);

    uiString		toUiString() const;

    bool		hasAuxKey() const	{ return auxkey_; }
    BufferString	auxKey() const;
    void		setAuxKey(const char*);

    mDeprecated		DBKey(const char*);
    mDeprecated static DBKey udf()		{ return getInvalid(); }
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


mExpClass(Basic) DBKeySet : public TypeSet<DBKey>
{
public:

    inline		DBKeySet() : TypeSet<DBKey>()		    {}
    inline		DBKeySet( const DBKeySet& oth )
			    : TypeSet<DBKey>(oth)		    {}
    inline		DBKeySet( size_type sz, DBKey ky )
			    : TypeSet<DBKey>(sz,ky)		    {}

    inline DBKeySet&	operator =( const DBKeySet& oth )
			{ copy( oth ); return *this; }

    void		addTo(BufferStringSet&) const;
};


mGlobal(Basic) inline BufferString toString( const DBKey& ky )
{ return ky.toString(); }
mGlobal(Basic) inline uiString toUiString( const DBKey& ky )
{ return ky.toUiString(); }
