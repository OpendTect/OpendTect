#ifndef stratlayseqattribcalc_h
#define stratlayseqattribcalc_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Bert
 Date:		Jan 2011
 RCS:		$Id$
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
					 const LaySeqAttribSet&,
					 DataPointSet&);
			~LayModAttribCalc();

    void		setCalcZWidth( float w ) { calczwdth_ = w; }

    const char*		message() const		{ return msg_.buf(); }
    const char*		nrDoneText() const	{ return "Models handled";}
    od_int64		nrDone() const		{ return seqidx_; }
    od_int64		totalNr() const		{ return calcs_.size(); }
    int			nextStep();

    static const char*	sKeyModelIdx()		{ return "Model Index"; }

protected:

    const LayerModel&		lm_;
    ObjectSet<LaySeqAttribCalc>	calcs_;
    od_int64			seqidx_;
    TypeSet<int>		dpscidxs_;
    int				dpsdepthcidx_;
    int				dpsmodnrcidx_;
    DataPointSet&		dps_;
    BufferString		msg_;
    float			calczwdth_;

};

}; // namespace Strat

#endif
