#ifndef volstatsattrib_h
#define volstatsattrib_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Kristofer Tingdahl
 Date:          07-10-1999
 RCS:           $Id$
________________________________________________________________________

-*/

#include "attribprovider.h"

/*!\brief Volume Statistics Attribute

VolumeStatistics stepout=1,1 shape=Rectangle|Ellipse|OpticalStack gate=[0,0]
		 steering=

VolumeStatistics collects all samples within the timegate from all traces
within the stepout.

If steering is enabled, the timegate is taken relative to the steering.

If the OpticalStack shape is chosen, the positions used are defined by a step 
and a direction: the line direction or its normal.

Inputs:
0-(nrvolumes-1)         The data
nrvolumes  -            Steerings (only if steering is enabled)

Outputs:
0       Avg
1       Med
2       Variance
3       Min
4       Max
5       Sum
6       Normalized Variance
7	Most Frequent
8	RMS
9	Extreme

*/

namespace Attrib
{

mClass VolStatsBase : public Provider
{
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

    virtual void		prepPriorToBoundsCalc();
    virtual void		initSteering() { stdPrepSteering(stepout_); }
    bool			isSingleTrace() const
				{ return !stepout_.inl && !stepout_.crl; }

protected:
				VolStatsBase(Desc&);

    void			init();
    int*			outputTypes() const;

    static void			updateDefaults(Desc&);

    bool			allowParallelComputation() const
				{ return true; }

    virtual bool		getInputOutput(int,TypeSet<int>& res) const;
    virtual bool		getInputData(const BinID&,int zintv);

    virtual bool		computeData(const DataHolder&,
	    				    const BinID& relpos,
					    int z0,int nrsamples,
					    int threadid) const = 0;

    const BinID*		desStepout(int input,int output) const;
    const Interval<float>* 	desZMargin( int inp, int ) const;
    virtual const Interval<float>* reqZMargin(int input,int output) const;

    BinID			stepout_;
    int				shape_;
    Interval<float>		gate_;
    Interval<float>             desgate_;
    int				minnrtrcs_;

    TypeSet<BinID>      	positions_;
    int				dataidx_;

    ObjectSet<const DataHolder>	inputdata_;
    const DataHolder*           steeringdata_;
};



mClass VolStats : public VolStatsBase
{
public:
    static void			initClass();
				VolStats(Desc&);

    static const char*		attribName()	  { return "VolumeStatistics"; }
    static const char*		allowEdgeEffStr() { return "allowedgeeffects"; }
    static const char*          optstackstepStr() { return "optstackstep"; }
    static const char*          optstackdirStr()  { return "optstackdir"; }
    static const char*          optStackDirTypeStr(int);

    void			prepPriorToBoundsCalc();
    void			setRdmPaths( TypeSet<BinID>* truepos,
	    				     TypeSet<BinID>* snappedpos )
			        { linetruepos_ = truepos
					    ? new TypeSet<BinID>(*truepos)
					    : new TypeSet<BinID>();
				  linepath_ = snappedpos
					    ? new TypeSet<BinID>(*snappedpos)
					    : new TypeSet<BinID>(); }

protected:
				~VolStats();

    static Provider*		createInstance(Desc&);
    static void			updateDesc(Desc&);

    virtual bool		getInputData(const BinID&,int zintv);
    virtual bool		getInputOutput(int,TypeSet<int>& res) const;

    virtual bool		computeData(const DataHolder&,
	    				    const BinID& relpos,
					    int z0,int nrsamples,
					    int threadid) const;

    void			getStackPositions(TypeSet<BinID>&) const;
    void			getIdealStackPos(
	    				const BinID&,const BinID&,const BinID&,
				  	TypeSet< Geom::Point2D<float> >&) const;
    void			reInitPosAndSteerIdxes();

    const Interval<float>*	reqZMargin(int input,int output) const;

    bool			dosteer_;
    bool			allowedgeeffects_;
    Interval<float>             desgate_;

    TypeSet<int>		steerindexes_;
    TypeSet<BinID>*		linepath_;
    TypeSet<BinID>*		linetruepos_;
    int				optstackdir_;
    int				optstackstep_;
};

} // namespace Attrib


#endif
