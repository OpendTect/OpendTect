#ifndef multiid_h
#define multiid_h

/*+
________________________________________________________________________

 CopyRight:	(C) de Groot-Bril Earth Sciences B.V.
 Author:	A.H. Bril
 Date:		15-1-2000
 RCS:		$Id: multiid.h,v 1.2 2001-02-16 15:42:24 bert Exp $
________________________________________________________________________

-*/

#include <compoundkey.h>

/*!\brief Compound key consisting of ints */

class MultiID : public CompoundKey
{
public:
			MultiID( const char* s=0 )
			: CompoundKey(s)	{}
			MultiID( const MultiID& mid )
			: CompoundKey(mid)	{}
			MultiID( int i )
			{ add(i); }

    MultiID&		operator =( const MultiID& mi )
			{ id = mi.id; return *this; }
    MultiID&		operator =( const CompoundKey& ck )
			{ id = (const char*)ck; return *this; }
    MultiID&		operator =( const char* s )
			{ id = s; return *this; }

    inline bool		operator==( const MultiID& m ) const
			{ return id == m.id; }
    inline bool		operator==( const char* s ) const
			{ return id == s; }

    inline int		ID( int idx ) const
			{ return atoi(key(idx)); }
    inline void		setID( int idx, int i )
			{ setKey( idx, getStringFromInt("%d",i) ); }
    int			leafID() const;

    inline MultiID&	add( int i )
			{ *this += getStringFromInt("%d",i); return *this; }

};


#endif
