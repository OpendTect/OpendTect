#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "basicmod.h"
#include "bufstring.h"


/*!\brief Concatenated short keys separated by dots.
Used for Object identifiers in the Object Manager, or stratigraphic IDs. */

mExpClass(Basic) CompoundKey
{
public:

			CompoundKey(const char* s=nullptr);
			CompoundKey(const CompoundKey&);
    virtual		~CompoundKey();

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
