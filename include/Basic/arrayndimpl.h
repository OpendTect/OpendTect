#ifndef arrayndimpl_h
#define arrayndimpl_h
/*
________________________________________________________________________

 CopyRight:	(C) de Groot-Bril Earth Sciences B.V.
 Author:	K. Tingdahl
 Date:		9-3-1999
 RCS:		$Id: arrayndimpl.h,v 1.3 2000-02-10 13:01:37 bert Exp $
________________________________________________________________________

*/


#include <databuf.h> 
#include <arraynd.h>

class Array1DSizeImpl : public Array1DSize
{
public:

    Array1DSizeImpl*	clone() const	{ return new Array1DSizeImpl(*this); }

			Array1DSizeImpl(int nsz=0); 
			Array1DSizeImpl(const Array1DSize&);

    int         	getSize(int dim) const; 
    bool        	setSize(int dim,int nsz);

    unsigned long 	getArrayPos(const TypeSet<int>&) const;
    bool          	validPos(const TypeSet<int>&) const;
    
    unsigned long	getArrayPos(int) const;
    bool		validPos( int p ) const
			{ return p < 0 || p >= sz ? NO : YES; }

protected:

    int			sz;

};


class Array2DSizeImpl : public Array2DSize
{
public:

    Array2DSizeImpl*	clone() const	{ return new Array2DSizeImpl(*this); }

                        Array2DSizeImpl(int sz0=0, int sz1=0);
			Array2DSizeImpl(const Array2DSize&);

    int                 getSize(int dim) const;
    bool                setSize(int dim,int nsz);

    unsigned long       getArrayPos(const TypeSet<int>&) const;
    unsigned long	getArrayPos(int,int) const; 

    bool                validPos(const TypeSet<int>&) const;
    bool                validPos(int,int) const;


protected:

    int                 sz[2];

    unsigned long       calcTotalSz() const;

};


class Array3DSizeImpl : public Array3DSize
{
public:

    Array3DSizeImpl*	clone() const	{ return new Array3DSizeImpl(*this); }

                        Array3DSizeImpl(int sz0=0, int sz1=0, int sz2=0);
                        Array3DSizeImpl(const Array3DSize&);

    int                 getSize(int dim) const; 
    bool                setSize(int dim,int nsz);

    unsigned long       getArrayPos(const TypeSet<int>&) const;
    unsigned long       getArrayPos(int,int,int) const; 

    bool                validPos(const TypeSet<int>&) const;
    bool                validPos(int,int,int) const;

protected:

    int                 sz[3];

    unsigned long       calcTotalSz() const;

};  


class ArrayNDSizeImpl : public ArrayNDSize
{
public:

    ArrayNDSizeImpl*	clone() const	{ return new ArrayNDSizeImpl(*this); }

                        ArrayNDSizeImpl(int ndim);
			ArrayNDSizeImpl(const ArrayNDSize&);
                        ArrayNDSizeImpl(const ArrayNDSizeImpl&);

			~ArrayNDSizeImpl();

    int                 getNDim() const;
    int                 getSize(int dim) const;
    bool                setSize(int dim,int nsz);

    unsigned long       getArrayPos(const TypeSet<int>&) const;
    bool                validPos(const TypeSet<int>&) const;

protected:

    TypeSet<int>&	sizes;

    unsigned long       calcTotalSz() const;

};


template <class Type> class Array1DImpl : public Array1D<Type>
{
public:
			Array1DImpl(int nsz)
			: sz(nsz)
			, dbuf(sz.getTotalSz(), sizeof(Type), NO)    {}

    virtual Type	getValOff( unsigned off ) const
			{ return *(((Type*) dbuf.data)+off); }

    virtual void	setValOff( unsigned off, Type val )
			{ *(((Type*) dbuf.data)+off) = val; }

    Type*               getData() const
                            { return (Type *)dbuf.data; } 
    const Array1DSize&  size() const
                            { return sz; }

    void		setSize( int s )
			{ sz.setSize(0,s); dbuf.reSize(sz.getTotalSz()); }

protected:

    // Don't change the order of these attributes!
    Array1DSizeImpl	sz;
    DataBuffer 		dbuf;

};


template <class Type> class Array2DImpl : public Array2D<Type>
{
public:
			Array2DImpl(int sz0, int sz1)
 			: sz(sz0,sz1)
                        , dbuf(sz.getTotalSz(),sizeof(Type),NO)    {}
                        Array2DImpl( const Array2DSize& nsz )
                        : sz( nsz )
                        , dbuf(sz.getTotalSz(),sizeof(Type),NO)    {} 

    virtual Type	getValOff( unsigned off ) const
			{ return *(((Type*) dbuf.data)+off); }

    virtual void	setValOff( unsigned off, Type val )
			{ *(((Type*) dbuf.data)+off) = val; }

    Type*               getData() const
                            { return (Type*)dbuf.data; }
    const Array2DSize&  size() const
                            { return sz; }

    void		setSize( int d0, int d1 )
			{ sz.setSize(0,d0); sz.setSize(1,d1);
			  dbuf.reSize(sz.getTotalSz()); }

protected:

    // Don't change the order of these attributes!
    Array2DSizeImpl	sz;	
    DataBuffer		dbuf;

};


template <class Type> class Array3DImpl : public Array3D<Type>
{
public:
                        Array3DImpl( int sz0, int sz1, int sz2 )
                        : sz(sz0,sz1,sz2)
                        , dbuf(sz.getTotalSz(),sizeof(Type),NO)    {}
                        Array3DImpl( const Array3DSize& nsz )
                        : sz(nsz)
                        , dbuf(sz.getTotalSz(),sizeof(Type),NO)    {}

    virtual Type	getValOff( unsigned off ) const
			{ return *(((Type*) dbuf.data)+off); }

    virtual void	setValOff( unsigned off, Type val )
			{ *(((Type*) dbuf.data)+off) = val; }

    Type*               getData() const
                            { return (Type*)dbuf.data; }
    const Array3DSize&	size() const
			    { return sz; }

    void		setSize( int d0, int d1, int d2 )
			{ sz.setSize(0,d0); sz.setSize(1,d1); sz.setSize(2,d2);
			  dbuf.reSize(sz.getTotalSz()); }
protected:

    // Don't change the order of these attributes!
    Array3DSizeImpl     sz;
    DataBuffer          dbuf;

};


template <class Type> class ArrayNDImpl : public ArrayND<Type>
{
public:
                        ArrayNDImpl( const ArrayNDSize& nsz)
                        : sz(nsz)
                        , dbuf(sz.getTotalSz(),sizeof(Type),NO)    {}

    virtual Type	getValOff( unsigned off ) const
			{ return *(((Type*) dbuf.data)+off); }

    virtual void	setValOff( unsigned off, Type val )
			{ *(((Type*) dbuf.data)+off) = val; }

    void                setVal( const TypeSet<int>& pos, Type val )
			    { setValOff(sz.getArrayPos(pos), val); }
    Type                getVal( const TypeSet<int>& pos ) const
                            { return getValOff(sz.getArrayPos(pos)); }

    Type*               getData() const
                            { return (Type*) dbuf.data; }
    const ArrayNDSize&	size() const
			    { return sz; }

    void		setSize( const TypeSet<int> d )
			{
			    int ndim = sz.getNDim();
			    for ( int idx=0; idx<ndim; idx++ )
				{ if ( idx<d.size() ) sz.setDim(idx,d[idx]); }
			    dbuf.reSize(sz.getTotalSz());
			}

protected:

    // Don't change the order of these attributes!
    ArrayNDSizeImpl     sz;
    DataBuffer          dbuf;

}; 

#endif
