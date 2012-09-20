#ifndef grubbsfilterattrib_h
#define grubbsfilterattrib_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Satyaki Maitra
 Date:          March 2011
 RCS:           $Id$
________________________________________________________________________

-*/

#include "expattribsmod.h"
#include "attribprovider.h"
#include "statruncalc.h"

/*!\brief GrubbsFilter Attribute

GrubbsFilter grubbsval gate= pos0= pos1= stepout=1,1 

*/

namespace Attrib
{

mClass(ExpAttribs) GrubbsFilter : public Provider
{
public:
    static void			initClass();
				GrubbsFilter(Desc&);

    static const char*		attribName()	{ return "GrubbsFilter"; }

    static const char*		grubbsvalStr()	{ return "grubbsvalue"; }
    static const char*		gateStr()	{ return "gate"; }
    static const char*		stepoutStr()	{ return "stepout"; }
    static const char*		replaceValStr()	{ return "replacewith"; }

    void			prepPriorToBoundsCalc();
    enum ReplaceType		{ Average, Median, Threshold, GrubbsValue,
				  Interpolate };

protected:
    				~GrubbsFilter() {}
    static Provider*		createInstance(Desc&);
    static void			updateDesc(Desc&);

    bool			allowParallelComputation() const
				{ return false; }

    float			replaceVal(const Stats::RunCalc<float>&) const;
    bool			getInputData(const BinID&,int zintv);
    bool			computeData(const DataHolder&,
	    				    const BinID& relpos,
					    int z0,int nrsamples,
					    int threadid) const;

    const BinID*		desStepout(int input,int output) const;
    const Interval<float>*	desZMargin(int input,int output) const;
    bool			getTrcPos();

    float			cogrubbsval_;
    BinID			stepout_;
    Interval<float>		gate_;
    TypeSet<BinID>		trcpos_;
    int				centertrcidx_;
    int				type_;

    int				dataidx_;

    ObjectSet<const DataHolder>	inputdata_;
};

} // namespace Attrib


#endif

