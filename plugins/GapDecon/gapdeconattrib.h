#ifndef gapdeconattrib_h
#define gapdeconattrib_h

/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Helene Huck
 Date:          14-07-2006
 RCS:           $Id: gapdeconattrib.h,v 1.4 2006-08-22 14:06:17 cvshelene Exp $
________________________________________________________________________

-*/

#include "attribprovider.h"

/*!\brief Gap Decon Attribute
*/

namespace Attrib
{

class GapDecon : public Provider
{
public:

    static void			initClass();
				GapDecon(Desc&);

    static const char*		attribName()	{ return "GapDecon"; }
    static const char*		gateStr()	{ return "gate"; }
    static const char*		lagsizeStr()	{ return "lagsize"; }
    static const char*		gapsizeStr()	{ return "gapsize"; }
    static const char*		nrtrcsStr()	{ return "nrtrcsmixed"; }
    static const char*		noiselevelStr()	{ return "noiselevel"; }
    static const char*		isinp0phaseStr(){ return "isinpzerophase"; }
    static const char*		isout0phaseStr(){ return "isoutzerophase"; }

protected:

    static Provider*		createInstance(Desc&);
    static void			updateDesc(Desc&);

    bool			allowParallelComputation() const
				{ return true; }

    bool			getInputOutput(int inp,TypeSet<int>& res) const;
    bool			getInputData(const BinID&,int zintv);
    bool			computeData(const DataHolder&,
	    				    const BinID& relpos,
					    int z0,int nrsamples) const;

    const BinID*		reqStepout(int input,int output) const;

    Interval<float>		gate_;
    int				lagsize_;
    int				gapsize_;
    int				noiselevel_;

    bool			isinpzerophase_;
    bool			isoutzerophase_;
    BinID			stepout_;

    bool			inited_;
    int				nlag_;
    int				ncorr_;
    int				ngap_;
    
    const DataHolder*		inputdata_;
    int				dataidx_;
};

} // namespace Attrib


#endif
