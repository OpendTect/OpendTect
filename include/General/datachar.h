#ifndef datachar_H
#define datachar_H

/*
________________________________________________________________________

 CopyRight:	(C) de Groot-Bril Earth Sciences B.V.
 Author:	A.H.Bril
 Date:		Nov 2000
 Contents:	Binary data interpretation
 RCS:		$Id: datachar.h,v 1.1 2001-02-13 17:46:49 bert Exp $
________________________________________________________________________

*/


#include <general.h>


/*!\brief Characteristics of data for byte-level data interpretation.

Interpretation (read or write) of data in buffers according to specified
characteristics. The Ibm Format is only supported for the types that are used
in SEG-Y sample data handling.
*/

#define mDeclConstr(T,t,f,i) \
	DataCharacteristics( T* ) \
	: type(t), fmt(f), nrbytes(sizeof(T)), issigned(i), \
	  littleendian(__islittle__)	{}


class DataCharacteristics
{
public:

    enum Type		{ Integer, Float };
    enum Format		{ Ieee, Ibm };

    Type		type;
    Format		fmt;
    int			nrbytes;
    bool		issigned;
    bool		littleendian;

			DataCharacteristics( Type t=Float, Format f=Ieee,
					     int n=4, bool i=true,
					     bool l=__islittle__ )
			: type(t), fmt(f), nrbytes(n), issigned(i)
			, littleendian(l)			{}
    inline bool		isInt() const		{ return type == Integer; }
    inline bool		isIeee() const		{ return fmt == Ieee; }

			DataCharacteristics( unsigned char c )	{ set(c); }
			DataCharacteristics( const char* s )	{ set(s); }

    unsigned char	dump() const;
    BufferString	toString() const;
    void		set(unsigned char);
    void		set(const char*);

			mDeclConstr(signed char,Integer,Ieee,true)
			mDeclConstr(int,Integer,Ieee,true)
			mDeclConstr(short,Integer,Ieee,true)
			mDeclConstr(long long,Integer,Ieee,true)
			mDeclConstr(unsigned char,Integer,Ieee,false)
			mDeclConstr(unsigned short,Integer,Ieee,false)
			mDeclConstr(unsigned int,Integer,Ieee,false)
			mDeclConstr(unsigned long long,Integer,Ieee,false)
			mDeclConstr(float,Float,Ieee,true)
			mDeclConstr(double,Float,Ieee,true)

    bool		operator ==( const DataCharacteristics& dc ) const
			{ return dump() == dc.dump(); }
    bool		operator !=( const DataCharacteristics& dc ) const
			{ return dump() != dc.dump(); }

    int			sizeFor( int n ) const		{ return nrbytes * n; }
    bool		convertsWellTo(const DataCharacteristics&) const;
    bool		needSwap() const
			{ return nrbytes > 1 && littleendian != __islittle__; }

    static bool		canBeUnsigned( Type t )
			{ return t != Float; }
    static int		snappedSize( Type t, int s )
			{
			    if ( s < 2 ) s = 1;
			    else if ( s > 6 ) s = 8;
			    else if ( s > 2 ) s = 4;
			    return s;
			}
    static int		nextSize( Type t, int s )
			{
			    if ( s < 0 || s > 4 ) return -1;
			    if ( s == 0 )	  return t == Float ? 4 : 1;
			    if ( t == Float )	  return s == 4 ? 8 : -1;
			    return s == 1 ? 2 : (s == 2 ? 4 : 8);
			}
};


#endif
