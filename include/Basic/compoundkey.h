#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	A.H. Bril
 Date:		15-1-2000
________________________________________________________________________

-*/


#include "basicmod.h"
#include "bufstring.h"


/*!\brief Concatenated short keys separated by dots.
Used for Object identifiers in the Object Manager, or stratigraphic IDs. */

mExpClass(Basic) CompoundKey
{
public:

    inline		CompoundKey( const char* s=0 )	{ if ( s ) impl_ = s; }
    inline		CompoundKey( const CompoundKey& ck )
			: impl_(ck.impl_)	{}
    inline CompoundKey&	operator=( const char* s )
						{ impl_ = s; return *this;}
    inline CompoundKey&	operator+=( const char* );
    inline bool		operator==( const char* s ) const
						{ return impl_ == s; }
    inline bool		operator==( const CompoundKey& oth ) const
						{ return impl_ == oth.impl_; }
    inline bool		operator!=( const char* s ) const
						{ return impl_ != s; }
    inline bool		operator!=(const CompoundKey& u) const
						{ return impl_ != u.impl_; }
    inline void		setEmpty()		{ impl_.setEmpty(); }
    inline bool		isEmpty() const		{ return impl_.isEmpty();}
    inline char*	getCStr()		{ return impl_.getCStr(); }
    inline const char*	buf() const		{ return impl_.buf(); }
    inline		operator const char*() const = delete;
//						{ return buf(); }

    int			nrKeys() const;
    BufferString	key(int) const;
    void		setKey(int,const char*);
    CompoundKey		upLevel() const;
    bool		isUpLevelOf(const CompoundKey&) const;

protected:

    BufferString	impl_;
    char*		fromKey(int) const;
    const char*		getKeyPart(int) const;

private:

    char*		fetchKeyPart(int,bool) const;

};


inline CompoundKey& CompoundKey::operator +=( const char* s )
{
    if ( !impl_.isEmpty() )
	impl_.add( "." );
    impl_.add( s );
    return *this;
}


