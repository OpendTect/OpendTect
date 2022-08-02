#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	A.H.Bril
 Date:		21-6-1996
 Contents:	Positions: Inline/crossline and Coordinate
________________________________________________________________________

-*/

#include "basicmod.h"


#include "trckey.h"

/*!
\brief TrcKey and a value.
*/

class BinIDValue;

mExpClass(Basic) TrcKeyValue
{
public:

    inline		TrcKeyValue( const TrcKey& tk=TrcKey::udf(),
				     float v=mUdf(float) )
			    : tk_(tk)
			    , val_(v)					{}
    inline		TrcKeyValue( const BinID& bid, float v=mUdf(float) )
			    : tk_(bid)
			    , val_(v)					{}
			TrcKeyValue(const BinIDValue&);

    // mDeprecated Pos::LineID& lineNr()	{ return tk_.lineNr(); }
    Pos::LineID		lineNr() const		{ return tk_.lineNr(); }
    // mDeprecated Pos::TraceID& trcNr()	{ return tk_.trcNr(); }
    Pos::TraceID	trcNr() const		{ return tk_.trcNr(); }

    TrcKeyValue&	setLineNr( Pos::LineID nr )
			{ tk_.setLineNr(nr); return *this; }
    TrcKeyValue&	setTrcNr( Pos::TraceID nr )
			{ tk_.setTrcNr(nr); return *this; }

    inline bool		operator==( const TrcKeyValue& oth ) const
			{ return oth.tk_==tk_ && mIsEqual(oth.val_,val_,1e-5); }
    inline bool		operator!=( const TrcKeyValue& oth ) const
			{ return !(*this==oth); }

    inline bool		isDefined() const;
    inline bool		isUdf() const		{ return !isDefined(); }
    static const TrcKeyValue& udf();

    TrcKey		tk_;
    float		val_;
};

inline bool TrcKeyValue::isDefined() const
{ return !tk_.isUdf() && !mIsUdf(val_); }

