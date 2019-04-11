#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Bert
 Date:		21-6-1996
 Contents:	Positions: Inline/crossline and Coordinate
________________________________________________________________________

-*/

#include "basicmod.h"


#include "trckey.h"

/*!\brief TrcKey and a value. */

class BinIDValue;

mExpClass(Basic) TrcKeyValue
{
public:

    mUseType( TrcKey,	linenr_type );
    mUseType( TrcKey,	trcnr_type );

    inline		TrcKeyValue( const TrcKey& tk=TrcKey::udf(),
				     float v=mUdf(float) )
			    : tk_(tk)
			    , val_(v)					{}
			TrcKeyValue(const BinIDValue&);

    linenr_type		lineNr() const		{ return tk_.lineNr(); }
    trcnr_type		trcNr() const		{ return tk_.trcNr(); }
    TrcKeyValue&	setLineNr( linenr_type nr )
			{ tk_.setLineNr(nr); return *this; }
    TrcKeyValue&	setTrcNr( trcnr_type nr )
			{ tk_.setTrcNr(nr); return *this; }

    inline bool		operator==( const TrcKeyValue& oth ) const
			{ return oth.tk_==tk_ && mIsEqual(oth.val_,val_,1e-5); }
    inline bool		operator!=( const TrcKeyValue& oth ) const
			{ return !(*this==oth); }

    inline bool		isDefined() const;
    inline bool		isUdf() const		{ return !isDefined(); }
    static const TrcKeyValue& udf();
    inline void		setUdf()		{ *this = udf(); }

    TrcKey		tk_;
    float		val_;

};

inline bool TrcKeyValue::isDefined() const
{
    return !tk_.isUdf() && !mIsUdf(val_);
}
