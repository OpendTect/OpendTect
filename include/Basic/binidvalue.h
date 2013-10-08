#ifndef binidvalue_h
#define binidvalue_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	A.H.Bril
 Date:		21-6-1996
 Contents:	Positions: Inline/crossline and Coordinate
 RCS:		$Id$
________________________________________________________________________

-*/

#include "basicmod.h"
#include "binid.h"
class BinIDValues;

/*!
\brief BinID and a value.
*/

mClass(Basic) BinIDValue
{
public:
		BinIDValue( int inl=0, int crl=0, float v=mUdf(float) )
		: binid(inl,crl), value(v)	{}
		BinIDValue( const BinID& b, float v=mUdf(float) )
		: binid(b), value(v)		{}
		BinIDValue(const BinIDValues&,int);

    bool	operator==( const BinIDValue& biv ) const
		{ return biv.binid == binid
		      && mIsEqual(value,biv.value,compareEpsilon()); }
    bool	operator!=( const BinIDValue& biv ) const
		{ return !(*this == biv); }

    BinID	binid;
    float	value;

    static float compareEpsilon()		{ return 1e-5; }
};


/*!
\brief BinID and values. If one of the values is Z, make it the first one.
*/

mExpClass(Basic) BinIDValues
{
public:
			BinIDValues( int inl=0, int crl=0, int n=2 )
			: binid(inl,crl), vals(0), sz(0) { setSize(n); }
			BinIDValues( const BinID& b, int n=2 )
			: binid(b), vals(0), sz(0)	{ setSize(n); }
			BinIDValues( const BinIDValues& biv )
			: vals(0), sz(0)		{ *this = biv; }
			BinIDValues( const BinIDValue& biv )
			: binid(biv.binid), vals(0), sz(0)
					{ setSize(1); value(0) = biv.value; }
			~BinIDValues();

    BinIDValues&	operator =(const BinIDValues&);

    bool		operator==( const BinIDValues& biv ) const;
    			//!< uses BinIDValue::compareepsilon
    inline bool		operator!=( const BinIDValues& biv ) const
			{ return !(*this == biv); }

    BinID		binid;
    int			size() const			{ return sz; }
    float&		value( int idx )		{ return vals[idx]; }
    float		value( int idx ) const		{ return vals[idx]; }
    float*		values()			{ return vals; }
    const float*	values() const			{ return vals; }

    void		setSize(int,bool kpvals=false);
    void		setVals(const float*);

protected:

    float*		vals;
    int			sz;
    static float	udf;

};


#endif
