#ifndef grubfilterattrib_h
#define grubfilterattrib_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Satyaki Maitra
 Date:          March 2011
 RCS:           $Id: grubbsfilterattrib.h,v 1.1 2011-03-17 05:22:42 cvssatyaki Exp $
________________________________________________________________________

-*/

#include "attribprovider.h"
#include "statruncalc.h"

/*!\brief GrubbFilter Attribute

GrubbFilter grubbval gate= pos0= pos1= stepout=1,1 

*/

namespace Attrib
{

mClass GrubbFilter : public Provider
{
public:
    static void			initClass();
				GrubbFilter(Desc&);

    static const char*		attribName()	{ return "grubbfilter"; }
    static const char*		grubvalStr()	{ return "grubbvalue"; }
    static const char*		gateStr()	{ return "gate"; }
    static const char*		stepoutStr()	{ return "stepout"; }
    static const char*		replaceValStr()	{ return "replacewith"; }

    void			prepPriorToBoundsCalc();
    enum ReplaceType		{ Average, Median, Threshold, GrubbValue,
				  Interpolate };

protected:
    				~GrubbFilter() {}
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

    float			cogrubbval_;
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
