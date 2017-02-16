#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Bert
 Date:		Mar 2008
________________________________________________________________________

-*/

#include "seiscommon.h"
#include "executor.h"
#include "binid.h"
#include "samplingdata.h"

class Scaler;
class SeisTrc;
class SeisTrcBuf;
class SeisTrcWriter;
namespace Seis { class Provider; class ProviderSet; }


/*!\brief Merges 2D and 3D post-stack data */

mExpClass(Seis) SeisMerger : public Executor
{ mODTextTranslationClass(SeisMerger);
public:

			SeisMerger(const ObjectSet<IOPar>& in,
				   const IOPar& out,bool is2d);
    virtual		~SeisMerger();

    uiString		message() const;
    od_int64		nrDone() const		{ return nrpos_; }
    od_int64		totalNr() const		{ return totnrpos_; }
    uiString		nrDoneText() const	{
					    return tr("Positions handled");
						}
    int			nextStep();
    void		setScaler(Scaler*);

    bool		stacktrcs_; //!< If not, first trace will be used

protected:

    bool			is2d_;
    ObjectSet<Seis::Provider>	provs_;
    Seis::ProviderSet*		provset_;
    SeisTrcWriter*		wrr_;
    int				curprovidx_;
    int				nrpos_;
    od_int64			totnrpos_;
    uiString			errmsg_;

    BinID			curbid_;
    SeisTrcBuf&			trcbuf_;
    int				nrsamps_;
    SamplingData<float>		sd_;
    Scaler*			scaler_;

    SeisTrc*			getNewTrc();
    SeisTrc*			getTrcFrom(Seis::Provider&);
    uiRetVal			get3DTraces(SeisTrc*);
    SeisTrc*			getStacked(SeisTrcBuf&);
    int				writeTrc(SeisTrc*);
    int				writeFromBuf();

};
