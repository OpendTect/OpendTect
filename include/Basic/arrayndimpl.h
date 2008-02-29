#ifndef arrayndimpl_h
#define arrayndimpl_h
/*
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	K. Tingdahl
 Date:		9-3-1999
 RCS:		$Id: arrayndimpl.h,v 1.56 2008-02-29 21:45:38 cvskris Exp $
________________________________________________________________________

*/


#include "arraynd.h"
#include "bufstring.h"
#include "envvars.h"
#include "filegen.h"
#include "filepath.h"
#include "thread.h"

#include <fstream>


#define mChunkSz 1024
#define mNonConstMem(x) const_cast<ArrayNDFileStor*>(this)->x

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


#define mDeclArrayNDProtMemb(inftyp) \
    inftyp			in; \
    ValueSeries<T>*		stor_; \
 \
    const ValueSeries<T>*	getStorage_() const { return stor_; }

template <class T> class Array1DImpl : public Array1D<T>
{
public:
    inline			Array1DImpl(int sz, bool file=false);

    inline			Array1DImpl(const Array1D<T>&);
    inline			Array1DImpl(const Array1DImpl<T>&);
    inline			~Array1DImpl();

    inline void			copyFrom(const Array1D<T>&);
    inline bool			setStorage(ValueSeries<T>*);
    inline bool			canSetStorage() const		{ return true; }

    inline void			set(int pos,T);
    inline T			get(int pos) const;
			
    inline const Array1DInfo&	info() const			{ return in; }
    inline bool			canSetInfo()			{ return true; }
    inline bool			setInfo(const ArrayNDInfo&);
    inline bool			setSize(int);

				// ValueSeries interface
    inline T*			arr()			{ return ptr_; }
    inline const T*		arr() const		{ return ptr_; }

protected:
    inline const T*		getData_() const	{ return ptr_; }
    T*				ptr_;	//not owned, only a shortcut
    					//for the 99% percent case

    mDeclArrayNDProtMemb(Array1DInfoImpl);

};


template <class T> class Array2DImpl : public Array2D<T>
{
public:
			Array2DImpl(int sz0,int sz1,bool file=false);
			Array2DImpl(const Array2DInfo&, bool file=false );
			Array2DImpl(const Array2D<T>&);
			Array2DImpl(const Array2DImpl<T>&);
			~Array2DImpl();

    bool		canSetStorage() const			{ return true; }
    bool		setStorage(ValueSeries<T>*);

    virtual void	set(int,int,T);
    virtual T		get(int,int) const;
    void		copyFrom(const Array2D<T>&);

    const Array2DInfo&	info() const;

    bool		canSetInfo();

    bool		setInfo(const ArrayNDInfo&);
    bool		setSize(int,int);

protected:

    inline const T*		getData_() const	{ return ptr_; }
    T*				ptr_;	//not owned, only a shortcut
    					//for the 99% percent case

    mDeclArrayNDProtMemb(Array2DInfoImpl);

};


template <class T> class Array3DImpl : public Array3D<T>
{
public:
    inline		Array3DImpl(int sz0,int sz1,int sz2,bool file=false);
    inline		Array3DImpl(const Array3DInfo&,bool file=false);
    inline		Array3DImpl(const Array3D<T>&);
    inline		Array3DImpl(const Array3DImpl<T>&);

    inline			~Array3DImpl();

    inline bool			canSetStorage() const		{ return true; }
    inline bool			setStorage(ValueSeries<T>*);

    inline void			set(int,int,int,T);
    inline T			get(int,int,int) const;
    inline void			copyFrom(const Array3D<T>&);

    inline const Array3DInfo&	info() const;
    inline bool			canSetInfo();
    inline bool			setInfo(const ArrayNDInfo&);
    inline bool			setSize(int,int,int);

protected:
    inline const T*		getData_() const	{ return ptr_; }
    T*				ptr_;	//not owned, only a shortcut
    					//for the 99% percent case

    mDeclArrayNDProtMemb(Array3DInfoImpl);
};


