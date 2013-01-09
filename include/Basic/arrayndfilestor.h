#ifndef arrayndfilestor_h
#define arrayndfilestor_h
/*
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	K. Tingdahl
 Date:		9-3-1999
 RCS:		$Id$
________________________________________________________________________

*/


#include "arraynd.h"
#include "envvars.h"
#include <fstream>

#define mArrNDChunkSz 1024


/*!
\ingroup Basic
\brief ArrayND I/O from/to a file.
*/

template <class T>
class ArrayNDFileStor : public ValueSeries<T>
{
public:

    inline bool		isOK() const;
    inline T		value( od_int64 pos ) const;
    inline void		setValue( od_int64 pos, T val );
    inline const T*	arr() const				{ return 0; }
    inline T*		arr()					{ return 0; }

    inline bool		setSize( od_int64 nsz );
    inline od_int64	size() const				{ return sz_; }

    inline		ArrayNDFileStor( od_int64 nsz );
    inline		ArrayNDFileStor();
    inline		~ArrayNDFileStor();

    inline void		setTempStorageDir( const char* dir );
private:

    inline void		open();
    inline void		close();

protected:

    std::fstream*		strm_;
    BufferString		name_;
    od_int64			sz_;
    bool			openfailed_;
    bool			streamfail_;
    mutable Threads::Mutex	mutex_;

};


template <class T> inline
bool ArrayNDFileStor<T>::isOK() const
{ return strm_; }


#define mArrNDChkStrm() \
    if ( strm_->fail() ) \
    { \
	const_cast<ArrayNDFileStor*>(this)->close(); \
	const_cast<ArrayNDFileStor*>(this)->streamfail_ = true; \
	return T(); \
    }

template <class T> inline
T ArrayNDFileStor<T>::value( od_int64 pos ) const
{
    Threads::MutexLocker mlock( mutex_ );
    if ( !strm_ ) const_cast<ArrayNDFileStor*>(this)->open();
    if ( !strm_ ) return T();

    strm_->seekg(pos*sizeof(T), std::ios::beg );
    mArrNDChkStrm()

    T res;
    strm_->read( (char *)&res, sizeof(T));
    mArrNDChkStrm()

    return res;
}

#undef mArrNDChkStrm
#define mArrNDChkStrm() \
    if ( strm_->fail() ) { close(); streamfail_ = true; return; }

template <class T> inline
void ArrayNDFileStor<T>::setValue( od_int64 pos, T val ) 
{
    Threads::MutexLocker mlock( mutex_ );
    if ( !strm_ ) open();
    if ( !strm_ ) return;

    strm_->seekp( pos*sizeof(T), std::ios::beg );
    mArrNDChkStrm()

    strm_->write( (const char *)&val, sizeof(T));
    mArrNDChkStrm()
}


template <class T> inline
bool ArrayNDFileStor<T>::setSize( od_int64 nsz )
{
    Threads::MutexLocker mlock( mutex_ );
    if ( strm_ ) close();
    sz_ = nsz;
    openfailed_ = streamfail_ = false;
    open();
    return strm_;
}


template <class T> inline
ArrayNDFileStor<T>::ArrayNDFileStor( od_int64 nsz )
    : sz_( nsz )
    , strm_( 0 )
    , name_(FilePath::getTempName("dat"))
    , openfailed_(false)
    , streamfail_(false)
{
    const char* stordir = GetEnvVar( "OD_ARRAY_TEMP_STORDIR" );
    if ( stordir )
	setTempStorageDir( stordir );
}


template <class T> inline
ArrayNDFileStor<T>::~ArrayNDFileStor()
{
    Threads::MutexLocker mlock( mutex_ );
    if ( strm_ ) close();
    File_remove( name_.buf(), mFile_NotRecursive );
}


template <class T> inline
void ArrayNDFileStor<T>::setTempStorageDir( const char* dir )
{
    close();
    File_remove( name_.buf(), mFile_NotRecursive );
    FilePath fp( name_.buf() );
    fp.setPath( File_isDirectory(dir) && File_isWritable(dir) ? dir : "/tmp/" );
    name_ = fp.fullPath();
}

#undef mArrNDChkStrm
#define mArrNDChkStrm() \
    if ( strm_->fail() ) { close(); openfailed_ = streamfail_ = true; return; }

template <class T> inline
void ArrayNDFileStor<T>::open()
{
    if ( strm_ ) close();
    else if ( openfailed_ || streamfail_ ) return;

    strm_ = new std::fstream( name_.buf(), std::fstream::binary
				     | std::fstream::out
				     | std::fstream::trunc );
    mArrNDChkStrm()

    char tmp[mArrNDChunkSz*sizeof(T)];
    memset( tmp, 0, mArrNDChunkSz*sizeof(T) );
    for ( int idx=0; idx<sz_; idx+=mArrNDChunkSz )
    {
	if ( (sz_-idx)/mArrNDChunkSz )
	    strm_->write( tmp, mArrNDChunkSz*sizeof(T) );
	else if ( sz_-idx )
	    strm_->write( tmp, (sz_-idx)*sizeof(T) );

	mArrNDChkStrm()
    }

    strm_->close();
    strm_->open( name_.buf(), std::fstream::binary
		    | std::fstream::out | std::fstream::in );
    mArrNDChkStrm()
}

template <class T> inline
void ArrayNDFileStor<T>::close()
{
    if ( strm_ ) strm_->close();
    delete strm_; strm_ = 0;
}


#endif
