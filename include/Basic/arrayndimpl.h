#ifndef arrayndimpl_h
#define arrayndimpl_h
/*
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	K. Tingdahl
 Date:		9-3-1999
 RCS:		$Id: arrayndimpl.h,v 1.27 2004-04-01 13:39:50 bert Exp $
________________________________________________________________________

*/


#include "arraynd.h"
#include "bufstring.h"
#include "filegen.h"
#include "filepath.h"

#include <fstream>

template <class T>
class ArrayNDMemStor : public ArrayND<T>::LinearStorage
{
public:

    bool	isOK() const			{ return ptr; }
    T		get(int pos) const		{ return ptr[pos]; }
    void	set(int pos, T val )		{ ptr[pos] = val; }

    const T*	getData() const			{ return ptr; }

    void	setSize( int nsz )		{ if ( sz == nsz ) return;
						  sz = nsz; alloc(); }
    int		size() const			{ return sz; }

		ArrayNDMemStor( int nsz )
		    : ptr (0), sz(nsz)		{ alloc(); }
    inline	~ArrayNDMemStor()		{ delete [] ptr; ptr = 0; }

protected:

    int		sz;
    T*		ptr;

    void	alloc()			{ delete [] ptr; ptr = new T [sz]; }

};


#define mChunkSz 1024
#define mNonConstMem(x) const_cast<ArrayNDFileStor*>(this)->x

template <class T>
class ArrayNDFileStor : public ArrayND<T>::LinearStorage
{
public:

    bool	isOK() const { return strm; }

#undef mChckStrm
#define mChckStrm \
    if ( strm->fail() ) \
	{ mNonConstMem(close()); mNonConstMem(stream_fail) = true; return T(0);}

    T		get( int pos ) const
		{
		    if ( !strm ) const_cast<ArrayNDFileStor*>(this)->open();
		    if ( !strm ) return T(0);

		    strm->seekg(pos*sizeof(T), ios::beg );
		    mChckStrm

		    T res;
		    strm->read( (char *)&res, sizeof(T));
		    mChckStrm

		    return res;
		}

#undef mChckStrm
#define mChckStrm \
    if ( strm->fail() ) { close(); stream_fail = true; return; }

    void	set( int pos, T val ) 
		{
		    if ( !strm ) open();
		    if ( !strm ) return;

		    strm->seekp( pos*sizeof(T), ios::beg );
		    mChckStrm

		    strm->write( (const char *)&val, sizeof(T));
		    mChckStrm
		}

    const T*	getData() const { return 0; }

    void	setSize( int nsz )
		{
		    if ( strm ) close();
		    sz = nsz;
		    open_failed = stream_fail = false;
		    open();
		}

    int		size() const { return sz; }

		ArrayNDFileStor( int nsz )
		    : sz( nsz )
		    , strm( 0 )
		    , name(FilePath::getTempName("dat"))
		    , open_failed(false)
		    , stream_fail(false)
		{ }

    inline	~ArrayNDFileStor()
		{
		    close();
		    File_remove( name, NO );
		}
private:

#undef mChckStrm
#define mChckStrm \
    if ( strm->fail() ) { close(); open_failed = stream_fail = true; return; }

    void	open()
		{
		    if ( strm ) close();
		    else if ( open_failed || stream_fail ) return;

		    strm = new fstream( name, fstream::binary
					    | fstream::out
					    | fstream::app
					    | fstream::trunc );
		    mChckStrm

		    char tmp[mChunkSz*sizeof(T)];
		    memset( tmp, 0, mChunkSz*sizeof(T) );
		    for ( int idx=0; idx<sz; idx+=mChunkSz )
		    {
			if ( (sz-idx)/mChunkSz )
			    strm->write( tmp, mChunkSz*sizeof(T) );
			else if ( sz-idx )
			    strm->write( tmp, (sz-idx)*sizeof(T) );

			mChckStrm
		    }

		    strm->close();
		    strm->open( name, fstream::binary
				    | fstream::out
				    | fstream::in );
		    mChckStrm
		}
#undef mChckStrm

    void	close()
		{
		    strm->close(); delete strm; strm = 0;
		}

protected:

