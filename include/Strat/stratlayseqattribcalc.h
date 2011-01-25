#ifndef stratlayseqattribcalc_h
#define stratlayseqattribcalc_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Bert
 Date:		Jan 2011
 RCS:		$Id: stratlayseqattribcalc.h,v 1.2 2011-01-25 09:41:24 cvsbert Exp $
________________________________________________________________________

-*/

#include "stattype.h"

namespace Strat
{
class UnitRef;
class Lithology;
class LayerModel;
class LaySeqAttrib;
class LayerSequence;


/*!\brief calculates attributes from layer sequences */

mClass LaySeqAttribCalc
{
public:

    			LaySeqAttribCalc(const LaySeqAttrib&,const LayerModel&);

    float		getValue(const LayerSequence&,float zpos) const;

protected:

    const LaySeqAttrib&			attr_;

    const Stats::Type			stattype_;
    const Stats::UpscaleType		statupscl_;
    ObjectSet<const Strat::UnitRef>	units_;
    ObjectSet<const Strat::Lithology>	liths_;

    float		getLocalValue(const LayerSequence&,float) const;
    float		getGlobalValue(const LayerSequence&) const;

};

}; // namespace Strat

#endif
