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
#include "general.h"
#endif
#include <stdlib.h>
#include <iostream>

/*!\brief Concatenated short keys separated by dots.

 Usage is for Object identifiers in the Object Manager, or UnitIDs.

 A Compound Key Glob Expression is a string used for matching.
 It is similar to a UNIX-type glob expression.

*/


mClass CompoundKey
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

    virtual bool	matchGE(const char*) const;

protected:

    BufferString	id_;
    char*		fromKey(int,bool cptobuf=false) const;
    friend std::istream& operator >>(std::istream&,CompoundKey&);

};


inline CompoundKey& CompoundKey::operator +=( const char* s )
{
    if ( !id_.isEmpty() ) id_ += ".";
    id_ += s;
    return *this;
}

mGlobal std::ostream& operator<<(std::ostream&,const CompoundKey&);


#endif
