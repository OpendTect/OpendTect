#ifndef arrayndinfo_h
#define arrayndinfo_h
/*
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	K. Tingdahl
 Date:		9-3-1999
 RCS:		$Id: arrayndinfo.h,v 1.7 2006-07-10 17:42:33 cvskris Exp $
________________________________________________________________________

An ArrayNDInfo contains the information about the size of ArrayND, and
in what order the data is stored (if accessable via a pointer).

*/

#include <gendefs.h>

#define mPolyArray1DInfoImplTp mPolyRet(ArrayNDInfo,Array1DInfoImpl)
#define mPolyArray2DInfoImplTp mPolyRet(ArrayNDInfo,Array2DInfoImpl)
#define mPolyArray3DInfoImplTp mPolyRet(ArrayNDInfo,Array3DInfoImpl)

class ArrayNDInfo
{
public:

    virtual ArrayNDInfo* clone() const		= 0;
    virtual		~ArrayNDInfo()		{} 

    virtual int		getNDim() const		{ return 1; }
    virtual int		getSize(int dim) const	= 0;
    virtual bool	setSize(int dim,int sz) = 0;
 
    inline uint64	getTotalSz() const	{ return totalSz; }
    virtual uint64	getMemPos(const int*) const		= 0;
    virtual bool	validPos(const int*) const		= 0;
    virtual void	getArrayPos(uint64, int*) const;

protected:

    uint64 		totalSz;

    virtual uint64	calcTotalSz()	{ return getSize(0); }

};

inline bool operator ==( const ArrayNDInfo& a1, const ArrayNDInfo& a2 )
{
    int nd = a1.getNDim();
    if ( nd != a2.getNDim() ) return false;
    for ( int idx=0; idx<nd; idx++ )
	if ( a1.getSize(idx) != a2.getSize(idx) ) return false;
    return true;
}

inline bool operator !=( const ArrayNDInfo& a1, const ArrayNDInfo& a2 )
{ return !(a1 == a2); }


class Array1DInfo : public ArrayNDInfo
{
public:

    int				getNDim() const			{ return 1; }

    virtual uint64		getMemPos(int) const		= 0;
    virtual bool		validPos(int) const		= 0;

};


class Array2DInfo : public ArrayNDInfo
{
public:

    int				getNDim() const			{ return 2; }

    virtual uint64		getMemPos(int,int) const	= 0;
    virtual bool		validPos(int,int) const		= 0;

};


class Array3DInfo : public ArrayNDInfo
{
public:

    int				getNDim() const			{ return 3; }

    virtual uint64		getMemPos(int, int, int) const= 0;
    virtual bool		validPos(int,int,int) const	= 0;

};


class Array1DInfoImpl : public Array1DInfo
{
public:
    mPolyArray1DInfoImplTp* clone() const
			{ return new Array1DInfoImpl(*this); }

			Array1DInfoImpl(int nsz=0); 
			Array1DInfoImpl(const Array1DInfo&);

    int         	getSize(int dim) const; 
    bool        	setSize(int dim,int nsz);

    uint64	 	getMemPos(const int*) const;
    bool          	validPos(const int*) const;
    
    uint64		getMemPos(int) const;
    bool		validPos( int p ) const
			{ return p < 0 || p >= sz ? false : true; }

protected:

    int			sz;

};


class Array2DInfoImpl : public Array2DInfo
{
public:

    mPolyArray2DInfoImplTp* clone() const { return new Array2DInfoImpl(*this); }

			Array2DInfoImpl(int sz0=0, int sz1=0);
			Array2DInfoImpl(const Array2DInfo&);

    int                 getSize(int dim) const;
    bool                setSize(int dim,int nsz);

    uint64		getMemPos(const int*) const;
    uint64		getMemPos(int,int) const; 

    bool                validPos(const int*) const;
    bool                validPos(int,int) const;


protected:

    int                 sz[2];

    uint64		calcTotalSz() const;

};


class Array3DInfoImpl : public Array3DInfo
{
public:

    mPolyArray3DInfoImplTp* clone() const { return new Array3DInfoImpl(*this); }

			Array3DInfoImpl(int sz0=0, int sz1=0, int sz2=0);
			Array3DInfoImpl(const Array3DInfo&);

    int                 getSize(int dim) const; 
    bool                setSize(int dim,int nsz);

    uint64		getMemPos(const int*) const;
    uint64		getMemPos(int,int,int) const; 

    bool                validPos(const int*) const;
    bool                validPos(int,int,int) const;

protected:

    int                 sz[3];

    uint64		calcTotalSz() const;

};  


class ArrayNDInfoImpl : public ArrayNDInfo
{
public:

    ArrayNDInfo*	clone() const;
    static ArrayNDInfo*	create( int ndim );

			ArrayNDInfoImpl(int ndim);
			ArrayNDInfoImpl(const ArrayNDInfo&);
			ArrayNDInfoImpl(const ArrayNDInfoImpl&);

			~ArrayNDInfoImpl();

    int                 getNDim() const;
    int                 getSize(int dim) const;
    bool                setSize(int dim,int nsz);

    uint64		getMemPos(const int*) const;
    bool                validPos(const int*) const;

protected:

    int*		sizes;
    int 		ndim;

    uint64		calcTotalSz() const;

};


/*!\brief ArrayNDIter is an object that is able to iterate through all samples
   in a ArrayND.
   \par
   ArrayNDIter will stand on the first position when initiated, and move to
   the second at the fist call to next(). next() will return false when
   no more positions are avaliable
*/

class ArrayNDIter
{
public:
				ArrayNDIter( const ArrayNDInfo& );
				~ArrayNDIter();

    bool			next();
    void			reset();

    template <class T> void inline	setPos( const T& idxabl );
    const int*			getPos() const { return position_; }
    int				operator[](int) const;

protected:
    bool			inc(int);

    int*			position_;
    const ArrayNDInfo&		sz_;
};


template <class T> inline void ArrayNDIter::setPos( const T& idxable )
{
    for ( int idx=sz_.getNDim()-1; idx>=0; idx-- )
	position_[idx] = idxable[idx];
}

#endif