template <class T> class ArrayNDImpl : public ArrayND<T>
{
public:
    static ArrayND<T>*		create(const ArrayNDInfo& nsz,bool file=false);

    inline			ArrayNDImpl(const ArrayNDInfo&,bool file=false);
    inline			ArrayNDImpl(const ArrayND<T>&,bool file=false);
    inline			ArrayNDImpl(const ArrayNDImpl<T>&,bool f=false);
    inline			~ArrayNDImpl();

    inline bool			canSetStorage() const		{ return true; }
    inline bool			setStorage(ValueSeries<T>*);

    inline void			set(const int*,T);
    inline T			get(const int*) const;

    inline const ArrayNDInfo&	info() const;
    inline bool			canSetInfo();
    inline bool			canChangeNrDims() const;
    inline bool			setInfo(const ArrayNDInfo&);
 
    inline bool			setSize(const int*);
    inline void			copyFrom(const ArrayND<T>&);

protected:
    inline const T*		getData_() const	{ return ptr_; }
    T*				ptr_;	//not owned, only a shortcut
    					//for the 99% percent case

    mDeclArrayNDProtMemb(ArrayNDInfo*)

}; 


//Implementations


template <class T> inline
bool ArrayNDFileStor<T>::isOK() const
{ return strm_; }

#undef mChckStrm
#define mChckStrm \
    if ( strm_->fail() ) \
	{ mNonConstMem(close()); mNonConstMem(streamfail_) = true; return T();}

template <class T> inline
T ArrayNDFileStor<T>::value( od_int64 pos ) const
{
    Threads::MutexLocker mlock( mutex_ );
    if ( !strm_ ) const_cast<ArrayNDFileStor*>(this)->open();
    if ( !strm_ ) return T();

    strm_->seekg(pos*sizeof(T), std::ios::beg );
    mChckStrm

    T res;
    strm_->read( (char *)&res, sizeof(T));
    mChckStrm

    return res;
}

#undef mChckStrm
#define mChckStrm \
    if ( strm_->fail() ) { close(); streamfail_ = true; return; }

template <class T> inline
void ArrayNDFileStor<T>::setValue( od_int64 pos, T val ) 
{
    Threads::MutexLocker mlock( mutex_ );
    if ( !strm_ ) open();
    if ( !strm_ ) return;

    strm_->seekp( pos*sizeof(T), std::ios::beg );
    mChckStrm

    strm_->write( (const char *)&val, sizeof(T));
    mChckStrm
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
    File_remove( name_, false );
}


template <class T> inline
void ArrayNDFileStor<T>::setTempStorageDir( const char* dir )
{
    close();
    File_remove( name_, false );
    FilePath fp( name_ );
    fp.setPath( File_isDirectory(dir) && File_isWritable(dir) ? dir : "/tmp/" );
    name_ = fp.fullPath();
}

#undef mChckStrm
#define mChckStrm \
    if ( strm_->fail() ) { close(); openfailed_ = streamfail_ = true; return; }

template <class T> inline
void ArrayNDFileStor<T>::open()
{
    if ( strm_ ) close();
    else if ( openfailed_ || streamfail_ ) return;

    strm_ = new std::fstream( name_, std::fstream::binary
				     | std::fstream::out
				     | std::fstream::trunc );
    mChckStrm

    char tmp[mChunkSz*sizeof(T)];
    memset( tmp, 0, mChunkSz*sizeof(T) );
    for ( int idx=0; idx<sz_; idx+=mChunkSz )
    {
	if ( (sz_-idx)/mChunkSz )
	    strm_->write( tmp, mChunkSz*sizeof(T) );
	else if ( sz_-idx )
	    strm_->write( tmp, (sz_-idx)*sizeof(T) );

	mChckStrm
    }

    strm_->close();
    strm_->open( name_, std::fstream::binary
		    | std::fstream::out
		    | std::fstream::in );
    mChckStrm
}
#undef mChckStrm

