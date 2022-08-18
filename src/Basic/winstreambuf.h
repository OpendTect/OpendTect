#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#define private protected
#include <fstream>
#undef private
#ifdef __win32__
# define fpos_t od_int64
#endif

#ifndef _POS_TYPE_FROM_STATE
//TODO this is a hack for VS >= 15.8.7, try to get rid of this winstreambuf hack altogether
#define _POS_TYPE_FROM_STATE(postype, state, position) postype(state, position)
#define _POS_TYPE_TO_STATE(pos) pos.state()
#define _BADOFF -1
#endif

namespace std
{

/*!
\brief Adapter to seek in filebuffers on win64.

  Usage like:

<pre>
  std::winfilebuf fb;
  std::istream strm( &fb );
</pre>

  After "C:\Program Files (x86)\Microsoft Visual Studio 8\VC\include\fstream"
  Only change is: fseek -> _fseeki64
*/

mClass(Basic) winfilebuf : public filebuf
{
public:
winfilebuf( const char* fnm, ios_base::openmode om )
    : realpos_(0)
{
	std::filebuf* openstrm = open( fnm, om );
	isok_ = openstrm;
}

bool		isOK() const	{ return isok_; }
od_int64    	getRealPos() const	{ return realpos_; }
protected:

#define defom (ios_base::openmode)(ios_base::in | ios_base::out)

virtual pos_type seekoff( off_type _Off, ios_base::seekdir _Way,
			  ios_base::openmode = defom )
{
    fpos_t _Fileposition;

    if ( _Mysb::gptr() == &_Mychar && _Way == ios_base::cur && _Pcvt == 0 )
	_Off -= (off_type)sizeof (char);

    if ( _Myfile==0 || !_Endwrite() || (_Off!=0 || _Way!=ios_base::cur) &&
	 _fseeki64(_Myfile,_Off,_Way) != 0 ||
	 fgetpos(_Myfile,&_Fileposition) != 0 )
	return (pos_type(_BADOFF));

    if ( _Mysb::gptr() == &_Mychar )
	_Mysb::setg( &_Mychar, &_Mychar+1, &_Mychar+1 );

    realpos_ = (od_int64)_Fileposition;
    return (_POS_TYPE_FROM_STATE(pos_type,_State,_Fileposition));
}

bool	    isok_;
od_int64    realpos_;
};


/*!
\brief Adapter for input streams on win64.
*/

mClass(Basic) winifstream : public istream
{
public:

winifstream( const char* fnm, ios_base::openmode om )
    : istream(nullptr)
{
    fb_ = new winfilebuf( fnm, om );
    rdbuf( fb_ );

    if ( fb_->isOK() )
	clear();
    else
	setstate( ios_base::failbit );
}

~winifstream()
{
    if ( !fb_->close() )
	setstate( ios_base::failbit );
    delete fb_;
}

bool is_open()
{ return fb_->is_open(); }

    winfilebuf*	fb_;
};


/*!
\brief Adapter for output streams on win64.
*/

mClass(Basic) winofstream : public ostream
{
public:

winofstream( const char* fnm, ios_base::openmode om )
    : ostream(nullptr)
{
    fb_ = new winfilebuf( fnm, om );
    rdbuf( fb_ );

    if ( fb_->isOK() )
	clear();
    else
	setstate( ios_base::failbit );
}

~winofstream()
{
    if ( !fb_->close() )
	setstate( ios_base::failbit );
    delete fb_;
}

bool is_open()
{ return fb_->is_open(); }

    winfilebuf*	fb_;
};


} // namespace std


#endif
