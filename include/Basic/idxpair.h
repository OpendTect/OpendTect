#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "basicmod.h"
#include "gendefs.h"
#ifdef __win__
# include <tuple>
#endif
#include <utility>

using Index_Type_Pair = std::pair<Index_Type,Index_Type>;


/*!\brief A pair of numbers; base class for BinID et al. */

mExpClass(Basic) IdxPair : public Index_Type_Pair
{
public:
using IdxType = Index_Type;

				IdxPair();
				IdxPair(IdxType,IdxType);
				~IdxPair();

    inline bool			operator ==(const IdxPair&) const;
    inline bool			operator !=( const IdxPair& oth ) const
						{ return !(*this == oth); }

    bool			isUdf() const	{ return *this == udf(); }
    void			setUdf()	{ *this = udf(); }
    static const IdxPair&	udf();

    inline od_int32&		operator[]( int idx )
				{ return idx ? second : first; }
    inline od_int32		operator[]( int idx ) const
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