template <class T> inline
void ArrayNDFileStor<T>::close()
{
    if ( strm_ ) strm_->close();
    delete strm_; strm_ = 0;
}


#define mImplFileConstructor \
    , stor_( 0 ) \
    , ptr_( 0 ) \
{ \
    setStorage( file \
	? (ValueSeries<T>*) new ArrayNDFileStor<T>(info().getTotalSz()) \
	: (ValueSeries<T>*) new MultiArrayValueSeries<T,T>(info().getTotalSz())); \
}


#define mImplCopyConstructor( clss, from ) \
template <class T> inline \
clss<T>::clss(const from<T>& templ) \
    : in(templ.info())  \
    , stor_( 0 ) \
    , ptr_( 0 ) \
{ \
    setStorage( new MultiArrayValueSeries<T,T>(info().getTotalSz()) ); \
    copyFrom(templ); \
} 


#define mImplDestructor( clss ) \
template <class T> inline \
clss<T>::~clss() { delete stor_; }


#define mImplSetStorage( clss ) \
template <class T> inline \
bool clss<T>::setStorage(ValueSeries<T>* s) \
{ \
    ptr_ = 0; \
    if ( !s->setSize(info().getTotalSz()) ) \
    { \
	delete s; \
	return false; \
    } \
    delete stor_; stor_=s; ptr_ = stor_->arr(); \
    return true; \
}


template <class T> inline
Array1DImpl<T>::Array1DImpl(int nsz, bool file )
    : in(nsz)
      mImplFileConstructor;

mImplCopyConstructor( Array1DImpl, Array1D );
mImplCopyConstructor( Array1DImpl, Array1DImpl );
mImplDestructor( Array1DImpl );
mImplSetStorage( Array1DImpl );

template <class T> inline
void Array1DImpl<T>::set( int pos, T v )	
{
    if ( ptr_ ) ptr_[pos] = v;
    else stor_->setValue(pos,v);
}


template <class T> inline
T Array1DImpl<T>::get( int pos ) const
{ return ptr_ ? ptr_[pos] : stor_->value(pos); }


template <class T> inline
void Array1DImpl<T>::copyFrom( const Array1D<T>& templ )
{
    if ( info()!=templ.info() )
	setInfo( templ.info() );

    const int nr = in.getTotalSz();

    if ( templ.getData() )
	memcpy( this->getData(), templ.getData(),sizeof(T)*nr );
    else
    {
	for ( int idx=0; idx<nr; idx++ )
	    set(idx, templ.get(idx));
    }
}


template <class T> inline
bool Array1DImpl<T>::setInfo( const ArrayNDInfo& ni )
{
    if ( ni.getNDim() != 1 ) return false; 
    return setSize( ni.getSize(0) );
}


template <class T> inline
bool Array1DImpl<T>::setSize( int s )
{
    in.setSize(0,s);
    if ( !stor_->setSize(s) )
    {
	ptr_ = 0;
	return false;
    }

    ptr_ = stor_->arr();
    return true;
}


template <class T> inline
Array2DImpl<T>::Array2DImpl( int sz0, int sz1, bool file )
    : in(sz0,sz1)
mImplFileConstructor;


template <class T> inline
Array2DImpl<T>::Array2DImpl( const Array2DInfo& nsz, bool file )
    : in( nsz )
mImplFileConstructor;


mImplCopyConstructor( Array2DImpl, Array2D );
mImplCopyConstructor( Array2DImpl, Array2DImpl );
mImplDestructor( Array2DImpl );
mImplSetStorage( Array2DImpl );


template <class T> inline
void Array2DImpl<T>::set( int p0, int p1, T v )
{
    const od_int64 offset = in.getOffset( p0, p1 );
    if ( ptr_ ) ptr_[offset] = v;
    else stor_->setValue( offset, v );
}


