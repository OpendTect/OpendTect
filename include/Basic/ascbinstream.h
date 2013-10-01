#ifndef ascbinstream_h
#define ascbinstream_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Bert
 Date:		Oct 2013
 RCS:		$Id$
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

    ascbinostream&	addBin(const void*,od_stream_Count nrbytes);

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
    ascbinistream&	getBin(void*,od_stream_Count nrbytes);

    od_istream&		stream()		{ return strm_; }

protected:

    od_istream&		strm_;
    const bool		binary_;
    const bool		strmmine_;

};



#endif
