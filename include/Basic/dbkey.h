#ifndef dbkey_h
#define dbkey_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	A.H. Bril
 Date:		15-1-2000
________________________________________________________________________

-*/

#include "basicmod.h"
#include "compoundkey.h"
#include "string2.h"
#include "fixedstring.h"
#include "typeset.h"


/*!\brief Compound key consisting of ints */

mExpClass(Basic) DBKey : public CompoundKey
{
public:

    typedef int		SubID;

			DBKey(const FixedString& s)
			: CompoundKey(s)	{}
			DBKey( const char* s=0 )
			: CompoundKey(s)	{}
			DBKey( const DBKey& mid )
			: CompoundKey(mid)	{}
			DBKey( SubID id )
			{ add(id); }
			DBKey(SubID,SubID);

    inline DBKey&	operator =( const DBKey& mi )
			{ impl_ = mi.impl_; return *this; }
    inline DBKey&	operator =( const CompoundKey& ck )
			{ impl_ = ck.buf(); return *this; }
    inline DBKey&	operator =( const FixedString& fs )
			{ impl_ = fs.str(); return *this; }
    inline DBKey&	operator =( const char* s )
			{ impl_ = s; return *this; }

    inline bool		operator==( const DBKey& m ) const
			{ return impl_ == m.impl_; }
    inline bool		operator==( const char* s ) const
			{ return impl_ == s; }

    inline SubID	subID( IdxType idx ) const
			{ return key(idx).toInt(); }
    inline void		setSubID( IdxType idx, SubID id )
			{ setKey( idx, toString(id) ); }
    SubID		getIDAt(int lvl) const;
    SubID		leafID() const;
    DBKey		parent() const;

    inline DBKey&	set( const DBKey& oth )
			{ *this = oth; return *this; }
    inline DBKey&	set( const char* s )
			{ *this = s; return *this; }
    inline DBKey&	add( SubID id )
			{ *this += toString(id); return *this; }

    od_int64		toInt64() const;
    static DBKey	fromInt64(od_int64);

    static const DBKey& getInvalid();
    mDeprecated static const DBKey& udf()	{ return getInvalid(); }
    inline void		setUdf()		{ setInvalid(); }
    inline void		setInvalid()		{ *this = getInvalid(); }
    bool		isUdf() const		{ return isInvalid(); }
    bool		isInvalid() const;

};


mExpClass(Basic) DBKeySet : public TypeSet<DBKey>
{
public:
			DBKeySet() : TypeSet<DBKey>()		{}
			DBKeySet( const DBKeySet& oth )
			    : TypeSet<DBKey>(oth)		{}
			DBKeySet( size_type sz, DBKey ky )
			    : TypeSet<DBKey>(sz,ky)		{}
    inline DBKeySet&	operator =( const DBKeySet& oth )
			{ copy( oth ); return *this; }

};


#endif
