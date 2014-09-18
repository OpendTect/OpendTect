#ifndef trckeyvalue_h
#define trckeyvalue_h

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


#include "trckey.h"

/*!
\brief TrcKey and a value.

*/

mExpClass(Basic) TrcKeyValue
{
public:

    inline		TrcKeyValue( const TrcKey& tk=TrcKey::udf(),
				     float v=mUdf(float) )
			    : tk_(tk)
			    , val_(v)					{}
			TrcKeyValue(const BinIDValue&);


    inline bool		operator==( const TrcKeyValue& oth ) const
			{ return oth.tk_==tk_ && mIsEqual(oth.val_,val_,1e-5); }
    inline bool		operator!=( const TrcKeyValue& oth ) const
			{ return !(*this==oth); }

    TrcKey		tk_;
    float		val_;
};


#endif
