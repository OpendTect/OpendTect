#ifndef arraynd_h
#define arraynd_h
/*
________________________________________________________________________

 CopyRight:	(C) de Groot-Bril Earth Sciences B.V.
 Author:	K. Tingdahl
 Date:		9-3-1999
 RCS:		$Id: arraynd.h,v 1.4 2000-05-06 14:29:42 bert Exp $
________________________________________________________________________

*/

#include <gendefs.h>
template <class Type> class TypeSet;


class ArrayNDSize
{
public:

    virtual ArrayNDSize* clone() const		= 0;
    virtual		~ArrayNDSize()		{} 

    virtual int		getNDim() const		{ return 1; }
    virtual int		getSize(int dim) const	= 0;
    virtual bool	setSize(int dim,int sz) = 0;
 
    inline unsigned long  getTotalSz() const	{ return totalSz; }
    virtual unsigned long getArrayPos(const TypeSet<int>&) const	= 0;
    virtual bool	  validPos(const TypeSet<int>&) const		= 0;

protected:

    unsigned long 		totalSz;

    virtual unsigned long	calcTotalSz()	{ return getSize(0); }

};

inline bool operator ==( const ArrayNDSize& a1, const ArrayNDSize& a2 )
{
    int nd = a1.getNDim();
    if ( nd != a2.getNDim() ) return NO;
    for ( int idx=0; idx<nd; idx++ )
	if ( a1.getSize(idx) != a2.getSize(idx) ) return NO;
    return YES;
}

inline bool operator !=( const ArrayNDSize& a1, const ArrayNDSize& a2 )
{ return !(a1 == a2); }


class Array1DSize : public ArrayNDSize
{
public:

    int				getNDim() const			{ return 1; }

    virtual unsigned long	getArrayPos(int) const		= 0;
    virtual bool		validPos(int) const		= 0;

};


class Array2DSize : public ArrayNDSize
{
public:

    int				getNDim() const			{ return 2; }

    virtual unsigned long	getArrayPos(int,int) const	= 0;
    virtual bool		validPos(int,int) const		= 0;

};


class Array3DSize : public ArrayNDSize
{
public:

    int				getNDim() const			{ return 3; }

    virtual unsigned long	getArrayPos(int, int, int) const= 0;
    virtual bool		validPos(int,int,int) const	= 0;

};


template <class Type> class ArrayND
{
public:

    virtual			~ArrayND()	{}

    virtual Type		getValOff( unsigned long off ) const
				{
				    Type res = *(Type*)(getData()+off);
				    unlockData();
				    return res;
				}
    virtual void		setValOff(unsigned long off, Type val)
                                {
				    *(Type*)(getData()+off) = val;
				    dataUpdated();
				    unlockData();
				}

    virtual Type                getVal( const TypeSet<int>& pos ) const
                                { return getValOff( size().getArrayPos(pos) ); }
    virtual void		setVal( const TypeSet<int>& pos, Type val )
				{ setValOff( size().getArrayPos( pos ), val ); }

    virtual Type*		getData() const			= 0;
    virtual void		dataUpdated()			{}
    virtual void		unlockData() const		{}

    virtual const ArrayNDSize&	size() const			= 0;

    template<class Type> class Proxy
    {
    public:
			Proxy( ArrayND<Type>& array_, unsigned long off_ )
			    : array( array_ )
			    , off( off_ )				{}	

	Proxy&		operator=( const Proxy& n )
			{ array.setValOff( off, n ); return *this; }
	Proxy&		operator=( Type n  )
			{ array.setValOff( off, n ); return *this; }
	Proxy&		operator+=( Type n )
			{
			    array.setValOff( off, array.getValOff(off)+n);
			    return *this;
			}
	Proxy&		operator-=( Type n )
			{
			    array.setValOff( off, array.getValOff(off)-n);
			    return *this;
			}
	Proxy&		operator*=( Type n )
			{
			    array.setValOff( off, array.getValOff(off)*n);
			    return *this;
			}
	Proxy&		operator/=( Type n )
			{
			    array.setValOff( off, array.getValOff(off)/n);
			    return *this;
			}

			operator Type() const
			{ return array.getValOff( off ); }
    private:
	ArrayND<Type>&	array;
	unsigned long	off;
    };			
};


template <class Type> class Array1D : public ArrayND<Type>
{
public: 

    virtual void        setVal( int p, Type val )
                        { setValOff(size().getArrayPos(p),val); }
    virtual Type        getVal( int p ) const
			{ return getValOff(size().getArrayPos(p)); }

    Type		operator() (int pos) const
			{ return getVal( pos ); }
    ArrayND::Proxy<Type>operator() (int pos)
			{ return Proxy<Type>( *this, pos ); }

    virtual const Array1DSize& size() const = 0;
};



template <class Type> class Array2D : public ArrayND<Type>
{
public: 

    virtual void        setVal( int p0, int p1, Type val )
                        { setValOff(size().getArrayPos(p0,p1),val); }
    virtual Type        getVal( int p0, int p1 ) const
			{ return getValOff(size().getArrayPos(p0,p1)); }

    Type		operator() (int p0, int p1) const
			{ return getValOff(size().getArrayPos(p0,p1)); }
    ArrayND::Proxy<Type>operator() (int p0, int p1)
			{
			    return ArrayND::Proxy<Type>(*this,
						size().getArrayPos(p0,p1));
			}

    virtual const Array2DSize& size() const = 0;

};


template <class Type> class Array3D : public ArrayND<Type>
{
public: 

    virtual void        setVal( int p0, int p1, int p2, Type val )
                        { setValOff(size().getArrayPos(p0,p1,p2),val); }
    virtual Type        getVal( int p0, int p1, int p2 ) const
			{ return getValOff(size().getArrayPos(p0,p1,p2)); }

    Type		operator() (int p0, int p1, int p2) const
			{ return getValOff(size().getArrayPos(p0,p1,p2)); }
    ArrayND::Proxy<Type>operator() (int p0, int p1, int p2)
			{
			    return ArrayND::Proxy<Type>(*this,
						size().getArrayPos(p0,p1,p2));
			}

    virtual const Array3DSize& size() const = 0;
};


#endif
