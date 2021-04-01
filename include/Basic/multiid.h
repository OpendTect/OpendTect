#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	A.H. Bril
 Date:		15-1-2000
 RCS:		$Id$
________________________________________________________________________

-*/

#include "basicmod.h"
#include "compoundkey.h"
#include "string2.h"
#include "fixedstring.h"


/*!
\brief Compound key consisting of ints.
*/

mExpClass(Basic) MultiID : public CompoundKey
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
			MultiID( int i1, int i2 )
			{ add(i1); add(i2); }
			MultiID( int i1, int i2, int i3 )
			{ add(i1); add(i2); add(i3); }

    inline MultiID&	operator =( const MultiID& mi )
			{ impl_ = mi.impl_; return *this; }
    inline MultiID&	operator =( const CompoundKey& ck )
			{ impl_ = ck.buf(); return *this; }
    inline MultiID&	operator =( const FixedString& fs )
			{ impl_ = fs.str(); return *this; }
    inline MultiID&	operator =( const char* s )
			{ impl_ = s; return *this; }

    inline bool		operator ==( const MultiID& m ) const
			{ return impl_ == m.impl_; }
    inline bool		operator ==( const char* s ) const
			{ return impl_ == s; }
    inline bool		operator !=( const MultiID& m ) const
			{ return impl_ != m.impl_; }
    inline bool		operator !=( const char* s ) const
			{ return impl_ != s; }

    inline int		ID( int idx ) const
			{ return key(idx).toInt(); }
    inline void		setID( int idx, int i )
			{ setKey( idx, toString(i) ); }
    int			leafID() const;
    MultiID		parent() const;

    inline MultiID&	add( int i )
			{ *this += toString(i); return *this; }

    static const MultiID& udf();
    inline void		setUdf()		{ *this = udf(); }
    inline bool		isUdf() const		{ return *this == udf(); }
};

