#ifndef textureattrib_h
#define textureattrib_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        P.F.M. de Groot
 Date:          September 2012
 RCS:           $Id$
________________________________________________________________________

-*/


#include "attribprovider.h"

/*!\Texture Attribute

Input:
0               Data

Outputs:
0               Texture attributes
*/
    

namespace Attrib
{

class Texture : public Provider
{
public:
    static void		initClass();
			Texture(Desc&);
    static const char*	attribName()		{ return "Texture"; }
    static const char*	actionStr()			{ return "action"; }
    static const char*  steeringStr()   	{ return "steering"; }
    static const char*  stepoutStr()      	{ return "stepout"; }
    static const char*  gateStr()			{ return "gate"; }
    static const char*	glcmsizeStr()		{ return "glcmsize"; }
    static const char*	scalingtypeStr()	{ return "scalingtype"; }
    static const char*	globalmeanStr()		{ return "globalmean"; }
    static const char*	globalstdevStr()	{ return "globalstdev"; }

    void		initSteering()		{ stdPrepSteering(stepout_); }
    void		prepareForComputeData();

protected:

			~Texture() {}
    static Provider*	createInstance(Desc&);
    static void		updateDesc(Desc&);
    static void		updateDefaults(Desc&);

    bool		allowParallelComputation() const; // { return false; } 

    bool		getInputOutput(int input,TypeSet<int>& res) const;
    bool		getInputData(const BinID&,int zintv);
    bool		computeData(const DataHolder&,const BinID& relpos,
				    int z0,int nrsamples,int threadid) const;
    const BinID*        desStepout(int,int) const;
    const Interval<int>* desZSampMargin(int,int) const;
    int			scaleVal(float) const;
    void		setFactorShift(float,float);

    int			action_;
    int			glcmsize_;
    int			firsttracescaling_;
    float		factor_;
    float		shift_;
    float		globalmean_;
    float		globalstdev_;
    bool		matrix_;
    bool		scalingtype_;

    Interval<int>	sampgate_;
    Interval<float>	gate_;
    Interval<int>	dessampgate_;
    BinID		stepout_;

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

}; // namespace Attrib

#endif
