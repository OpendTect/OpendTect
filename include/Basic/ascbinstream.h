#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "basicmod.h"
#include "commondefs.h"
#include "plftypes.h"
#include "od_iosfwd.h"


/*!\brief writes to a stream that can be Ascii or Binary.

  In Ascii mode every write will get a post-character or post-string written,
  usually either a tab (default) or a newline. In binary mode the bytes of
  the input values are simply dumped to the stream.

 */


mExpClass(Basic) ascbinostream
{
public:

			ascbinostream(od_ostream&,bool binary);
			ascbinostream(od_ostream*,bool binary);
    virtual		~ascbinostream();
    bool		isOK() const;
    bool		isBad() const;

#   define		mDeclascbinostreamAddFns(typ) \
    ascbinostream&      add(typ,char post=od_tab); \
    ascbinostream&      add(typ,const char* post)

			mDeclascbinostreamAddFns(char);
			mDeclascbinostreamAddFns(unsigned char);
			mDeclascbinostreamAddFns(od_int16);
			mDeclascbinostreamAddFns(od_uint16);
			mDeclascbinostreamAddFns(od_int32);
			mDeclascbinostreamAddFns(od_uint32);
			mDeclascbinostreamAddFns(od_int64);
			mDeclascbinostreamAddFns(od_uint64);
			mDeclascbinostreamAddFns(float);
			mDeclascbinostreamAddFns(double);

    template <class T>
    ascbinostream&      addEOL( T t )		{ return add( t, od_newline ); }

    template <class T,class IT>
    inline ascbinostream& addArr(const T*,IT sz,const char* between="\t",
				 const char* post="\n");
			//!< Will write both sz and the array

    ascbinostream&	addBin(const void*,od_stream_Count nrbytes);
    template <class T>
    inline ascbinostream& addBin( const T& t )
			{ return addBin( &t, sizeof(T) ); }

    od_ostream&		stream()		{ return strm_; }

protected:

    od_ostream&		strm_;
    const bool		binary_;
    const bool		strmmine_;

};


/*!\brief reads from a stream that was created in ascbinostream style.  */

mExpClass(Basic) ascbinistream
{
public:

			ascbinistream(od_istream&,bool binary);
			ascbinistream(od_istream*,bool binary);
    virtual		~ascbinistream();
    bool		isOK() const;
    bool		isBad() const;

    ascbinistream&	get(char&);
    ascbinistream&	get(unsigned char&);
    ascbinistream&	get(od_int16&);
    ascbinistream&	get(od_uint16&);
    ascbinistream&	get(od_int32&);
    ascbinistream&	get(od_uint32&);
    ascbinistream&	get(od_int64&);
    ascbinistream&	get(od_uint64&);
    ascbinistream&	get(float&);
    ascbinistream&	get(double&);

    template <class T,class IT>
    inline ascbinistream& getArr(T* arr,IT sz);
			//!< Will NOT read the sz! Read sz and allocate first

    ascbinistream&	getBin(void*,od_stream_Count nrbytes);
    template <class T>
    inline ascbinistream& getBin( T& t )
			{ return getBin( &t, sizeof(T) ); }

    od_istream&		stream()		{ return strm_; }

protected:

    od_istream&		strm_;
    const bool		binary_;
    const bool		strmmine_;

};


template <class T,class IT>
inline ascbinostream& ascbinostream::addArr( const T* arr, IT sz,
					     const char* between,
					     const char* post )
{
    if ( binary_ )
	return addBin( &sz, sizeof(IT) ).addBin( arr, sz * sizeof(T) );

    strm_ << sz;
    for ( int idx=0; idx<sz; idx++ )
	strm_ << arr[idx] << (idx == sz-1 ? post : between);

    return *this;
}


template <class T,class IT>
inline ascbinistream& ascbinistream::getArr( T* arr, IT sz )
{
    if ( binary_ )
	return getBin( arr, sz*sizeof(T) );

    for ( int idx=0; idx<sz; idx++ )
	strm_ >> arr[idx];
    return *this;
}
