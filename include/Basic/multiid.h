#ifndef multiid_h
#define multiid_h

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


/*!\brief Compound key consisting of ints */

mExpClass(Basic) MultiID : public CompoundKey
{
public:

    typedef int		SubID;

			MultiID(const FixedString& s)
			: CompoundKey(s)	{}
			MultiID( const char* s=0 )
			: CompoundKey(s)	{}
			MultiID( const MultiID& mid )
			: CompoundKey(mid)	{}
			MultiID( SubID id )
			{ add(id); }
			MultiID( SubID id1, SubID id2 )
			{ add(id1).add(id2); }
			MultiID( SubID id1, SubID id2, SubID id3 )
			{ add(id1).add(id2).add(id3); }

    inline MultiID&	operator =( const MultiID& mi )
			{ impl_ = mi.impl_; return *this; }
    inline MultiID&	operator =( const CompoundKey& ck )
			{ impl_ = ck.buf(); return *this; }
    inline MultiID&	operator =( const FixedString& fs )
			{ impl_ = fs.str(); return *this; }
    inline MultiID&	operator =( const char* s )
			{ impl_ = s; return *this; }

    inline bool		operator==( const MultiID& m ) const
			{ return impl_ == m.impl_; }
    inline bool		operator==( const char* s ) const
			{ return impl_ == s; }

    inline SubID	subID( IdxType idx ) const
			{ return key(idx).toInt(); }
    inline void		setSubID( IdxType idx, SubID id )
			{ setKey( idx, toString(id) ); }
    SubID		leafID() const;
    MultiID		parent() const;

    inline MultiID&	set( const MultiID& oth )
			{ *this = oth; return *this; }
    inline MultiID&	set( const char* s )
			{ *this = s; return *this; }
    inline MultiID&	add( SubID id )
			{ *this += toString(id); return *this; }

    static const MultiID& udf();
    inline void		setUdf()		{ *this = udf(); }
    bool		isUdf() const;

};

#endif
