#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "wellmod.h"

#include "bufstring.h"
#include "color.h"
#include "ranges.h"

namespace Well
{

/*!
\brief Log display parameters.
*/

mExpClass(Well) LogDisplayPars
{
public:
			LogDisplayPars(const char* nm=nullptr);
			~LogDisplayPars();

    BufferString	name_;
    float		cliprate_		= mUdf(float);
						//!< If undef, use range_
    Interval<float>	range_;		//!< If cliprate_ set, filled using it
    bool		logarithmic_		= false;
    bool		seisstyle_		= false;
    bool		nocliprate_		= false;
    bool		logfill_		= false;
    int			repeat_			= 1;
    float		repeatovlap_		= mUdf(float);
    OD::Color		linecolor_		= OD::Color::White();
    OD::Color		logfillcolor_		= OD::Color::White();
    const char*		seqname_		= "";
    bool		singlfillcol_		= false;
};


/*!
\brief Log display parameter set.
*/

mExpClass(Well) LogDisplayParSet
{
public:
			LogDisplayParSet();
			~LogDisplayParSet();

    LogDisplayPars*	getLeft() const			{ return leftlogpar_; }
    LogDisplayPars*	getRight() const		{ return rightlogpar_; }
    void		setLeft( LogDisplayPars* lp )	{ leftlogpar_ = lp; }
			//!<Becomes mine
    void		setRight( LogDisplayPars* rp )	{ rightlogpar_ = rp; }
			//!<Becomes mine

protected:

    LogDisplayPars*	leftlogpar_;
    LogDisplayPars*	rightlogpar_;

};

} // namespace Well
