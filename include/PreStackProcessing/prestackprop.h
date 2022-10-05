#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "prestackprocessingmod.h"

#include "datapack.h"
#include "enums.h"
#include "prestackgather.h"
#include "ranges.h"
#include "stattype.h"

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
    enum LSQType	{ A0, Coeff, StdDevA0, StdDevCoeff,
		          CorrCoeff };
			mDeclareEnumUtils(LSQType)

    mExpClass(PreStackProcessing) Setup
    {
    public:
			Setup();
			~Setup();

	mDefSetupMemb(CalcType,calctype)	// def: Least-Square
	mDefSetupMemb(Stats::Type,stattype)	// def: Average
	mDefSetupMemb(LSQType,lsqtype)		// def: A0
	mDefSetupMemb(AxisType,offsaxis)	// def: Norm
	mDefSetupMemb(AxisType,valaxis)		// def: Norm
	mDefSetupMemb(Interval<float>,offsrg)	// def: unlimited
	mDefSetupMemb(float,xscaler)		// def: 1
	mDefSetupMemb(Interval<int>,anglerg)	// def: [0, 30]
	mDefSetupMemb(bool,useangle)		// def: false
	mDefSetupMemb(int,aperture)		// def: 1
    };
			PropCalc(const Setup&);
    virtual		~PropCalc();

    Setup&		setup()				{ return setup_; }
    const Setup&	setup() const			{ return setup_; }
    const Gather*	getGather() const               { return gather_; }

    bool		hasAngleData() const		{ return angledata_; }
    void		setGather(DataPackID);
    void		setGather(const PreStack::Gather&);
    void		setAngleData(DataPackID);
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

    ConstRefMan<Gather> gather_ = nullptr;
    int*		innermutes_ = nullptr;
    int*		outermutes_ = nullptr;
    ConstRefMan<Gather> angledata_ = nullptr;;

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
