#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Bert
 Date:		Jan 2008
________________________________________________________________________

-*/

#include "prestackprocessingmod.h"

#include "datapack.h"
#include "enums.h"
#include "ranges.h"
#include "stattype.h"

class Gather;
class SeisPSReader;

namespace PreStack
{

/*!
\brief Calculates 'post-stack' properties of a PreStack data store.
*/

mExpClass(PreStackProcessing) PropCalc
{
public:

    enum CalcType	{ Stats, LLSQ };
			mDeclareEnumUtils(CalcType)
    enum AxisType	{ Norm, Log, Exp, Sqr, Sqrt, Abs, Sinsq };
			mDeclareEnumUtils(AxisType)
    enum LSQType	{ A0, Coeff, StdDevA0, StdDevCoeff, CorrCoeff };
			mDeclareEnumUtils(LSQType)

    mExpClass(PreStackProcessing) Setup
    {
    public:
			Setup()
			    : calctype_(Stats)
			    , stattype_(Stats::Average)
			    , lsqtype_(A0)
			    , offsaxis_(Norm)
			    , valaxis_(Norm)
			    , offsrg_(0.f,mUdf(float))
			    , anglerg_(Interval<float>::udf()) //Degrees
			    , aperture_(0)  {}

	mDefSetupMemb(CalcType,calctype)
	mDefSetupMemb(Stats::Type,stattype)
	mDefSetupMemb(LSQType,lsqtype)
	mDefSetupMemb(AxisType,offsaxis)
	mDefSetupMemb(AxisType,valaxis)
	mDefSetupMemb(Interval<float>,offsrg)
	mDefSetupMemb(float,xscaler)
	mDefSetupMemb(Interval<float>,anglerg)
	mDefSetupMemb(int,aperture)
    };
			PropCalc(const Setup&);
    virtual		~PropCalc();

    Setup&		setup()				{ return setup_; }
    const Setup&	setup() const			{ return setup_; }
    const Gather*	getGather() const               { return gather_; }

    bool		hasAngleData() const		{ return angledata_; }
    void		setGather(const Gather&);
    void		setAngleData(const Gather&);
			    /*!< Only used if AngleA0 or AngleCoeff. If not set,
				 offset values from traces will be assumed to
				 contain angles. */
    float		getVal(int sampnr) const;
    float		getVal(float z) const;

    static float	getVal(const PropCalc::Setup&,TypeSet<float>& yvals,
			       TypeSet<float>* xvals);

private:

    Setup		setup_;
    ConstRefMan<Gather>	gather_;
    ConstRefMan<Gather>	angledata_;
    int*		innermutes_;
    int*		outermutes_;

    Interval<float>	axisvalsrg_;
    bool		scalexvals_		= false;

    void		removeGather();
    void		gatherChanged();
    void		init();
    bool		getAngleFromMainGather() const;

};

} // namespace PreStack
