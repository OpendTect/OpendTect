#ifndef curvgrad_h
#define curvgrad_h

/*+
  ________________________________________________________________________
 
 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Haibin Di
 Date:          July 2013
 RCS:           $Id$
  ________________________________________________________________________
-*/


#include "expattribsmod.h"
#include "attribprovider.h"


namespace Attrib 
{

mExpClass(ExpAttribs) CurvGrad: public Provider
{

public:
			CurvGrad(Desc&);

    static void		initClass();
    void		initSteering();

    static const char*	attribName()	{ return "CurvGrad"; }
    static const char*	stepoutStr()	{ return "Stepout"; }

protected:
			~CurvGrad()	{}

    static Provider*	createInstance(Desc&);
    static void		updateDesc(Desc&);

    bool		allowParallelComputation() const   { return true; }
    bool		getInputOutput(int input,TypeSet<int>& res) const;
    bool		getInputData(const BinID&,int zintv);
    bool 		computeData(const DataHolder&,const BinID& relpos,
				int z0,int nrsamples,int threadid) const;
    float		calCurvGrad(float *) const;

    const BinID*	desStepout(int,int) const;
    const Interval<int>* desZSampMargin(int,int) const;

    int			attribute_;
    BinID		stepout_;
    Interval<int>	sampgate_;
    
    int			dataidx_;
    bool		dosteer_;
    const DataHolder*	steeringdata_;

    struct PosAndSteeridx
    {
	TypeSet<int>	steeridx_;
	TypeSet<BinID>	pos_;
    }			posandsteeridx_;

    ObjectSet<const DataHolder>	inputdata_;
};

} //namespace Attrib

#endif
