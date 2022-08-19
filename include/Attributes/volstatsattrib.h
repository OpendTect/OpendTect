#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "attributesmod.h"
#include "attribprovider.h"

namespace Attrib
{

/*!
\brief Use VolStats instead.
*/

mExpClass(Attributes) VolStatsBase : public Provider
{ mODTextTranslationClass(VolStatsBase);
public:
    static void			initDesc(Desc&);

    static const char*		nrvolumesStr()	  { return "nrvolumes"; }
    static const char*		stepoutStr()	  { return "stepout"; }
    static const char*		shapeStr()	  { return "shape"; }
    static const char*		gateStr()	  { return "gate"; }
    static const char*		absolutegateStr() { return "absolutegate"; }
    static const char*		nrtrcsStr()	  { return "nrtrcs"; }
    static const char*		steeringStr()	  { return "steering"; }
    static const char*		shapeTypeStr(int);

    void			prepPriorToBoundsCalc() override;
    void			initSteering() override
				{ stdPrepSteering(stepout_); }
    bool			isSingleTrace() const override
				{ return !stepout_.inl() && !stepout_.crl(); }

protected:
				VolStatsBase(Desc&);

    void			init();
    int*			outputTypes() const;

    static void			updateDefaults(Desc&);

    bool			allowParallelComputation() const override
				{ return true; }

    bool			getInputOutput(int,
					    TypeSet<int>& res) const override;
    bool			getInputData(const BinID&,int zintv) override;

    bool			computeData(const DataHolder&,
					    const BinID& relpos,
					    int z0,int nrsamples,
					    int threadid) const override = 0;

    const BinID*		desStepout(int input,int output) const override;
    const Interval<float>*	desZMargin( int inp, int ) const override;
    const Interval<float>*	reqZMargin(int input,
					    int output) const override;

    BinID			stepout_;
    int				shape_;
    Interval<float>		gate_;
    Interval<float>		desgate_;
    int				minnrtrcs_;

    TypeSet<BinID>		positions_;
    int				dataidx_;

    ObjectSet<const DataHolder> inputdata_;
    const DataHolder*		steeringdata_;
};


/*!
\brief Volume Statistics Attribute

  VolumeStatistics collects all samples within the timegate from all traces
  within the stepout.

  If steering is enabled, the timegate is taken relative to the steering.

  If the OpticalStack shape is chosen, the positions used are defined by a step
  and a direction: the line direction or its normal.

<pre>
  VolumeStatistics stepout=1,1 shape=Rectangle|Ellipse|OpticalStack gate=[0,0]
		   steering=
  Inputs:
  0-(nrvolumes-1)	  The data
  nrvolumes  -		  Steerings (only if steering is enabled)

  Outputs:
  0	  Avg
  1	  Med
  2	  Variance
  3	  Min
  4	  Max
  5	  Sum
  6	  Normalized Variance
  7	  Most Frequent
  8	  RMS
  9	  Extreme
</pre>
*/

mExpClass(Attributes) VolStats : public VolStatsBase
{ mODTextTranslationClass(VolStats);
public:
    static void			initClass();
				VolStats(Desc&);

    static const char*		attribName()	  { return "VolumeStatistics"; }
    static const char*		allowEdgeEffStr() { return "allowedgeeffects"; }
    static const char*		optstackstepStr() { return "optstackstep"; }
    static const char*		optstackdirStr()  { return "optstackdir"; }
    static const char*		optStackDirTypeStr(int);

    void			prepPriorToBoundsCalc() override;
    void			setRdmPaths(const TypeSet<BinID>& truepos,
					    const TypeSet<BinID>& snappedpos)
					    override;

protected:
				~VolStats();

    static Provider*		createInstance(Desc&);
    static void			updateDesc(Desc&);

    bool			getInputData(const BinID&,int zintv) override;
    bool			getInputOutput(int,
					    TypeSet<int>& res) const override;

    bool			computeData(const DataHolder&,
					    const BinID& relpos,
					    int z0,int nrsamples,
					    int threadid) const override;

    void			getStackPositions(TypeSet<BinID>&) const;
    void			getIdealStackPos(
					const BinID&,const BinID&,const BinID&,
					TypeSet< Geom::Point2D<float> >&) const;
    void			reInitPosAndSteerIdxes();

    const Interval<float>*	reqZMargin(int input,int output) const override;

    bool			dosteer_;
    bool			allowedgeeffects_;

    TypeSet<int>		steerindexes_;
    TypeSet<BinID>		linepath_;
    TypeSet<BinID>		linetruepos_;
    int				optstackdir_;
    int				optstackstep_;
};

} // namespace Attrib
