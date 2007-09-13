#ifndef arrayndimpl_h
#define arrayndimpl_h
/*
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	K. Tingdahl
 Date:		9-3-1999
 RCS:		$Id: arrayndimpl.h,v 1.50 2007-09-13 19:38:38 cvsnanne Exp $
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

    bool	isOK() const { return strm_; }

#undef mChckStrm
#define mChckStrm \
    if ( strm_->fail() ) \
	{ mNonConstMem(close()); mNonConstMem(streamfail_) = true; return T();}

    T		value( od_int64 pos ) const
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

    void	setValue( od_int64 pos, T val ) 
		{
		    Threads::MutexLocker mlock( mutex_ );
		    if ( !strm_ ) open();
		    if ( !strm_ ) return;

		    strm_->seekp( pos*sizeof(T), std::ios::beg );
		    mChckStrm

		    strm_->write( (const char *)&val, sizeof(T));
		    mChckStrm
		}

    const T*	arr() const			{ return 0; }
    T*		arr()				{ return 0; }

    bool	setSize( od_int64 nsz )
		{
		    Threads::MutexLocker mlock( mutex_ );
		    if ( strm_ ) close();
		    sz_ = nsz;
		    openfailed_ = streamfail_ = false;
		    open();
		    return strm_;
		}

    od_int64	size() const { return sz_; }

		ArrayNDFileStor( od_int64 nsz )
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

    inline	~ArrayNDFileStor()
		{
		    Threads::MutexLocker mlock( mutex_ );
		    if ( strm_ ) close();
		    File_remove( name_, false );
		}

    void	setTempStorageDir( const char* dir )
		{
		    close();
		    File_remove( name_, false );
		    FilePath fp( name_ );
		    fp.setPath( File_isDirectory(dir) && File_isWritable(dir)
					? dir : "/tmp/" );
		    name_ = fp.fullPath();
		}

private:

#undef mChckStrm
#define mChckStrm \
    if ( strm_->fail() ) { close(); openfailed_ = streamfail_ = true; return; }

    void	open()
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

    void	close()
		{
		    if ( strm_ ) strm_->close();
		    delete strm_; strm_ = 0;
		}

protected:

    std::fstream*		strm_;
    BufferString		name_;
    od_int64			sz_;
    bool			openfailed_;
    bool			streamfail_;
    mutable Threads::Mutex	mutex_;

};


    // Don't change the order of these attributes!
#define mDeclArrayNDProtMemb(inftyp) \
    inftyp			in; \
    ValueSeries<T>*		stor_; \
 \
    const ValueSeries<T>*	getStorage_() const { return stor_; }

#define mImplSetStorage( _getsize ) \
{ \
    if ( !s->setSize(_getsize) ) \
	return false; \
    delete stor_; stor_=s;  \
    return true; \
}

template <class T> class Array1DImpl : public Array1D<T>
{
public:
			Array1DImpl(int nsz, bool file=false)
			    : in(nsz)
			    , stor_(file ? (ValueSeries<T>*)
					 new ArrayNDFileStor<T>(in.getTotalSz())
				        : (ValueSeries<T>*)
					 new ArrayValueSeries<T,T>(in.getTotalSz()))
			{}

			Array1DImpl(const Array1D<T>& templ)
			    : in(templ.info()) 
			    , stor_(new ArrayValueSeries<T,T>(in.getTotalSz()))
			{ copyFrom(templ); }
			//!Copy contructor
			Array1DImpl(const Array1DImpl<T>& templ)
			    : in(templ.info()) 
			    , stor_(new ArrayValueSeries<T,T>(in.getTotalSz()))
			{ copyFrom(templ); }
			~Array1DImpl() { delete stor_; }

    bool		canSetStorage() const { return true; }
    bool		setStorage(ValueSeries<T>* s)
			    mImplSetStorage( in.getTotalSz() );

    virtual void	set( int pos, T v ) { stor_->setValue(pos,v); }
    virtual T		get( int pos ) const {return stor_->value(pos); }
    void		copyFrom( const Array1D<T>& templ )
			{
			    if ( info()!=templ.info() )
				setInfo(templ.info());

			    const int nr = in.getTotalSz();

			    if ( templ.getData() )
				memcpy( this->getData(),
					templ.getData(),sizeof(T)*nr );
			    else
				for ( int idx=0; idx<nr; idx++ )
				    set(idx, templ.get(idx));
			}
			
    const mPolyArray1DInfoTp&  info() const		{ return in; }
    bool		canSetInfo() { return true; }
    bool		setInfo( const ArrayNDInfo& ni )
			{
			    if ( ni.getNDim() != 1 ) return false; 
			    return setSize( ni.getSize(0) );
			}


    bool		setSize( int s )
    			{
			    in.setSize(0,s);
			    return stor_->setSize(s);
			}

			// ValueSeries interface
    T*			arr()			{ return stor_->arr(); }
    const T*		arr() const		{ return stor_->arr(); }

protected:

    mDeclArrayNDProtMemb(Array1DInfoImpl)

};


template <class T> class Array2DImpl : public Array2D<T>
{
public:
			Array2DImpl(int sz0, int sz1, bool file=false)
			    : in(sz0,sz1)
			    , stor_(file ? (ValueSeries<T>*)
					 new ArrayNDFileStor<T>(in.getTotalSz())
				        : (ValueSeries<T>*)
					 new ArrayValueSeries<T,T>(in.getTotalSz()))
			{}
			Array2DImpl( const Array2DInfo& nsz, bool file=false )
			    : in( nsz )
			    , stor_(file ? (ValueSeries<T>*)
					 new ArrayNDFileStor<T>(in.getTotalSz())
				        : (ValueSeries<T>*)
					 new ArrayValueSeries<T,T>(in.getTotalSz()))
			{} 
			Array2DImpl( const Array2D<T>& templ )
			    : in( (const Array2DInfo&)templ.info() )
			    , stor_( new ArrayValueSeries<T,T>(in.getTotalSz()))
			{
			    copyFrom(templ);
			}

			//!Copy contructor
			Array2DImpl( const Array2DImpl<T>& templ )
			    : in( (const Array2DInfo&)templ.info() )
			    , stor_( new ArrayValueSeries<T,T>(in.getTotalSz()))
			{
			    copyFrom(templ);
			}

			~Array2DImpl() { delete stor_; }

    bool		canSetStorage() const { return true; }
    bool		setStorage(ValueSeries<T>* s)
			    mImplSetStorage( in.getTotalSz() );

    virtual void	set( int p0, int p1, T v )
			{ stor_->setValue(in.getOffset(p0,p1), v); }
    virtual T		get( int p0, int p1 ) const
			{ return stor_->value(in.getOffset(p0,p1)); }
    void		copyFrom( const Array2D<T>& templ )
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

    const mPolyArray2DInfoTp&  info() const { return in; }

    bool		canSetInfo() { return true; }

    bool		setInfo( const ArrayNDInfo& ni )
			{
			    if ( ni.getNDim() != 2 ) return false; 
			    return setSize( ni.getSize(0), ni.getSize(1) );
			}

    bool		setSize( int d0, int d1 )
			{
			    in.setSize(0,d0);
			    in.setSize(1,d1);
			    return stor_->setSize( in.getTotalSz() );
			}

protected:

    mDeclArrayNDProtMemb(Array2DInfoImpl)

};


template <class T> class Array3DImpl : public Array3D<T>
{
public:
			Array3DImpl( int sz0, int sz1, int sz2, bool file=false)
			    : in(sz0,sz1,sz2)
			    , stor_(file ? (ValueSeries<T>*)
					 new ArrayNDFileStor<T>(in.getTotalSz())
					: (ValueSeries<T>*)
					 new ArrayValueSeries<T,T>(in.getTotalSz()))
			{}
			Array3DImpl( const Array3DInfo& nsz, bool file=false )
			    : in(nsz)
			    , stor_(file ? (ValueSeries<T>*)
					 new ArrayNDFileStor<T>(in.getTotalSz())
					: (ValueSeries<T>*)
					 new ArrayValueSeries<T,T>(in.getTotalSz()))
			{}
			Array3DImpl( const Array3D<T>& templ )
			    : in( templ.info() )
			    , stor_( new ArrayValueSeries<T,T>(in.getTotalSz()))
			{ copyFrom(templ); }
			//!Copy contructor
			Array3DImpl( const Array3DImpl<T>& templ )
			    : in( templ.info() )
			    , stor_( new ArrayValueSeries<T,T>(in.getTotalSz()))
			{ copyFrom(templ); }

			~Array3DImpl() { delete stor_; }

    bool		canSetStorage() const { return true; }
    bool		setStorage(ValueSeries<T>* s)
			    mImplSetStorage( in.getTotalSz() );

    virtual void	set( int p0, int p1, int p2, T v )
			{ stor_->setValue(in.getOffset(p0,p1,p2), v); }
    virtual T		get( int p0, int p1, int p2 ) const
			{ return stor_->value(in.getOffset(p0,p1,p2)); }
    void		copyFrom( const Array3D<T>& templ )
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
				int sz2 = in.getSize(2);

				for ( int id0=0; id0<sz0; id0++ )
				{
				    for ( int id1=0; id1<sz1; id1++ )
				    {
					for ( int id2=0; id2<sz2; id2++ )
					    set(id0,id1,id2,
						templ.get(id0,id1,id2));
				    }
				}
			    }
			}

    const mPolyArray3DInfoTp&	info() const { return in; }
    bool		canSetInfo() { return true; }
    bool		setInfo( const ArrayNDInfo& ni )
			{
			    if ( ni.getNDim() != 3 ) return false; 
			    return setSize( ni.getSize(0), ni.getSize(1),
					    ni.getSize(2) );
			}


    bool		setSize( int d0, int d1, int d2 )
			{
			    in.setSize(0,d0);
			    in.setSize(1,d1);
			    in.setSize(2,d2);
			    return stor_->setSize( in.getTotalSz() );
			}
protected:

    mDeclArrayNDProtMemb(Array3DInfoImpl)

};


template <class T> class ArrayNDImpl : public ArrayND<T>
{
public:
static ArrayND<T>*	create(const ArrayNDInfo& nsz,bool file=false);

			ArrayNDImpl(const ArrayNDInfo& nsz,bool file=false)
			    : in(nsz.clone())
			    , stor_(file ? (ValueSeries<T>*)
				     new ArrayNDFileStor<T>(in->getTotalSz())
				    : (ValueSeries<T>*)
				     new ArrayValueSeries<T,T>(in->getTotalSz()))
			    {}
			ArrayNDImpl(const ArrayND<T>& templ,bool file=false)
			    : in(templ.info().clone())
			    , stor_(file ? (ValueSeries<T>*)
				     new ArrayNDFileStor<T>(in->getTotalSz())
				    : (ValueSeries<T>*)
				     new ArrayValueSeries<T,T>(in->getTotalSz()))
			{
			    if ( templ.getData() )
			    {
				int nr = in->getTotalSz();
				memcpy( this->getData(),
					templ.getData(),sizeof(T)*nr );
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
			//!Copy contructor
			ArrayNDImpl(const ArrayNDImpl<T>& templ,bool file=false)
			    : in(templ.info().clone())
			    , stor_(file ? (ValueSeries<T>*)
				     new ArrayNDFileStor<T>(in->getTotalSz())
				    : (ValueSeries<T>*)
				     new ArrayValueSeries<T,T>(in->getTotalSz()))
			{
			    if ( templ.getData() )
			    {
				int nr = in->getTotalSz();
				memcpy( this->getData(),
					templ.getData(),sizeof(T)*nr );
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

			~ArrayNDImpl()
			{ delete in; delete stor_; }

    bool		canSetStorage() const { return true; }
    bool		setStorage(ValueSeries<T>* s)
			    mImplSetStorage( in->getTotalSz() );

    virtual void	set( const int* pos, T v )
			{ stor_->setValue(in->getOffset(pos), v); }
    virtual T		get( const int* pos ) const
			{ return stor_->value(in->getOffset(pos)); }


    const ArrayNDInfo&	info() const { return *in; }
    bool		canSetInfo() { return true; }
    bool		canChangeNrDims() const { return true; }
    bool		setInfo( const ArrayNDInfo& ni )
			{
			    int ndim = in->getNDim();
			    for ( int idx=0; idx<ndim; idx++ )
				{ in->setSize( idx, ni.getSize(idx) ); }
			    return stor_->setSize( in->getTotalSz() );
			}
 
    bool		setSize( const int* d )
			{
			    const int ndim = in->getNDim();
			    for ( int idx=0; idx<ndim; idx++ )
				in->setSize( idx, d[idx] );
			    return stor_->setSize( in->getTotalSz() );
			}

protected:

    mDeclArrayNDProtMemb(ArrayNDInfo*)

}; 


/*!\brief No stored data just plane parameters */

