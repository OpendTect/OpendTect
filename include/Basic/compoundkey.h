#ifndef compoundkey_h
#define compoundkey_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	A.H. Bril
 Date:		15-1-2000
 RCS:		$Id$
________________________________________________________________________

-*/


#ifndef general_h
#include "basicmod.h"
#include "general.h"
#endif

/*!\brief Concatenated short keys separated by dots.
Used for Object identifiers in the Object Manager, or stratigraphic IDs. */

mExpClass(Basic) CompoundKey
{
public:

    inline		CompoundKey( const char* s=0 )	{ if ( s ) id_ = s; }
    inline		CompoundKey( const CompoundKey& ck ) 
			: id_(ck.id_)			{}
    inline CompoundKey&	operator=(const char* s)	{id_ = s; return *this;}
    inline CompoundKey&	operator+=(const char*);
    inline bool		operator==(const char* s) const	{ return id_ == s; }
    inline bool		operator==(const CompoundKey& u) const
							{ return id_ == u.id_; }
    inline bool		operator!=(const char* s) const	{ return id_ != s; }
    inline bool		operator!=(const CompoundKey& u) const
							{ return id_ != u.id_; }
    inline void		setEmpty()			{ id_.setEmpty(); }
    inline bool		isEmpty() const			{ return id_.isEmpty();}
    inline char*	buf()				{ return id_.buf(); }
    inline const char*	buf() const			{ return id_.buf(); }
    inline		operator const char*() const	{ return buf(); }

    int			nrKeys() const;
    BufferString	key(int) const;
    void		setKey(int,const char*);
    CompoundKey		upLevel() const;
    bool		isUpLevelOf(const CompoundKey&) const;

protected:

    BufferString	id_;
    char*		fromKey(int) const;
    const char*		getKeyPart(int) const;
    mGlobal(Basic) friend std::istream& operator >>(std::istream&,CompoundKey&);

private:

    char*		fetchKeyPart(int,bool) const;

};


inline CompoundKey& CompoundKey::operator +=( const char* s )
{
    if ( !id_.isEmpty() )
	id_.add( "." );
    id_.add( s );
    return *this;
}

mGlobal(Basic) std::ostream& operator <<(std::ostream&,const CompoundKey&);
mGlobal(Basic) std::istream& operator >>(std::istream&,CompoundKey&);


#endif

