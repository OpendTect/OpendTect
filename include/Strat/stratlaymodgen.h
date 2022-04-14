#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Bert
 Date:		Oct 2010
________________________________________________________________________


-*/

#include "stratmod.h"
#include "executor.h"

namespace Strat
{
class LayerModel;
class LayerSequence;
class LayerSequenceGenDesc;

/*!\brief Generates LayerSequences.  */

mExpClass(Strat) LayerModelGenerator : public Executor
{ mODTextTranslationClass(LayerModelGenerator);
public:

			LayerModelGenerator(const LayerSequenceGenDesc&,
					    LayerModel&,int nrseqs=100);

    void		setNrSeq( int nr )	{ nrseqs_ = nr; }

    od_int64		nrDone() const override		{ return seqnr_; }
    od_int64		totalNr() const override	{ return nrseqs_; }
    uiString		uiNrDoneText() const override
			    { return tr("Sequences generated"); }

    uiString		uiMessage() const override	{ return msg_; }

private:

    bool		goImpl(od_ostream*,bool,bool,int) override;
    int			nextStep() override;

    const LayerSequenceGenDesc& desc_;
    LayerModel&			lm_;

    uiString			msg_;
    od_int64			nrseqs_;
    od_int64			seqnr_ = 0;

};


}; // namespace Strat

