#ifndef stratlaymodgen_h
#define stratlaymodgen_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Bert
 Date:		Oct 2010
 RCS:		$Id$
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
{
public:

			LayerModelGenerator(const LayerSequenceGenDesc&,
					    LayerModel&,int nrseqs=100);

     void		setNrSeq( int nr )	{ nrseqs_ = nr; }
     void		reset();

     virtual od_int64	nrDone() const		{ return seqnr_; }
     virtual od_int64	totalNr() const		{ return nrseqs_; }
     virtual const char* nrDoneText() const	{ return "Sequences generated";}
     virtual const char* message() const	{ return msg_; }
     virtual int	nextStep();

protected:

    const LayerSequenceGenDesc&	desc_;
    LayerModel&			lm_;

    BufferString		msg_;
    od_int64			nrseqs_;
    od_int64			seqnr_;

};


}; // namespace Strat

#endif

