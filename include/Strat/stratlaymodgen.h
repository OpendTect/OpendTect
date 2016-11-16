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
     void		reset();

     virtual od_int64	nrDone() const		{ return seqnr_; }
     virtual od_int64	totalNr() const		{ return nrseqs_; }
     virtual uiString	nrDoneText() const	{ 
					    return tr("Sequences generated");
						}
     virtual uiString	message() const	{ return msg_; }
     virtual int	nextStep();

protected:

    const LayerSequenceGenDesc&	desc_;
    LayerModel&			lm_;

    uiString			msg_;
    od_int64			nrseqs_;
    od_int64			seqnr_;

};


}; // namespace Strat
