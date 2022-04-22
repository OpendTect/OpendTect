#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Bert
 Date:		Jan 2011
________________________________________________________________________

-*/

#include "stratmod.h"
#include "stattype.h"
#include "ranges.h"
#include "executor.h"
class DataPointSet;

namespace Strat
{
class Level;
class UnitRef;
class Lithology;
class LayerModel;
class LaySeqAttrib;
class LayerSequence;

/*!\brief calculates attributes from layer sequences

  Note that if the attribute is global, then the zrange is not used.
 */

mExpClass(Strat) LaySeqAttribCalc
{ mODTextTranslationClass(LaySeqAttribCalc);
public:

			LaySeqAttribCalc(const LaySeqAttrib&,const LayerModel&);

    float		getValue(const LayerSequence&,
				 const Interval<float>& zrange) const;

    bool		isDist() const;
    bool		isVel() const;

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

    friend class	LayModAttribCalc;
};


mExpClass(Strat) LayModAttribCalc : public Executor
{ mODTextTranslationClass(LayModAttribCalc);
public:

    typedef TypeSet<Interval<float> > ExtrGateSet;

			LayModAttribCalc(const LayerModel&,
				     const LaySeqAttribSet&,
				     DataPointSet&);
			~LayModAttribCalc();

			// will use nearest layer if no extraction gates
    void		setExtrGates(const ObjectSet<ExtrGateSet>&,
				const Strat::Level* stoplvl=0);

    uiString		uiMessage() const override	{ return msg_; }
    uiString		uiNrDoneText() const override
			    { return tr("Models handled");}

    od_int64		nrDone() const override		{ return seqidx_; }
    od_int64		totalNr() const override;
    int			nextStep() override;

    static const char*	sKeyModelIdx()		{ return "Model Index"; }

protected:

    const LayerModel&		lm_;
    ObjectSet<LaySeqAttribCalc> calcs_;
    od_int64			seqidx_;
    TypeSet<int>		dpscidxs_;
    DataPointSet&		dps_;
    uiString			msg_;
    ObjectSet<ExtrGateSet>	extrgates_;
    const Strat::Level*		stoplvl_;

    int				doFinish();

};

}; // namespace Strat