template <class T> inline
T Array2DImpl<T>::get( int p0, int p1 ) const
{
    const od_int64 offset = in.getOffset( p0, p1);
    return ptr_ ? ptr_[offset] : stor_->value( offset );
}


template <class T> inline
void Array2DImpl<T>::copyFrom( const Array2D<T>& templ )
{
    if ( info()!=templ.info() )
	setInfo(templ.info());

    if ( templ.getData() )
    {
	const int nr = in.getTotalSz();
	memcpy( this->getData(),
		templ.getData(),sizeof(T)*nr );
    }
    else
    {
	int sz0 = in.getSize(0);
	int sz1 = in.getSize(1);

	for ( int id0=0; id0<sz0; id0++ )
	{
	    for ( int id1=0; id1<sz1; id1++ )
		set(id0,id1,templ.get(id0,id1));
	}
    }
}


template <class T> inline
const Array2DInfo& Array2DImpl<T>::info() const
{ return in; }


template <class T> inline
bool Array2DImpl<T>::canSetInfo()
{ return true; }


template <class T> inline
bool Array2DImpl<T>::setInfo( const ArrayNDInfo& ni )
{
    if ( ni.getNDim() != 2 ) return false; 
    return setSize( ni.getSize(0), ni.getSize(1) );
}


template <class T> inline
bool Array2DImpl<T>::setSize( int d0, int d1 )
{
    in.setSize(0,d0);
    in.setSize(1,d1);
    if ( !stor_->setSize( in.getTotalSz() ) )
    {
	ptr_ = 0;
	return false;
    }

    ptr_ = stor_->arr();
    return true;
}


template <class T> inline
Array3DImpl<T>::Array3DImpl( int sz0, int sz1, int sz2, bool file)
    : in(sz0,sz1,sz2)
mImplFileConstructor;


template <class T> inline
Array3DImpl<T>::Array3DImpl( const Array3DInfo& nsz, bool file )
    : in(nsz)
mImplFileConstructor;

mImplCopyConstructor( Array3DImpl, Array3D );
mImplCopyConstructor( Array3DImpl, Array3DImpl );
mImplDestructor( Array3DImpl );
mImplSetStorage( Array3DImpl );

template <class T> inline
void Array3DImpl<T>::set( int p0, int p1, int p2, T v )
{
    const od_int64 offset = in.getOffset( p0, p1, p2 );
    if ( ptr_ ) ptr_[offset] = v;
    else stor_->setValue( offset, v );
}



template <class T> inline
T Array3DImpl<T>::get( int p0, int p1, int p2 ) const
{
    const od_int64 offset = in.getOffset( p0, p1, p2 );
    return ptr_ ? ptr_[offset] : stor_->value( offset );
}


template <class T> inline
void Array3DImpl<T>::copyFrom( const Array3D<T>& templ )
{
    if ( info()!=templ.info() )
	setInfo(templ.info());

    if ( templ.getData() )
    {
	const int nr = in.getTotalSz();
	memcpy( this->getData(), templ.getData(),sizeof(T)*nr );
    }
    else
    {
	int sz0 = in.getSize(0);
	int sz1 = in.getSize(1);
	int sz2 = in.getSize(2);

	for ( int id0=0; id0<sz0; id0++ )
	{
	    for ( int id1=0; id1<sz1; id1++ )
	    {
		for ( int id2=0; id2<sz2; id2++ )
		    set(id0,id1,id2, templ.get(id0,id1,id2));
	    }
	}
    }
}


template <class T> inline
const Array3DInfo& Array3DImpl<T>::info() const
{ return in; }


template <class T> inline
bool Array3DImpl<T>::canSetInfo()
{ return true; }


template <class T> inline
bool Array3DImpl<T>::setInfo( const ArrayNDInfo& ni )
{
    if ( ni.getNDim() != 3 ) return false; 
    return setSize( ni.getSize(0), ni.getSize(1), ni.getSize(2) );
}


