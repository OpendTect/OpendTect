#ifndef bindatadesc_h
#define bindatadesc_h

/*
________________________________________________________________________

 CopyRight:	(C) de Groot-Bril Earth Sciences B.V.
 Author:	A.H.Bril
 Date:		Feb 2001
 Contents:	Binary data interpretation
 RCS:		$Id: bindatadesc.h,v 1.3 2001-05-02 13:49:51 windev Exp $
________________________________________________________________________

*/

#include <Pmacros.h>

#define mDeclConstr(T,ii,is) \
	BinDataDesc( const T* ) { set( ii, is, sizeof(T) ); } \
	BinDataDesc( const T& ) { set( ii, is, sizeof(T) ); }


/*!\brief Description of binary data.

Binary data in 'blobs' can usually be described by only a few pieces of info.
These are:

* Is the data of floating point type or integer?
* Is the data signed or unsigned? Usually, floating point data cannot be
  unsigned.
* How big is each number in terms of bytes? This can be 1, 2, 4 or 8
  bytes.

The info from this class can be stringified (user readable string) or dumped
binary into a short int (2 bytes).

*/

class BinDataDesc
{
public:

    enum ByteCount	{ N1=1, N2=2, N4=4, N8=8 };

			BinDataDesc( bool ii=false, bool is=true,
				     ByteCount b=N4 )
			: isint(ii), issigned(is), nrbytes(b)	{}
			BinDataDesc( bool ii, bool is, int b )
			: isint(ii), issigned(is),
			  nrbytes(nearestByteCount(ii,b))	{}
			BinDataDesc( unsigned short c )		{ set(c); }
			BinDataDesc( const char* s )		{ set(s); }
    virtual		~BinDataDesc()				{}

    inline bool		isInteger() const		{ return isint; }
    inline bool		isSigned() const		{ return issigned; }
    inline ByteCount	nrBytes() const			{ return nrbytes; }
    inline void		set( bool ii, bool is, ByteCount b )
			{ isint = ii; issigned = is; nrbytes = b; }
    inline void		set( bool ii, bool is, int b )
			{ isint = ii; issigned = is;
			  nrbytes = nearestByteCount(ii,b); }
    void		setInteger( bool yn )		{ isint = yn; }
    void		setSigned( bool yn )		{ issigned = yn; }
    void		setNrBytes( ByteCount n )	{ nrbytes = n; }
    void		setNrBytes( int n )
			{ nrbytes = nearestByteCount(isint,n); }

			// dump/restore
    virtual int		maxStringifiedSize() const	{ return 18; }
    virtual void	toString(char*) const;
			//!< Into a buffer allocated by client!
    virtual void	set(const char*);
    virtual unsigned short dump() const;
    virtual void	set(unsigned short);

			mDeclConstr(signed char,true,true)
			mDeclConstr(short,true,true)
			mDeclConstr(int,true,true)
			mDeclConstr(unsigned char,true,false)
			mDeclConstr(unsigned short,true,false)
			mDeclConstr(unsigned int,true,false)
			mDeclConstr(float,false,true)
			mDeclConstr(double,false,true)

    inline bool		operator ==( const BinDataDesc& dc ) const
			{ return isEqual(dc); }
    inline bool		operator !=( const BinDataDesc& dc ) const
			{ return !isEqual(dc); }
    inline bool		isEqual( const BinDataDesc& dc ) const
			{ return dump() == dc.dump(); }

    int			sizeFor( int n ) const		{ return nrbytes * n; }
    virtual bool	convertsWellTo(const BinDataDesc&) const;

    static ByteCount	nearestByteCount( bool is_int, int s )
			{
			    if ( !is_int ) return s > 6 ? N8 : N4;
			    if ( s < 2 ) s = 1;
			    else if ( s > 2 ) s = 4;
			    return (ByteCount)s;
			}
    static int		nextSize( bool is_int, int s )
			{
			    if ( s < 0 || s > 4 ) return -1;
			    if ( s == 0 )	  return is_int ? 1 : 4;
			    if ( !is_int )	  return s == 4 ? 8 : -1;
			    return s == 1 ? 2 : 4;
			}

protected:

    bool		isint;
    bool		issigned;
    ByteCount		nrbytes;

};

#undef mDeclConstr


#endif
