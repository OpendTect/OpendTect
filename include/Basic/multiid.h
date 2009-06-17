#ifndef multiid_h
#define multiid_h

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	A.H. Bril
 Date:		15-1-2000
 RCS:		$Id: multiid.h,v 1.8 2009-06-17 17:40:55 cvskris Exp $
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
    MultiID&		operator =( const char* s )
			{ id_ = s; return *this; }

    inline bool		operator==( const MultiID& m ) const
			{ return id_ == m.id_; }
    inline bool		operator==( const char* s ) const
			{ return id_ == s; }

    inline int		ID( int idx ) const
			{ return atoi(key(idx).buf()); }
    inline void		setID( int idx, int i )
			{ setKey( idx, getStringFromInt(i) ); }
    int			leafID() const;

    inline MultiID&	add( int i )
			{ *this += getStringFromInt(i); return *this; }

};


#endif
