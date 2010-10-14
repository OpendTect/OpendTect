#ifndef multiid_h
#define multiid_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	A.H. Bril
 Date:		15-1-2000
 RCS:		$Id: multiid.h,v 1.13 2010-10-14 09:58:06 cvsbert Exp $
________________________________________________________________________

-*/

#include "compoundkey.h"
#include "string2.h"
#include "fixedstring.h"


/*!\brief Compound key consisting of ints */

mClass MultiID : public CompoundKey
{
public:
			MultiID(const FixedString& s)
			: CompoundKey(s)	{}
			MultiID( const char* s=0 )
			: CompoundKey(s)	{}
			MultiID( const MultiID& mid )
			: CompoundKey(mid)	{}
			MultiID( int i )
			{ add(i); }

    MultiID&		operator =( const MultiID& mi )
			{ id_ = mi.id_; return *this; }
    MultiID&		operator =( const CompoundKey& ck )
			{ id_ = (const char*)ck; return *this; }
    MultiID&		operator =( const FixedString& fs )
			{ id_ = fs.str(); return *this; }
    MultiID&		operator =( const char* s )
			{ id_ = s; return *this; }

    inline bool		operator==( const MultiID& m ) const
			{ return id_ == m.id_; }
    inline bool		operator==( const char* s ) const
			{ return id_ == s; }

    inline int		ID( int idx ) const
			{ return toInt(key(idx).buf()); }
    inline void		setID( int idx, int i )
			{ setKey( idx, toString(i) ); }
    int			leafID() const;

    inline MultiID&	add( int i )
			{ *this += toString(i); return *this; }

};


#endif
