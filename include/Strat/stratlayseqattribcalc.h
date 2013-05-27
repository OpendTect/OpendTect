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

#include "stratmod.h"
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

mExpClass(Strat) LaySeqAttribCalc
{
public:

    			LaySeqAttribCalc(const LaySeqAttrib&,const LayerModel&);

    float		getValue(const LayerSequence&,
	    			 const Interval<float>& zrange) const;
    
    void		setExtrGates(const TypeSet<Interval<float> >& extrgates)
						 { extrgates_ = extrgates; }

protected:

    const LaySeqAttrib&			attr_;

    Stats::Type				stattype_;
    Stats::UpscaleType			statupscl_;
    int					validx_;
    ObjectSet<const Strat::UnitRef>	units_;
    ObjectSet<const Strat::Lithology>	liths_;
    TypeSet<Interval<float> >&		extrgates_;

    float		getLocalValue(const LayerSequence&,
	    			      const Interval<float>&) const;
    float		getGlobalValue(const LayerSequence&) const;
    void		applyTransform(TypeSet<float>&) const;

};


mExpClass(Strat) LayModAttribCalc : public Executor
{
public:
    			LayModAttribCalc(const LayerModel&,
				     const LaySeqAttribSet&,
				     DataPointSet&);
			~LayModAttribCalc();

    			// point-specific extraction gates have precedence over
    			// global calczwdth_
    void		setCalcZWidth( float w ) { calczwdth_ = w; }
    void		setExtrGates(
			const TypeSet<TypeSet<Interval<float> > >& extrgates )
						   { extrgates_ = extrgates; }

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
    DataPointSet&		dps_;
    BufferString		msg_;
    float			calczwdth_;
    TypeSet<TypeSet<Interval<float> > >&	extrgates_;

};

}; // namespace Strat

#endif

