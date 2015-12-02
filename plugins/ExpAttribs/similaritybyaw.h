#ifndef SimilaritybyAW_h
#define SimilaritybyAW_h


/*+
________________________________________________________________________
 
 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Haibin Di
 Date:          August 2013
 RCS:           $Id$ 
 ________________________________________________________________________
-*/

#include "expattribsmod.h"
#include "attribprovider.h"

namespace Attrib 
{

mExpClass(ExpAttribs) SimilaritybyAW : public Provider
{
public:
			SimilaritybyAW(Desc&);

    static void		initClass();
    void		initSteering();
    
    static const char*	attribName()	{ return "SimilaritybyAW"; }
    static const char*	refTimeGateStr(){ return "refTimeGate"; }
    static const char*	searchRangeStr(){ return "searchRange"; }
    static const char*	stepoutStr()	{ return "Stepout"; }

protected:
			~SimilaritybyAW()	{} 

    static Provider*	createInstance(Desc&);
    static void		updateDesc(Desc&);

    bool		allowParallelComputation() const   { return true; }
    bool		getInputOutput(int input,TypeSet<int>& res) const;
    bool		getInputData(const BinID&,int zintv);
    bool 		computeData(const DataHolder&,const BinID& relpos,
				int z0,int nrsamples,int threadid) const;
    float		calSimilaritybyAW(float *) const;
    float		calSimilarity(float *,float *,int) const;

    const BinID*	desStepout(int,int) const;
    const Interval<int>* desZSampMargin(int,int) const;

    int			attribute_;
    Interval<float>	reftimegate_;
    Interval<float>	searchrange_;

    BinID		stepout_;
    BinID		horgate_;
    Interval <int>	vergate_;
    Interval <int>	desgate_;
    int			inlstep_;
    int 		crlstep_;
    int 		verstep_;
    int			verstep0_;
    int			verstep1_;
    int			desstep_;

    int			dataidx_;
    bool		dosteer_;
    const DataHolder*	steeringdata_;

    struct PosAndSteeridx
    {
	TypeSet<int>	steeridx_;
	TypeSet<BinID>	pos_;
    }			posandsteeridx_;

    ObjectSet <const DataHolder> inputdata_;
};

}

#endif
