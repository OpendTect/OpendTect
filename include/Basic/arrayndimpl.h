#ifndef arrayndimpl_h
#define arrayndimpl_h
/*
________________________________________________________________________

 CopyRight:	(C) de Groot-Bril Earth Sciences B.V.
 Author:	K. Tingdahl
 Date:		9-3-1999
 RCS:		$Id: arrayndimpl.h,v 1.10 2001-04-18 14:45:36 bert Exp $
________________________________________________________________________

*/


#include <arraynd.h>

#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>
#include <stdio.h>
#include <unistd.h>
#include "bufstring.h"

template <class T>
class ArrayNDMemStor : public ArrayND<T>::LinearStorage
{
public:

    bool	isOK() const { return ptr; }
    T		get(int pos) const {return ptr[pos]; }
    void	set(int pos, T val ) { ptr[pos] = val; }

    const T*	getData() const { return ptr; }

    void	setSize( int nsz )
		{ delete ptr; ptr = new T[nsz]; }
    int		size() const { return sz; }

		ArrayNDMemStor( int nsz )
		    : ptr ( new T[nsz] ), sz( nsz ) {}
    inline	~ArrayNDMemStor() { delete ptr; }

protected:
    int		sz;
    T*		ptr;
};


template <class T>
class ArrayNDFileStor : public ArrayND<T>::LinearStorage
{
public:

    bool	isOK() const { return handle != -1; }

    T		get(int pos) const
		{
		    lseek( handle, pos*sizeof(T), SEEK_SET );
		    T res;
		    read( handle, &res, sizeof(T));
		    return res;
		}

    void	set(int pos, T val ) 
		{
		    lseek( handle, pos*sizeof(T), SEEK_SET );
		    write( handle, &val, sizeof(T));
		}

    const T*	getData() const { return 0; }

    void	setSize( int nsz )
		{
		    sz = nsz;
		    if ( sz ) lseek( handle, (sz-1)*sizeof(T), SEEK_SET );
		    if ( write( handle, &sz, 1 ) == -1 )
		    {
			close( handle );
			handle = -1;
		    }
		}
    int		size() const { return sz; }

		ArrayNDFileStor( int nsz )
		    : sz( nsz )
		{
		    name = "/tmp/XXXXXX";

		    handle = mkstemp( name.buf() );
		    if ( handle==-1 ) return;

		}
		
    inline	~ArrayNDFileStor()
		{
		    close( handle );
		    remove( name );
		}

protected:
    BufferString name;
    int		sz;
    int		handle;
};


template <class T> class Array1DImpl : public Array1D<T>
{
public:
			Array1DImpl(int nsz, bool file=false)
			    : in(nsz)
			    , stor(file ? (ArrayND<T>::LinearStorage*)
					 new ArrayNDFileStor<T>(in.getTotalSz())
				        : (ArrayND<T>::LinearStorage*)
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
			
    const Array1DInfo&  info() const		{ return in; }

    void		setSize( int s ) { in.setSize(0,s); stor->setSize(s); }
protected:

    // Don't change the order of these attributes!
    Array1DInfoImpl		in;
    ArrayND<T>::LinearStorage*	stor;

    const ArrayND<T>::LinearStorage*	getStorage_() const { return stor; }
};


template <class T> class Array2DImpl : public Array2D<T>
{
public:
			Array2DImpl(int sz0, int sz1, bool file=false)
			    : in(sz0,sz1)
			    , stor(file ? (ArrayND<T>::LinearStorage*)
					 new ArrayNDFileStor<T>(in.getTotalSz())
				        : (ArrayND<T>::LinearStorage*)
					 new ArrayNDMemStor<T>(in.getTotalSz()))
			{}
			Array2DImpl( const Array2DInfo& nsz, bool file=false )
			    : in( nsz )
			    , stor(file ? (ArrayND<T>::LinearStorage*)
					 new ArrayNDFileStor<T>(in.getTotalSz())
				        : (ArrayND<T>::LinearStorage*)
					 new ArrayNDMemStor<T>(in.getTotalSz()))
			{} 
			Array2DImpl( const Array2D<T>& templ )
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

    const Array2DInfo&  info() const { return in; }

    void		setSize( int d0, int d1 )
			{
			    in.setSize(0,d0);
			    in.setSize(1,d1);
			    stor->setSize(in.getTotalSz());
			}

protected:

    // Don't change the order of these attributes!
    Array2DInfoImpl		in;	
    ArrayND<T>::LinearStorage*	stor;

    const ArrayND<T>::LinearStorage*	getStorage_() const { return stor; }
};


template <class T> class Array3DImpl : public Array3D<T>
{
public:
			Array3DImpl( int sz0, int sz1, int sz2, bool file=false)
			    : in(sz0,sz1,sz2)
			    , stor(file ? (ArrayND<T>::LinearStorage*)
					 new ArrayNDFileStor<T>(in.getTotalSz())
					: (ArrayND<T>::LinearStorage*)
					 new ArrayNDMemStor<T>(in.getTotalSz()))
			{}
			Array3DImpl( const Array3DInfo& nsz, bool file=false )
			    : in(nsz)
			    , stor(file ? (ArrayND<T>::LinearStorage*)
					 new ArrayNDFileStor<T>(in.getTotalSz())
					: (ArrayND<T>::LinearStorage*)
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


    const Array3DInfo&	info() const { return in; }

    void		setSize( int d0, int d1, int d2 )
			{
			    in.setSize(0,d0);
			    in.setSize(1,d1);
			    in.setSize(2,d2);
			    stor->setSize(in.getTotalSz());
			}
protected:

    // Don't change the order of these attributes!
    Array3DInfoImpl     		in;
    ArrayND<T>::LinearStorage*		stor;


    const ArrayND<T>::LinearStorage*	getStorage_() const { return stor; }
};


template <class T> class ArrayNDImpl : public ArrayND<T>
{
public:
static ArrayND<T>*	create( const ArrayNDInfo& nsz, bool file=false );

			ArrayNDImpl( const ArrayNDInfo& nsz, bool file=false)
			    : in(nsz.clone())
			    , stor(file ? (ArrayND<T>::LinearStorage*)
				     new ArrayNDFileStor<T>(in->getTotalSz())
				    : (ArrayND<T>::LinearStorage*)
				     new ArrayNDMemStor<T>(in->getTotalSz()))
			    {}
			ArrayNDImpl( const ArrayND<T>& templ)
			    : in(templ.info().clone())
			    , stor(file ? (ArrayND<T>::LinearStorage*)
				     new ArrayNDFileStor<T>(in->getTotalSz())
				    : (ArrayND<T>::LinearStorage*)
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

    void		setSize( const int* d )
			{
			    int ndim = in->getNDim();
			    for ( int idx=0; idx<ndim; idx++ )
			    { in->setSize(idx,d[idx]); }
			    stor->setSize(in->getTotalSz());
			}

    protected:

    // Don't change the order of these attributes!
    ArrayNDInfo*	in;
    ArrayND<T>::LinearStorage*	stor;

    const ArrayND<T>::LinearStorage*	getStorage_() const { return stor; }
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
