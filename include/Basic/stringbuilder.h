#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "basicmod.h"
#include "bufstring.h"

/*!\brief Builds a string by adding strings. Much faster than string
  manipulation. Only supports adding. */

mExpClass(Basic) StringBuilder
{
public:
			StringBuilder();
			StringBuilder(const StringBuilder&);
			StringBuilder(const char*);
    virtual		~StringBuilder();

    StringBuilder&	operator=(const StringBuilder&);
    StringBuilder&	operator=( const char* s )  { return set( s ); }

    const char*		str() const		    { return buf_; }
    const char*		result() const		    { return buf_ ? buf_ : ""; }
    char*		getCStr(int minlen=-1);

    bool		operator==(const StringBuilder&) const;
    bool		operator!=(const StringBuilder&) const;

    bool		isEmpty() const		    { return !buf_; }
    StringBuilder&	setEmpty();
    StringBuilder&	set(const char*);
    template <class T>
    StringBuilder&	set(const T&);

    StringBuilder&	add(const char*);
    StringBuilder&	add(char,int nr=1);
    StringBuilder&	add(const QString&);
    template <class T>
    StringBuilder&	add(const T&);

    StringBuilder&	addSpace( int nr=1 )	{ return add(' ',nr); }
    StringBuilder&	addTab( int nr=1 )	{ return add('\t',nr); }
    StringBuilder&	addNewLine( int nr=1 )	{ return add('\n',nr); }

protected:

    char*		buf_		= nullptr;
    int			bufsz_		= 0;
    int			curpos_		= 0;

    bool		setBufSz(int,bool cp_old);

};


template <class T> inline
StringBuilder& StringBuilder::add( const T& t )
{
    return add( toString(t) );
}


template <> inline
StringBuilder& StringBuilder::add( const float& f )
{
    return add( toStringPrecise(f) );
}


template <> inline
StringBuilder& StringBuilder::add( const double& d )
{
    return add( toStringPrecise(d) );
}

