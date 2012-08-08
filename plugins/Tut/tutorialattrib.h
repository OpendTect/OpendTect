#ifndef tutorialattrib_h
#define tutorialattrib_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        R. K. Singh
 Date:          May 2007
 RCS:           $Id: tutorialattrib.h,v 1.9 2012-08-08 05:15:32 cvsranojay Exp $
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

mClass(Tut) Tutorial : public Provider
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
    void		initSteering()		{ stdPrepSteering(stepout_); }

protected:

			~Tutorial() {}
    static Provider*	createInstance(Desc&);
    static void		updateDesc(Desc&);

    bool		allowParallelComputation() const	{ return true; }

    bool		getInputOutput(int input,TypeSet<int>& res) const;
    bool		getInputData(const BinID&,int zintv);
    bool		computeData(const DataHolder&,const BinID& relpos,
				    int z0,int nrsamples,int threadid) const;
    const BinID*        desStepout(int,int) const;
    const Interval<int>* desZSampMargin(int,int) const;

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

}; // namespace Attrib

#endif


