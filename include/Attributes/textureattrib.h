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
#include "arraynd.h"

/*!
\brief %Texture Attribute
Texture Attribute definitions from
http://www.fp.ucalgary.ca/mhallbey/equations.htm

<pre>
Input:
0               Data

Outputs:
0               Texture attributes
</pre>
*/

namespace Attrib
{

mClass(TextureAttrib) Texture : public Provider
{
public:
    static void		initClass();
			Texture(Desc&);

    static const char*	attribName()		{ return "Texture"; }
    static const char*  steeringStr()   	{ return "steering"; }
    static const char*  stepoutStr()      	{ return "stepout"; }
    static const char*  gateStr()		{ return "gate"; }
    static const char*	glcmsizeStr()		{ return "glcmsize"; }
    static const char*	globalminStr()		{ return "globalmin"; }
    static const char*	globalmaxStr()		{ return "globalmax"; }

    void		initSteering()		{ stdPrepSteering(stepout_); }
    void		prepareForComputeData();

protected:
			~Texture() {}
    static Provider*	createInstance(Desc&);
    static void		updateDefaults(Desc&);

    bool		allowParallelComputation() const;  

    bool		getInputOutput(int input,TypeSet<int>& res) const;
    bool		getInputData(const BinID&,int zintv);
    bool		computeData(const DataHolder&,const BinID& relpos,
				    int z0,int nrsamples,int threadid) const;
    const BinID*        desStepout(int,int) const;
    const Interval<int>* desZSampMargin(int,int) const;
    int			scaleVal(float) const;
    void		setFactorShift(float,float);

    int			glcmsize_;
    float		scalingfactor_;
    float		scalingshift_;
    float		globalmin_;
    float		globalmax_;

    Interval<int>	sampgate_;
    Interval<float>	gate_;
    Interval<int>	dessampgate_;
    BinID		stepout_;

    struct PosAndSteeridx
    {
	TypeSet<int>	steeridx_;
	TypeSet<BinID>	pos_;
	TypeSet<int>	posidx_;
    };

    PosAndSteeridx	posandsteeridx_;

    ObjectSet<const DataHolder> inpdata_;
    int			dataidx_;
    const DataHolder*   steeringdata_;
    int			computeGLCM(int idx,int z0,Array2D<int>&) const;
    void		fillGLCM(int sampleidx,int z0,int posidx1,int posidx2,
	    			 int& glcmcount,Array2D<int>& glcm) const;
};

} // namespace Attrib

#endif
