#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "attributesmod.h"
#include "attribprovider.h"
#include "binid.h"

namespace Attrib
{

/*!
\brief %Shift Attribute
  %Shift takes the input at a specified position and outputs it at other
  relative positions.

<pre>
  %Shift pos= steering=Yes/No

  Input:
  0 - attrib to be hashed
  1 - steering (optional)
  
  %Output:
  0 - hashed attrib
</pre>
*/


mExpClass(Attributes) Shift : public Provider
{
public:
    static void			initClass();
				Shift(Desc&);

    static const char*		attribName()	{ return "Shift"; }
    static const char*		posStr()	{ return "pos"; }
    static const char*		timeStr()	{ return "time"; }
    static const char*		steeringStr()	{ return "steering"; }
    void			initSteering() override
				{ stdPrepSteering(stepout_); }

    void			prepPriorToBoundsCalc() override;

    virtual bool		isSingleTrace() const override
				{ return !stepout_.inl() && !stepout_.crl(); }

protected:
				~Shift();
    static Provider*		createInstance(Desc&);
    static void			updateDesc(Desc&);
    void			init();

    bool			allowParallelComputation() const override
				{ return false; }

    bool			getInputOutput(int inp,
					    TypeSet<int>& res) const override;
    bool			getInputData(const BinID&,int zintv) override;
    bool			computeData(const DataHolder&,
					    const BinID& relpos,
					    int z0,int nrsamples,
					    int threadid) const override;

    const BinID*		reqStepout(int input,int output) const override;
    const Interval<float>*	reqZMargin(int input,int output) const override;
    const Interval<float>*	desZMargin(int input,int output) const override;

    BinID			pos_;
    float			time_;
    bool			dosteer_;
    
    BinID			stepout_;
    Interval<float>		interval_;
    Interval<float>		desinterval_;
    int				dataidx_;
    int				steeridx_;

    const DataHolder*		inputdata_;
    const DataHolder*		steeringdata_;
};

} // namespace Attrib
