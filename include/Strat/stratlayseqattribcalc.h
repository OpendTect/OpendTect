#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "stratmod.h"
#include "executor.h"

#include "datapointset.h"
#include "ranges.h"
#include "stattype.h"

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
			~LaySeqAttribCalc();

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
    RefMan<DataPointSet>	dps_;
    uiString			msg_;
    ObjectSet<ExtrGateSet>	extrgates_;
    const Strat::Level*		stoplvl_;

    bool			doFinish(bool,od_ostream* =nullptr) override;

};

} // namespace Strat
