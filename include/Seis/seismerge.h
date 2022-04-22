#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Bert
 Date:		Mar 2008
________________________________________________________________________

-*/

#include "seismod.h"
#include "executor.h"
#include "binid.h"
#include "samplingdata.h"

class Scaler;
class SeisTrc;
class SeisTrcBuf;
class SeisTrcReader;
class SeisTrcWriter;


/*!\brief Merges 2D and 3D post-stack data */

mExpClass(Seis) SeisMerger : public Executor
{ mODTextTranslationClass(SeisMerger);
public:

			SeisMerger(const ObjectSet<IOPar>& in,const IOPar& out,
				   bool is2d);
			SeisMerger(const IOPar&);	//For post-processing
    virtual		~SeisMerger();

    uiString		uiMessage() const override;
    od_int64		nrDone() const override		{ return nrpos_; }
    od_int64		totalNr() const override	{ return totnrpos_; }
    uiString		uiNrDoneText() const override
			{ return tr("Positions handled"); }
    int			nextStep() override;
    void		setScaler(Scaler*);

    bool		stacktrcs_; //!< If not, first trace will be used

protected:

    bool			is2d_;
    ObjectSet<SeisTrcReader>	rdrs_;
    SeisTrcWriter*		wrr_;
    int				currdridx_;
    int				nrpos_;
    int				totnrpos_;
    uiString			errmsg_;

    BinID			curbid_;
    SeisTrcBuf&			trcbuf_;
    int				nrsamps_;
    SamplingData<float>		sd_;
    Scaler*			scaler_;

    SeisTrc*			getNewTrc();
    SeisTrc*			getTrcFrom(SeisTrcReader&);
    void			get3DTraces();
    SeisTrc*			getStacked(SeisTrcBuf&);
    bool			toNextPos();
    int				writeTrc(SeisTrc*);
    int				writeFromBuf();

};


