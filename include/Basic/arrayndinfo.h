#ifndef arrayndinfo_h
#define arrayndinfo_h
/*
________________________________________________________________________

 CopyRight:	(C) de Groot-Bril Earth Sciences B.V.
 Author:	K. Tingdahl
 Date:		9-3-1999
 RCS:		$Id: arrayndinfo.h,v 1.3 2001-05-02 13:50:03 windev Exp $
________________________________________________________________________

An ArrayNDInfo contains the information about the size of ArrayND, and
in what order the data is stored (if accessable via a pointer).

*/

#include <gendefs.h>

class ArrayNDInfo
{
public:

    virtual ArrayNDInfo* clone() const		= 0;
    virtual		~ArrayNDInfo()		{} 

    virtual int		getNDim() const		{ return 1; }
    virtual int		getSize(int dim) const	= 0;
    virtual bool	setSize(int dim,int sz) = 0;
 
    inline unsigned long  getTotalSz() const	{ return totalSz; }
    virtual unsigned long getMemPos(const int*) const		= 0;
    virtual bool	  validPos(const int*) const		= 0;
    virtual void	getArrayPos(unsigned long, int*) const;

protected:

    unsigned long 		totalSz;

    virtual unsigned long	calcTotalSz()	{ return getSize(0); }

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

    virtual unsigned long	getMemPos(int) const		= 0;
    virtual bool		validPos(int) const		= 0;

};


class Array2DInfo : public ArrayNDInfo
{
public:

    int				getNDim() const			{ return 2; }

    virtual unsigned long	getMemPos(int,int) const	= 0;
    virtual bool		validPos(int,int) const		= 0;

};


class Array3DInfo : public ArrayNDInfo
{
public:

    int				getNDim() const			{ return 3; }

    virtual unsigned long	getMemPos(int, int, int) const= 0;
    virtual bool		validPos(int,int,int) const	= 0;

};


class Array1DInfoImpl : public Array1DInfo
{
#define cloneTp		mPolyRet(ArrayNDInfo,Array1DInfoImpl)
public:
    cloneTp*		clone() const
			{ return new Array1DInfoImpl(*this); }

			Array1DInfoImpl(int nsz=0); 
			Array1DInfoImpl(const Array1DInfo&);

    int         	getSize(int dim) const; 
    bool        	setSize(int dim,int nsz);

    unsigned long 	getMemPos(const int*) const;
    bool          	validPos(const int*) const;
    
    unsigned long	getMemPos(int) const;
    bool		validPos( int p ) const
			{ return p < 0 || p >= sz ? false : true; }

protected:

    int			sz;

#undef cloneTp
};


class Array2DInfoImpl : public Array2DInfo
{
#define cloneTp		mPolyRet(ArrayNDInfo,Array2DInfoImpl)
public:

    cloneTp*		clone() const	{ return new Array2DInfoImpl(*this); }

			Array2DInfoImpl(int sz0=0, int sz1=0);
			Array2DInfoImpl(const Array2DInfo&);

    int                 getSize(int dim) const;
    bool                setSize(int dim,int nsz);

    unsigned long       getMemPos(const int*) const;
    unsigned long	getMemPos(int,int) const; 

    bool                validPos(const int*) const;
    bool                validPos(int,int) const;


protected:

    int                 sz[2];

    unsigned long       calcTotalSz() const;

#undef cloneTp
};


class Array3DInfoImpl : public Array3DInfo
{
#define cloneTp		mPolyRet(ArrayNDInfo,Array3DInfoImpl)
public:

    cloneTp*		clone() const	{ return new Array3DInfoImpl(*this); }

			Array3DInfoImpl(int sz0=0, int sz1=0, int sz2=0);
			Array3DInfoImpl(const Array3DInfo&);

    int                 getSize(int dim) const; 
    bool                setSize(int dim,int nsz);

    unsigned long       getMemPos(const int*) const;
    unsigned long       getMemPos(int,int,int) const; 

    bool                validPos(const int*) const;
    bool                validPos(int,int,int) const;

protected:

    int                 sz[3];

    unsigned long       calcTotalSz() const;

#undef cloneTp
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

    unsigned long       getMemPos(const int*) const;
    bool                validPos(const int*) const;

protected:

    int*		sizes;
    int 		ndim;

    unsigned long       calcTotalSz() const;

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

    const int*			getPos() const { return position; }
    int				operator[](int) const;

protected:
    bool			inc(int);

    int*			position;
    const ArrayNDInfo&		sz;
};

#endif