template <class T> inline
bool Array3DImpl<T>::setSize( int d0, int d1, int d2 )
{
    in.setSize(0,d0);
    in.setSize(1,d1);
    in.setSize(2,d2);
    if ( !stor_->setSize( in.getTotalSz() ) )
    {
	ptr_ = 0;
	return false;
    }

    ptr_ = stor_->arr();
    return true;
}


template <class T> inline
ArrayNDImpl<T>::ArrayNDImpl( const ArrayNDInfo& nsz, bool file )
    : in( nsz.clone() )
mImplFileConstructor;


template <class T> inline
ArrayNDImpl<T>::ArrayNDImpl( const ArrayND<T>& templ, bool file )
    : in(templ.info().clone())
mImplFileConstructor;


//mImplCopyConstructor( ArrayNDImpl, ArrayND );
//mImplCopyConstructor( ArrayNDImpl, ArrayNDImpl );
mImplDestructor( ArrayNDImpl );
mImplSetStorage( ArrayNDImpl );

template <class T> inline
void ArrayNDImpl<T>::copyFrom( const ArrayND<T>& templ )
{
    if ( info()!=templ.info() )
    {
	delete in;
	in = templ.info().clone();
    }

    if ( templ.getData() )
    {
	const int nr = in->getTotalSz();
	memcpy( this->getData(), templ.getData(),sizeof(T)*nr );
    }
    else
    {
	ArrayNDIter iter( *in );

	do
	{
	    set(iter.getPos(), templ.get(iter.getPos));
	} while ( this->next() );
    }
}


template <class T> inline
void ArrayNDImpl<T>::set( const int* pos, T v )
{
    const od_int64 offset = in->getOffset(pos);
    if ( ptr_ ) ptr_[offset] = v ;
    else stor_->setValue( offset, v);
}


template <class T> inline
T ArrayNDImpl<T>::get( const int* pos ) const
{
    const od_int64 offset = in->getOffset(pos);
    return ptr_ ? ptr_[offset] : stor_->value( offset );
}


template <class T> inline
const ArrayNDInfo& ArrayNDImpl<T>::info() const
{ return *in; }


template <class T> inline
bool ArrayNDImpl<T>::canSetInfo()
{ return true; }


template <class T> inline
bool ArrayNDImpl<T>::canChangeNrDims() const
{ return true; }


template <class T> inline
bool ArrayNDImpl<T>::setInfo( const ArrayNDInfo& ni )
{
    if ( ni.getNDim()!=in->getNDim() )
	return false;

    const int ndim = in->getNDim();
    TypeSet<int> sizes( ndim, 0 );
    for ( int idx=0; idx<ndim; idx++ )
	sizes[idx] = ni.getSize(idx);

    return setSize( sizes.arr() );
}

 
template <class T> inline
bool ArrayNDImpl<T>::setSize( const int* d )
{
    const int ndim = in->getNDim();
    for ( int idx=0; idx<ndim; idx++ )
	in->setSize( idx, d[idx] );

    if ( stor_->setSize( in->getTotalSz() ) )
    {
	ptr_ = 0;
	return false;
    }

    ptr_ = stor_->arr();
    return true;
}


template <class T> inline
ArrayND<T>* ArrayNDImpl<T>::create( const ArrayNDInfo& nsz, bool file )
{
    int ndim = nsz.getNDim();

    if ( ndim==1 ) return new Array1DImpl<T>( nsz.getSize(0), file);
    if ( ndim==2 ) return new Array2DImpl<T>( nsz.getSize(0),nsz.getSize(1),
					      file);
    if ( ndim==3 ) return new Array3DImpl<T>( nsz.getSize(0),
						 nsz.getSize(1),
						 nsz.getSize(2), file);

    return new ArrayNDImpl<T>( nsz, file );
}

#undef mDeclArrayNDProtMemb
#undef mImplSetStorage
#undef mImplFileConstructor
#undef mImplCopyConstructor
#undef mImplDestructor

#endif
