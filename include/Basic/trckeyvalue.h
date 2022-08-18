#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
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