template <class T> class ArrayNDPlaneImpl : public ArrayND<T>
{
public:
			ArrayNDPlaneImpl( const ArrayNDInfo& nsz )
			    : in(nsz.clone())
			    , a0(0)
			    , an(nsz.getNDim(),0)	{}
			ArrayNDPlaneImpl( const ArrayNDPlaneImpl& anpi )
			    : in(anpi.in->clone())
			    , a0(anpi.a0)
			    , an(anpi.an)		{}
     ArrayNDPlaneImpl<T>& operator =( const ArrayNDPlaneImpl& anpi )
	 		{
			    delete in; in = anpi.in->clone();
			    a0 = anpi.a0; an = anpi.an;
			}
			~ArrayNDPlaneImpl()		{ delete in; }

    void		setA0( T x )			{ a0 = x; }
    void		setAn( int idx, T x )		{ an[idx] = x; }

    virtual void	set(const int*,T)		{}
    virtual T		get(const int*) const;

    const ArrayNDInfo&	info() const			{ return *in; }
    bool		canSetInfo()			{ return true; }
    bool		canChangeNrDims() const		{ return false; }
    bool		setInfo( const ArrayNDInfo& ni )
			{
			    *this = ArrayNDPlaneImpl(ni);
			    return true;
			}
 
    void		setSize( const int* d )
			{
			    const int ndim = in->getNDim();
			    for ( int idx=0; idx<ndim; idx++ )
				in->setSize(idx,d[idx]);
			}

protected:

    T			a0;
    TypeSet<T>		an;

    mDeclArrayNDProtMemb(ArrayNDInfo*)

}; 

