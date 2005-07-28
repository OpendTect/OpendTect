#ifndef attribposvecoutput_h
#define attribposvecoutput_h

/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Bert Bril
 Date:          June 2005
 RCS:           $Id: attribposvecoutput.h,v 1.1 2005-07-28 10:53:49 cvshelene Exp $
________________________________________________________________________

-*/

#include "executor.h"
#include "bufstringset.h"
class NLAModel;
class FeatureSet;
class BinIDValueSet;
class PosVecDataSet;
class BufferStringSet;

namespace Attrib
{

class EngineMan;
class DescSet;

class PosVecOutputGen : public Executor
{
public:

		PosVecOutputGen(const DescSet&,
				  const BufferStringSet& attribdefkeys,
				  const ObjectSet<BinIDValueSet>& positions,
				  ObjectSet<PosVecDataSet>& output,
				  const NLAModel* m=0);
		~PosVecOutputGen()		{ cleanUp(); }

    const char*	message() const;
    const char*	nrDoneText() const
		{ return outex_ ? outex_->nrDoneText() : "Positions handled"; }
    int		nrDone() const
		{ return outex_ ? outex_->nrDone() : 0; }
    int		totalNr() const
		{ return outex_ ? outex_->totalNr() : -1; }

protected:

    ObjectSet<FeatureSet>	workfss_;
    const BufferStringSet&	inps_;
    const DescSet&		ads_;
    const NLAModel*		nlamodel_;
    const ObjectSet<BinIDValueSet>& bvss_;
    ObjectSet<PosVecDataSet>&	vdss_;

    bool			failed_;
    BufferStringSet		linenames_;
    int				curlnr_;
    EngineMan*			aem_;
    Executor*			outex_;
    mutable BufferString	msg_;

    void			cleanUp();
    void			nextExec();
    void			addResults();
    int				nextStep();

};

};//namespace

#endif
