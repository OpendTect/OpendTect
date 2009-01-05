#ifndef attribposvecoutput_h
#define attribposvecoutput_h

/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Bert Bril
 Date:          June 2005
 RCS:           $Id: attribposvecoutput.h,v 1.5 2009-01-05 09:49:43 cvsranojay Exp $
________________________________________________________________________

-*/

#include "executor.h"
#include "bufstringset.h"

class NLAModel;
class BinIDValueSet;
class PosVecDataSet;

namespace Attrib
{

class EngineMan;
class DescSet;

mClass PosVecOutputGen : public Executor
{
public:

		PosVecOutputGen(const DescSet&,
				const BufferStringSet& attribdefkeys,
				const ObjectSet<BinIDValueSet>& positions,
				ObjectSet<PosVecDataSet>& output,
				const NLAModel* mdl=0);
		~PosVecOutputGen();

    const char*	message() const;
    const char*	nrDoneText() const
		{ return outex_ ? outex_->nrDoneText() : "Positions handled"; }
    od_int64	nrDone() const
		{ return outex_ ? outex_->nrDone() : 0; }
    od_int64	totalNr() const
		{ return outex_ ? outex_->totalNr() : -1; }

protected:

    const BufferStringSet&	inps_;
    const DescSet&		ads_;
    const ObjectSet<BinIDValueSet>& bvss_;
    ObjectSet<PosVecDataSet>&	vdss_;
    EngineMan*			aem_;
    Executor*			outex_;
    mutable BufferString	msg_;

    int				nextStep();
};

}; // namespace Attrib

#endif