template <class T>
inline T ArrayNDPlaneImpl<T>::get( const int* pos ) const
{
    const int ndim = in->getNDim();
    T res = a0;
    for ( int idx=0; idx<ndim; idx++ )
	res += an[idx] * pos[idx];

    return res;
}


/*!\brief No stored data just 3 plane parameters */

template <class T> class Array2DPlaneImpl : public Array2D<T>
{
public:
			Array2DPlaneImpl( const Array2DInfo& nsz )
			    : in(nsz.clone())
			    , a0(0)
			    , a1(0)
			    , a2(0)			{}
			Array2DPlaneImpl( int n1, int n2, T x0, T x1, T x2 )
			    : in(new Array2DInfoImpl(n1,n2))
			    , a0(x0)
			    , a1(x1)
			    , a2(x2)			{}
			Array2DPlaneImpl( const Array2DPlaneImpl& anpi )
			    : in(anpi.in->clone())
			    , a0(anpi.a0)
			    , a1(anpi.a1)
			    , a2(anpi.a2)		{}
     Array2DPlaneImpl<T>& operator =( const Array2DPlaneImpl& anpi )
	 		{
			    delete in; in = anpi.in->clone();
			    a0 = anpi.a0; a1 = anpi.a1; a2 = anpi.a2;
			}
			~Array2DPlaneImpl()		{ delete in; }

    void		setA0( T x )			{ a0 = x; }
    void		setA1( T x )			{ a1 = x; }
    void		setA2( T x )			{ a2 = x; }

    virtual T		get( const int* i ) const
			{ return get( i[0], i[1] ); }
    virtual T		get( int i1, int i2 ) const
			{ return a0 + a1 * i1 + a2 * i2; }
    virtual void	set(const int*,T)		{}
    virtual void	set(int,int,T)			{}

    const Array2DInfo&	info() const			{ return *in; }
    bool		canSetInfo()			{ return true; }
    bool		canChangeNrDims() const		{ return false; }
    bool		setInfo( const ArrayNDInfo& ni )
			{
			    if ( ni.getNDim() != 2 ) return false; 
			    in->setSize( 0, ni.getSize(0) );
			    in->setSize( 1, ni.getSize(1) );
			    return true;
			}
 
    void		setSize( const int* d )
    			{
			    in->setSize( 0, d[0] );
			    in->setSize( 1, d[1] );
			}

protected:

    T			a0;
    T			a1;
    T			a2;

    mDeclArrayNDProtMemb(Array2DInfo*)

}; 


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

#endif