    fstream*	strm;
    BufferString name;
    int		sz;
    bool	open_failed;
    bool	stream_fail;

};


    // Don't change the order of these attributes!
#define mDeclArrayNDProtMemb(inftyp) \
    inftyp			in; \
    ArrayNDLinearStorage*	stor; \
 \
    const ArrayNDLinearStorage*	getStorage_() const { return stor; }


template <class T> class Array1DImpl : public Array1D<T>
{
public:
			Array1DImpl(int nsz, bool file=false)
			    : in(nsz)
			    , stor(file ? (ArrayNDLinearStorage*)
					 new ArrayNDFileStor<T>(in.getTotalSz())
				        : (ArrayNDLinearStorage*)
					 new ArrayNDMemStor<T>(in.getTotalSz()))
			{}

			Array1DImpl(const Array1D<T>& templ)
			    : in(templ.info()) 
			    , stor(new ArrayNDMemStor<T>(in.getTotalSz()))
			{
			    int nr = in.getTotalSz();

			    if ( templ.getData() )
				memcpy( getData(),templ.getData(),sizeof(T)*nr);
			    else
				for ( int idx=0; idx<nr; idx++ )
				    set(idx, templ.get(idx));
			}
			~Array1DImpl() { delete stor; }

    virtual void	set( int pos, T v ) { stor->set(pos,v); }
    virtual T		get( int pos ) const {return stor->get(pos); }
			
    const mPolyArray1DInfoTp&  info() const		{ return in; }
    bool		canSetInfo() { return true; }
    bool		setInfo( const ArrayNDInfo& ni )
			{
			    if ( ni.getNDim() != 1 ) return false; 
			    setSize( ni.getSize( 0 ) );
			    return true;
			}


    void		setSize( int s ) { in.setSize(0,s); stor->setSize(s); }

protected:

    mDeclArrayNDProtMemb(Array1DInfoImpl)

};


