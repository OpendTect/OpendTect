#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Bert
 Date:		Oct 2013
________________________________________________________________________

-*/

#include "basicmod.h"
#include "gendefs.h"


typedef Twins<Index_Type> Index_Type_Pair;
typedef IdxPair IdxPairDelta;
typedef IdxPair IdxPairStep;


/*!\brief A pair of numbers; base class for BinID et al. */

mExpClass(Basic) IdxPair : public Index_Type_Pair
{
public:

    typedef Index_Type		pos_type;

				IdxPair() : Index_Type_Pair(0,0)	{}
				IdxPair( pos_type f, pos_type s )
				    : Index_Type_Pair(f,s)		{}
				mImplSimpleEqOpers1Memb(IdxPair,pair_)

    bool			isUdf() const	{ return *this == udf(); }
    void			setUdf()	{ *this = udf(); }
    static const IdxPair&	udf();

    const char*			getUsrStr(const char* prefx,const char* sep,
				    const char* postfx,bool only2nd) const;
    bool			parseUsrStr(const char* str,const char* prefx,
				    const char* sep,const char* postfx);
};
