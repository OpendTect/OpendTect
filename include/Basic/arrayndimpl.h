#ifndef arrayndimpl_h
#define arrayndimpl_h
/*
________________________________________________________________________

 CopyRight:	(C) de Groot-Bril Earth Sciences B.V.
 Author:	K. Tingdahl
 Date:		9-3-1999
 RCS:		$Id: arrayndimpl.h,v 1.6 2000-12-14 14:06:40 kristofer Exp $
________________________________________________________________________

*/


#include <databuf.h> 
#include <arraynd.h>


template <class T> class Array1DImpl : public Array1D<T>
{
public:
			Array1DImpl(int nsz)
			    : in(nsz)
			    , dbuf(in.getTotalSz(), sizeof(T), false)    {}
			Array1DImpl(const Array1D<T>& templ)
			    : in(templ.info()) 
			    , dbuf(in.getTotalSz(), sizeof(T), false)
			{
			    int nr = in.getTotalSz();

			    if ( templ.getData() )
				memcpy( getData(),templ.getData(),sizeof(T)*nr);
			    else
				for ( int idx=0; idx<nr; idx++ )
				    set(idx, templ.get(idx));
			}

    virtual void	set( int pos, T v ) { *(((T*) dbuf.data)+pos) = v; }
    virtual T		get( int pos ) const {return *(((T*)dbuf.data)+pos);}
			

    T*			getData() const { return (T*)dbuf.data; } 
    const Array1DInfo&  info() const { return in; }

    void		setSize( int s )
			{ in.setSize(0,s); dbuf.reSize(in.getTotalSz()); }

protected:

    // Don't change the order of these attributes!
    Array1DInfoImpl	in;
    DataBuffer 		dbuf;
};


template <class T> class Array2DImpl : public Array2D<T>
{
public:
			Array2DImpl(int sz0, int sz1)
			    : in(sz0,sz1)
			    , dbuf(in.getTotalSz(),sizeof(T),false)    {}
			Array2DImpl( const Array2DInfo& nsz )
			    : in( nsz )
			    , dbuf(in.getTotalSz(),sizeof(T),false)    {} 
			Array2DImpl( const Array2D<T>& templ )
			    : in( templ.info() )
			    , dbuf(in.getTotalSz(),sizeof(T),false)
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

    virtual void	set( int p0, int p1, T v )
			{ *(((T*) dbuf.data)+in.getArrayPos(p0,p1)) = v; }
    virtual T		get( int p0, int p1 ) const
			{ return *(((T*) dbuf.data)+in.getArrayPos(p0,p1)); }

    T*			getData() const { return (T*)dbuf.data; }
    const Array2DInfo&  info() const { return in; }

    void		setSize( int d0, int d1 )
			{
			    in.setSize(0,d0);
			    in.setSize(1,d1);
			    dbuf.reSize(in.getTotalSz());
			}

protected:

    // Don't change the order of these attributes!
    Array2DInfoImpl	in;	
    DataBuffer		dbuf;

};


template <class T> class Array3DImpl : public Array3D<T>
{
public:
			Array3DImpl( int sz0, int sz1, int sz2 )
			    : in(sz0,sz1,sz2)
			    , dbuf(in.getTotalSz(),sizeof(T),false)    {}
			Array3DImpl( const Array3DInfo& nsz )
			    : in(nsz)
			    , dbuf(in.getTotalSz(),sizeof(T),false)    {}
			Array3DImpl( const Array3D<T>& templ )
			    : in( templ.info() )
			    , dbuf(in.getTotalSz(),sizeof(T),false)
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

    virtual void	set( int p0, int p1, int p2, T v )
			{ *(((T*) dbuf.data)+in.getArrayPos(p0,p1,p2)) = v; }
    virtual T		get( int p0, int p1, int p2 ) const
			{ return *(((T*) dbuf.data)+in.getArrayPos(p0,p1,p2)); }


    T*			getData() const { return (T*)dbuf.data; }
    const Array3DInfo&	info() const { return in; }

    void		setSize( int d0, int d1, int d2 )
			{
			    in.setSize(0,d0);
			    in.setSize(1,d1);
			    in.setSize(2,d2);
			    dbuf.reSize(in.getTotalSz());
			}
protected:

    // Don't change the order of these attributes!
    Array3DInfoImpl     in;
    DataBuffer          dbuf;

};


template <class T> class ArrayNDImpl : public ArrayND<T>
{
public:
    static ArrayND<T>*	create( const ArrayNDInfo& nsz );

			ArrayNDImpl( const ArrayNDInfo& nsz)
			    : in(nsz.clone())
			    , dbuf(nsz.getTotalSz(),sizeof(T),false)	{}
			ArrayNDImpl( const ArrayND<T>& templ)
			    : in(templ.info().clone())
			    , dbuf(in->getTotalSz(),sizeof(T),false)
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
			{ delete in; }

    virtual void	set( const int* pos, T v )
			{ *(((T*) dbuf.data)+in->getArrayPos(pos)) = v; }
    virtual T		get( const int* pos ) const
			{ return *(((T*) dbuf.data)+in->getArrayPos(pos)); }


    T*			getData() const { return (T*) dbuf.data; }
    const ArrayNDInfo&	info() const { return *in; }

    void		setSize( const int* d )
			{
			    int ndim = in->getNDim();
			    for ( int idx=0; idx<ndim; idx++ )
			    { in->setSize(idx,d[idx]); }
			    dbuf.reSize(in->getTotalSz());
			}

protected:

    // Don't change the order of these attributes!
    ArrayNDInfo*	in;
    DataBuffer          dbuf;

}; 


template <class T> inline
ArrayND<T>* ArrayNDImpl<T>::create( const ArrayNDInfo& nsz )
{
    int ndim = nsz.getNDim();

    if ( ndim==1 ) return new Array1DImpl<T>( nsz.getSize(0));
    if ( ndim==2 ) return new Array2DImpl<T>( nsz.getSize(0),nsz.getSize(1));
    if ( ndim==3 ) return new Array3DImpl<T>( nsz.getSize(0),
						 nsz.getSize(1),
						 nsz.getSize(2));

    return new ArrayNDImpl<T>( nsz );
}

#endif
