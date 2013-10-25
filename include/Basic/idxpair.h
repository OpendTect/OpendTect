#ifndef idxpair_h
#define idxpair_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Bert
 Date:		Oct 2013
 RCS:		$Id$
________________________________________________________________________

-*/

#include "basicmod.h"
#include "gendefs.h"
#include <utility>


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

    bool			isUdf() const	{ return *this == udf(); }
    void			setUdf()	{ *this = udf(); }
    static const IdxPair&	udf();

    inline IdxType&		operator[]( int idx )
				{ return idx ? second : first; }
    inline IdxType		operator[]( int idx ) const
				{ return idx ? second : first; }

    const char*			getUsrStr(const char* prefx,const char* sep,
				    const char* postfx,bool only2nd) const;
    bool			parseUsrStr(const char* str,const char* prefx,
				    const char* sep,const char* postfx);
};



inline bool IdxPair::operator ==( const IdxPair& oth ) const
{
    return first == oth.first && second == oth.second;
}


#endif
