#ifndef seisdatapackwriter_h
#define seisdatapackwriter_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Y.C. Liu
 Date:		April 2007
 RCS:		$Id$
________________________________________________________________________

-*/

#include "seismod.h"
#include "trckeyzsampling.h"
#include "executor.h"
#include "multiid.h"

namespace PosInfo { class CubeData; }
class RegularSeisDataPack;
class SeisTrcWriter;
class SeisTrc;


mExpClass(Seis) SeisDataPackWriter : public Executor
{ mODTextTranslationClass(SeisDataPackWriter);
public:
			SeisDataPackWriter(const MultiID&,
			       const RegularSeisDataPack&,
			       const TypeSet<int>& components=TypeSet<int>());
			~SeisDataPackWriter();

    void		setSelection(const TrcKeySampling&,
				     const Interval<int>&);
    const RegularSeisDataPack* dataPack() const	{ return dp_; }
    void		setNextDataPack(const RegularSeisDataPack&);

    od_int64		nrDone() const;
    od_int64		totalNr() const;
    uiString		uiMessage() const;
    uiString		uiNrDoneText() const
			{ return tr("Traces written:"); }
    int			nextStep();

    TrcKeySampling	hSampling() const	{ return tks_; }
    Interval<int>	zSampling() const       { return zrg_; }

private:

    void			setCubeIdxRange();
    bool			setTrc();

    TypeSet<int>		compidxs_;
    MultiID			mid_;
    const RegularSeisDataPack*	dp_;

    int				nrdone_;
    int				totalnr_;
    TrcKeySamplingIterator	iterator_;
    const PosInfo::CubeData*	posinfo_;
    SeisTrcWriter*		writer_;
    SeisTrc*			trc_;

    TrcKeySampling		tks_;
    Interval<int>		zrg_;

    void			getPosInfo();
    void			obtainDP();
    void			releaseDP();

};


#endif
