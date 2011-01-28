#ifndef stratlayseqattribcalc_h
#define stratlayseqattribcalc_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Bert
 Date:		Jan 2011
 RCS:		$Id: stratlayseqattribcalc.h,v 1.3 2011-01-28 11:09:51 cvsbert Exp $
________________________________________________________________________

-*/

#include "stattype.h"
#include "ranges.h"
#include "executor.h"
class DataPointSet;

namespace Strat
{
class UnitRef;
class Lithology;
class LayerModel;
class LaySeqAttrib;
class LayerSequence;

/*!\brief calculates attributes from layer sequences

  Note that if the attribute is global, then the zrange is not used.
 */

mClass LaySeqAttribCalc
{
public:

    			LaySeqAttribCalc(const LaySeqAttrib&,const LayerModel&);

    float		getValue(const LayerSequence&,
	    			 const Interval<float>& zrange) const;

protected:

    const LaySeqAttrib&			attr_;

    Stats::Type				stattype_;
    Stats::UpscaleType			statupscl_;
    int					validx_;
    ObjectSet<const Strat::UnitRef>	units_;
    ObjectSet<const Strat::Lithology>	liths_;

    float		getLocalValue(const LayerSequence&,
	    			      const Interval<float>&) const;
    float		getGlobalValue(const LayerSequence&) const;
    void		applyTransform(TypeSet<float>&) const;

};


mClass LayModAttribCalc : public Executor
{
public:
    			LayModAttribCalc(const LayerModel&,
					 const LaySeqAttribSet&,DataPointSet&);
			~LayModAttribCalc();

    const char*		message() const	{ return "Extracting layer attributes";}
    const char*		nrDoneText() const { return "Models handled";}
    od_int64		nrDone() const	{ return seqidx_; }
    od_int64		totalNr() const	{ return calcs_.size(); }
    int			nextStep();

protected:

    const LayerModel&		lm_;
    ObjectSet<LaySeqAttribCalc>	calcs_;
    od_int64			seqidx_;
    TypeSet<int>		dpsidxs_;
    DataPointSet&		dps_;

};

}; // namespace Strat

#endif
