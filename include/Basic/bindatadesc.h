#ifndef bindatadesc_h
#define bindatadesc_h

/*
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	A.H.Bril
 Date:		Feb 2001
 Contents:	Binary data interpretation
 RCS:		$Id$
________________________________________________________________________

*/

#include "basicmod.h"
#include "gendefs.h"

#define mDeclBinDataDescConstr(T,ii,is) \
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
binary into two unsigned chars.

In normal work one will use the DataCharacteristics subclass, which can also
provide a 'run-time' data interpreter class for fast conversion to internal
data types.

*/

mClass(Basic) BinDataDesc
{
public:

    enum ByteCount	{ N1=1, N2=2, N4=4, N8=8 };

			BinDataDesc( bool ii=false, bool is=true,
				     ByteCount b=N4 )
			: isint_(ii), issigned_(is), nrbytes_(b)	{}
			BinDataDesc( bool ii, bool is, int b )
			: isint_(ii), issigned_(is),
			  nrbytes_(nearestByteCount(ii,b))	{}
			BinDataDesc( unsigned char c1, unsigned char c2 )
			    					{ set(c1,c2); }
			BinDataDesc( const char* s )		{ set(s); }
    virtual		~BinDataDesc()				{}

    inline bool		isInteger() const		{ return isint_; }
    inline bool		isSigned() const		{ return issigned_; }
    inline ByteCount	nrBytes() const			{ return nrbytes_; }
    inline void		set( bool ii, bool is, ByteCount b )
			{ isint_ = ii; issigned_ = is; nrbytes_ = b; }
    inline void		set( bool ii, bool is, int b )
			{ isint_ = ii; issigned_ = is;
			  nrbytes_ = nearestByteCount(ii,b); }
    void		setInteger( bool yn )		{ isint_ = yn; }
    void		setSigned( bool yn )		{ issigned_ = yn; }
    void		setNrBytes( ByteCount n )	{ nrbytes_ = n; }
    void		setNrBytes( int n )
			{ nrbytes_ = nearestByteCount(isint_,n); }

			// dump/restore
    virtual int		maxStringifiedSize() const	{ return 18; }
    virtual void	toString(char*) const;
			//!< Into a buffer allocated by client!
    virtual void	set(const char*);
    virtual void	dump(unsigned char&,unsigned char&) const;
    virtual void	set(unsigned char,unsigned char);

			mDeclBinDataDescConstr(signed char,true,true)
			mDeclBinDataDescConstr(short,true,true)
			mDeclBinDataDescConstr(int,true,true)
			mDeclBinDataDescConstr(unsigned char,true,false)
			mDeclBinDataDescConstr(unsigned short,true,false)
			mDeclBinDataDescConstr(unsigned int,true,false)
			mDeclBinDataDescConstr(float,false,true)
			mDeclBinDataDescConstr(double,false,true)

    inline bool		operator ==( const BinDataDesc& dc ) const
			{ return isEqual(dc); }
    inline bool		operator !=( const BinDataDesc& dc ) const
			{ return !isEqual(dc); }
    inline bool		isEqual( const BinDataDesc& dc ) const
			{
			    unsigned char c11=0, c12=0, c21=0, c22=0;
			    dump(c11,c12); dc.dump(c21,c22);
			    return c11 == c21 && c12 == c22;
			}

    int			sizeFor( int n ) const		{ return nrbytes_ * n; }
    virtual bool	convertsWellTo(const BinDataDesc&) const;

    static ByteCount	nearestByteCount( bool is_int, int s )
			{
			    if ( !is_int ) return s > 6 ? N8 : N4;
			    if ( s > 6 ) s = 8;
			    else if ( s > 2 ) s = 4;
			    else if ( s < 2 ) s = 1;
			    return (ByteCount)s;
			}

    static int		nextSize( bool is_int, int s )
			{
			    if ( s < 0 || s > 8 ) return -1;
			    if ( s == 0 )	  return is_int ? 1 : 4;
			    if ( !is_int )	  return s == 4 ? 8 : -1;
			    return s == 1 ? 2 : ( s == 2 ? 4 : 8 );
			}

protected:

    bool		isint_;
    bool		issigned_;
    ByteCount		nrbytes_;

    void		setFrom(unsigned char,bool);

};

#undef mDeclBinDataDescConstr


#endif

