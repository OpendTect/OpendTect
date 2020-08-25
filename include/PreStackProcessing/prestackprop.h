#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	A.H. Bril
 Date:		Jan 2008
 RCS:		$Id$
________________________________________________________________________

-*/

#include "prestackprocessingmod.h"

#include "datapack.h"
#include "enums.h"
#include "ranges.h"
#include "stattype.h"

class SeisPSReader;

namespace PreStack
{

class Gather;

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
    enum LSQType	{ A0, Coeff, StdDevA0, StdDevCoeff,
		          CorrCoeff };
			mDeclareEnumUtils(LSQType)

    mExpClass(PreStackProcessing) Setup
    {
    public:
			Setup()
			    : calctype_(LLSQ)
			    , stattype_(Stats::Average)
			    , lsqtype_(A0)
			    , offsaxis_(Norm)
			    , valaxis_(Norm)
			    , offsrg_(0,mUdf(float))
			    , xscaler_(1.f)
			    , anglerg_(0,30)
			    , useangle_(false)
			    , aperture_(0)  {}

	mDefSetupMemb(CalcType,calctype)
	mDefSetupMemb(Stats::Type,stattype)
	mDefSetupMemb(LSQType,lsqtype)
	mDefSetupMemb(AxisType,offsaxis)
	mDefSetupMemb(AxisType,valaxis)
	mDefSetupMemb(Interval<float>,offsrg)
	mDefSetupMemb(float,xscaler)
	mDefSetupMemb(Interval<int>,anglerg)
	mDefSetupMemb(bool,useangle)
	mDefSetupMemb(int,aperture)
    };
			PropCalc(const Setup&);
    virtual		~PropCalc();

    Setup&		setup()				{ return setup_; }
    const Setup&	setup() const			{ return setup_; }
    const Gather*	getGather() const               { return gather_; }

    bool		hasAngleData() const		{ return angledata_; }
    void		setGather(DataPack::ID);
    void		setGather(const PreStack::Gather&);
    void		setAngleData(DataPack::ID);
    void		setAngleData(const PreStack::Gather&);
			/*!< Only used if AngleA0 or AngleCoeff. If not set,
			     offset values from traces will be assumed to
			     contain angles. */
    float		getVal(int sampnr) const;
    float		getVal(float z) const;

    static float	getVal(const PropCalc::Setup& su,
			       TypeSet<float>& vals,TypeSet<float>& offs);

protected:

    void		removeGather();

    Gather*		gather_;
    int*		innermutes_;
    int*		outermutes_;
    Gather*		angledata_;

    Setup		setup_;
	bool		anglevalinradians_ = false;
	Interval<float>	axisvalsrg_;

private:

    void		init();
    bool		getAngleFromMainGather() const;
    void		handleNewGather();

public:

	void		setAngleValuesInRadians( bool yn )
				{ anglevalinradians_ = yn; }
};

} // namespace PreStack

