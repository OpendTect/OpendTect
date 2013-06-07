#ifndef positionattrib_h
#define positionattrib_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Nanne Hemstra
 Date:          November 2002
 RCS:           $Id$
________________________________________________________________________

-*/

#include "attribprovider.h"
#include "position.h"
#include "arrayndimpl.h"

/*!\brief Position Attribute

Position stepout=2,2 gate=[0,0] oper=Min|Max steering=

Calculates 'attribute 0' on every position within the cube defined by 'stepout'
and 'gate'. Then determines at which position the oper is valid.
At this position 'attribute 1' is calculated.


Input:
0       Input attribute
1       Output attribute

Output:
0       Value of output attribute on statistical calculated position

*/

namespace Attrib
{

mClass Position : public Provider
{
public:
    static void			initClass();
				Position(Desc&);

    static const char*		attribName()	{ return "Position"; }
    static const char*		stepoutStr()	{ return "stepout"; }
    static const char*		gateStr()	{ return "gate"; }
    static const char*  	operStr()	{ return "oper"; }
    static const char*		steeringStr()	{ return "steering"; }
    static const char*		operTypeStr(int);
    void			initSteering()	{ stdPrepSteering(stepout_); }

    void			prepPriorToBoundsCalc();
    virtual bool		isSingleTrace() const
				{ return !stepout_.inl && !stepout_.crl; }

protected:
    				~Position();
    static Provider*		createInstance(Desc&);
    static void			updateDesc(Desc&);

    bool			allowParallelComputation() const
				{ return true; }

    bool			getInputOutput(int inp,TypeSet<int>& res) const;
    bool			getInputData(const BinID&,int zintv);
    bool			computeData(const DataHolder&,
	    				    const BinID& relpos,
					    int z0,int nrsamples,
					    int threadid) const;

    const BinID*		reqStepout(int input,int output) const
    				{ return &stepout_; }
    const Interval<float>*	reqZMargin(int input,int output) const;
    const Interval<float>*	desZMargin(int input,int output) const;

    BinID			stepout_;
    Interval<float>		gate_;
    int				oper_;
    bool			dosteer_;

    TypeSet<BinID>              positions_;
    Interval<float>             desgate_;

    int				inidx_;
    int				outidx_;
    TypeSet<int>		steerindexes_;

    ObjectSet<const DataHolder>	inputdata_;
    Array2DImpl<const DataHolder*>* outdata_;
    const DataHolder*		steerdata_;
};

} // namespace Attrib


#endif
