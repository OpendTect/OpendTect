#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "tutmod.h"
#include "attribprovider.h"

/*!\brief Tutorial Attribute

Input:
0               Data

Outputs:
0               The scaled trace
*/
    

namespace Attrib
{

mExpClass(Tut) Tutorial : public Provider
{
public:
    static void		initClass();
			Tutorial(Desc&);

    static const char*	attribName()		{ return "Tutorial"; }
    static const char*	actionStr()		{ return "action"; }
    static const char*	shiftStr()		{ return "shift"; }
    static const char*	factorStr()		{ return "factor"; }
    static const char*	weaksmoothStr()		{ return "smoothstrength"; }
    static const char*  horsmoothStr()          { return "smoothdir"; }
    static const char*  steeringStr()   	{ return "steering"; }
    static const char*  stepoutStr()      	{ return "stepout"; }
    void		initSteering() override { stdPrepSteering(stepout_); }

protected:

			~Tutorial();
    static Provider*	createInstance(Desc&);
    static void		updateDesc(Desc&);

    bool		allowParallelComputation() const override
							      { return true; }

    bool		getInputOutput(int input,
					    TypeSet<int>& res) const override;
    bool		getInputData(const BinID&,int zintv) override;
    bool		computeData(const DataHolder&,const BinID& relpos,
			    int z0,int nrsamples,int threadid) const override;
    const BinID*	desStepout(int input,int output) const override;
    const Interval<int>*   desZSampMargin(int input,int output) const override;

    int			action_;
    float		factor_;
    float		shift_;
    bool		weaksmooth_;
    bool                horsmooth_;
    Interval<int>	sampgate_;
    BinID               stepout_;

    struct PosAndSteeridx
    {
	TypeSet<int>	steeridx_;
	TypeSet<BinID>	pos_;
    };

    PosAndSteeridx	posandsteeridx_;

    ObjectSet<const DataHolder> inpdata_;
    int				dataidx_;
    const DataHolder*   	steeringdata_;
};

} // namespace Attrib
