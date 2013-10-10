#ifndef posidxpair_h
#define posidxpair_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Bert/Salil
 Date:		Oct 2013
 RCS:		$Id$
________________________________________________________________________

-*/

#include "basicmod.h"
#include "gendefs.h"
#include <utility>


namespace Pos
{

typedef std::pair<Index_Type,Index_Type> Index_Type_Pair;
typedef IdxPair IdxPairDelta;
typedef IdxPair IdxPairStep;


/*!\brief A pair of numbers; base class for BinID et al. */

mExpClass(Basic) IdxPair : public Index_Type_Pair
{
public:

    typedef Index_Type		IdxType;

    				IdxPair() : Index_Type_Pair(0,0)	{}
    				IdxPair( IdxType f, IdxType s )
				    : Index_Type_Pair(f,s)		{}
    inline bool			operator ==(const IdxPair&) const;
    inline bool			operator !=( const IdxPair& oth ) const
						{ return !(*this == oth); }

    				// aliases for first
    inline IdxType&		inl()		{ return first; }
    inline IdxType&		lineNr()	{ return first; }
    inline IdxType&		row()		{ return first; }

    				// aliases for second
    inline IdxType&		crl()		{ return second; }
    inline IdxType&		trcNr()		{ return second; }
    inline IdxType&		col()		{ return second; }

    				// const versions of the aliases
    inline IdxType		inl() const	{ return first; }
    inline IdxType		crl() const	{ return second; }
    inline IdxType		lineNr() const	{ return first; }
    inline IdxType		trcNr() const	{ return second; }
    inline IdxType		row() const	{ return first; }
    inline IdxType		col() const	{ return second; }

    inline IdxType&		operator[]( int idx )
				{ return idx ? second : first; }
    inline IdxType		operator[]( int idx ) const
				{ return idx ? second : first; }

    inline od_int64		toInt64() const;
    inline static IdxPair	fromInt64(od_int64);
    od_int64			sqDistTo(const IdxPair&) const;
    bool			isNeighborTo(const IdxPair&,
	    			 const IdxPairStep&,bool conn8=true) const;

    inline bool			isUdf() const	{ return *this == udf(); }
    static const IdxPair&	udf();

protected:

    const char*			getUsrStr(const char* prefx,const char* sep,
				    const char* postfx,bool only2nd) const;
    bool			parseUsrStr(const char* str,const char* prefx,
				    const char* sep,const char* postfx);

};



inline bool IdxPair::operator ==( const IdxPair& oth ) const
{
    return first == oth.first && second == oth.second;
}


inline od_int64 IdxPair::toInt64() const
{
    return (((od_uint64)first) << 32) + (((od_uint64)second) & 0xFFFFFFFF);
}


inline IdxPair IdxPair::fromInt64( od_int64 i64 )
{
    return IdxPair( (IdxType)(i64 >> 32), (IdxType)(i64 & 0xFFFFFFFF) );
}


} // namespace Pos

#endif