template <class T> class Array2DImpl : public Array2D<T>
{
public:
			Array2DImpl(int sz0, int sz1, bool file=false)
			    : in(sz0,sz1)
			    , stor(file ? (ArrayNDLinearStorage*)
					 new ArrayNDFileStor<T>(in.getTotalSz())
				        : (ArrayNDLinearStorage*)
					 new ArrayNDMemStor<T>(in.getTotalSz()))
			{}
			Array2DImpl( const Array2DInfo& nsz, bool file=false )
			    : in( nsz )
			    , stor(file ? (ArrayNDLinearStorage*)
					 new ArrayNDFileStor<T>(in.getTotalSz())
				        : (ArrayNDLinearStorage*)
					 new ArrayNDMemStor<T>(in.getTotalSz()))
			{} 
			Array2DImpl( const Array2D<T>& templ )
			    : in( (const Array2DInfo&)templ.info() )
			    , stor( new ArrayNDMemStor<T>(in.getTotalSz()))
			{
			    if ( templ.getData() )
			    {
				int nr = in.getTotalSz();
				memcpy( getData(),templ.getData(),sizeof(T)*nr);
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
			~Array2DImpl() { delete stor; }

    virtual void	set( int p0, int p1, T v )
			{ stor->set(in.getMemPos(p0,p1), v); }
    virtual T		get( int p0, int p1 ) const
			{ return stor->get(in.getMemPos(p0,p1)); }

    const mPolyArray2DInfoTp&  info() const { return in; }

    bool		canSetInfo() { return true; }

    bool		setInfo( const ArrayNDInfo& ni )
			{
			    if ( ni.getNDim() != 2 ) return false; 
			    setSize( ni.getSize( 0 ), ni.getSize( 1 ) );
			    return true;
			}

    void		setSize( int d0, int d1 )
			{
			    in.setSize(0,d0);
			    in.setSize(1,d1);
			    stor->setSize(in.getTotalSz());
			}

protected:

    mDeclArrayNDProtMemb(Array2DInfoImpl)

};


template <class T> class Array3DImpl : public Array3D<T>
{
public:
			Array3DImpl( int sz0, int sz1, int sz2, bool file=false)
			    : in(sz0,sz1,sz2)
			    , stor(file ? (ArrayNDLinearStorage*)
					 new ArrayNDFileStor<T>(in.getTotalSz())
					: (ArrayNDLinearStorage*)
					 new ArrayNDMemStor<T>(in.getTotalSz()))
			{}
			Array3DImpl( const Array3DInfo& nsz, bool file=false )
			    : in(nsz)
			    , stor(file ? (ArrayNDLinearStorage*)
					 new ArrayNDFileStor<T>(in.getTotalSz())
					: (ArrayNDLinearStorage*)
					 new ArrayNDMemStor<T>(in.getTotalSz()))
			{}
			Array3DImpl( const Array3D<T>& templ )
			    : in( templ.info() )
			    , stor( new ArrayNDMemStor<T>(in.getTotalSz()))
			{
			    if ( templ.getData() )
			    {
				int nr = in.getTotalSz();
				memcpy( getData(),templ.getData(),sizeof(T)*nr);
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
			~Array3DImpl() { delete stor; }

    virtual void	set( int p0, int p1, int p2, T v )
			{ stor->set(in.getMemPos(p0,p1,p2), v); }
    virtual T		get( int p0, int p1, int p2 ) const
			{ return stor->get(in.getMemPos(p0,p1,p2)); }


    const mPolyArray3DInfoTp&	info() const { return in; }
    bool		canSetInfo() { return true; }
    bool		setInfo( const ArrayNDInfo& ni )
			{
			    if ( ni.getNDim() != 3 ) return false; 
			    setSize( ni.getSize( 0 ), ni.getSize( 1 ),
				     ni.getSize( 2 ) );
			    return true;
			}


    void		setSize( int d0, int d1, int d2 )
			{
			    in.setSize(0,d0);
			    in.setSize(1,d1);
			    in.setSize(2,d2);
			    stor->setSize(in.getTotalSz());
			}
protected:

    mDeclArrayNDProtMemb(Array3DInfoImpl)

};


template <class T> class ArrayNDImpl : public ArrayND<T>
{
public:
static ArrayND<T>*	create( const ArrayNDInfo& nsz, bool file=false );

			ArrayNDImpl( const ArrayNDInfo& nsz, bool file=false)
			    : in(nsz.clone())
			    , stor(file ? (ArrayNDLinearStorage*)
				     new ArrayNDFileStor<T>(in->getTotalSz())
				    : (ArrayNDLinearStorage*)
				     new ArrayNDMemStor<T>(in->getTotalSz()))
			    {}
			ArrayNDImpl( const ArrayND<T>& templ)
			    : in(templ.info().clone())
			    , stor(file ? (ArrayNDLinearStorage*)
				     new ArrayNDFileStor<T>(in->getTotalSz())
				    : (ArrayNDLinearStorage*)
				     new ArrayNDMemStor<T>(in->getTotalSz()))
			{
			    if ( templ.getData() )
			    {
				int nr = in.getTotalSz();
				memcpy( getData(),templ.getData(),sizeof(T)*nr);
			    }
			    else
			    {
				ArrayNDIter iter( *in );

				do
				{
				    set(iter.getPos(), templ.get(iter.getPos));
				} while ( next() );
			    }
			}

			~ArrayNDImpl()
			{ delete in; delete stor; }

    virtual void	set( const int* pos, T v )
			{ stor->set(in->getMemPos(pos), v); }
    virtual T		get( const int* pos ) const
			{ return stor->get(in->getMemPos(pos)); }


    const ArrayNDInfo&	info() const { return *in; }
    bool		canSetInfo() { return true; }
    bool		canChangeNrDims() const { return true; }
    bool		setInfo( const ArrayNDInfo& ni )
			{
			    int ndim = in->getNDim();
			    for ( int idx=0; idx<ndim; idx++ )
			    { in->setSize(idx,ni->getSize(idx)); }
			    stor->setSize(in->getTotalSz());
			    return true;
			}
 
    void		setSize( const int* d )
			{
			    int ndim = in->getNDim();
			    for ( int idx=0; idx<ndim; idx++ )
			    { in->setSize(idx,d[idx]); }
			    stor->setSize(in->getTotalSz());
			}

protected:

    mDeclArrayNDProtMemb(ArrayNDInfo*)

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

#endif
