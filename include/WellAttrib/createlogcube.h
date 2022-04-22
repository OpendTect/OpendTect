#pragma once

/*+
________________________________________________________________________

(C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
Author:        Bruno
Date:          July 2011
RCS:           $Id: createlogcube.h,v 1.1 2009-01-19 13:02:33 cvsbruno Exp
$
________________________________________________________________________
-*/

#include "wellattribmod.h"
#include "seisbuf.h"
#include "seistrc.h"
#include "wellextractdata.h"

class BinID;
namespace Well { class Data; }

mExpClass(WellAttrib) LogCubeCreator : public ParallelTask
{ mODTextTranslationClass(LogCubeCreator);
public:
			LogCubeCreator(const BufferStringSet& lognms,
				       const MultiID& wllid,
				       const Well::ExtractParams& pars,
				       int nrtrcs=1);
			LogCubeCreator(const BufferStringSet& lognms,
				       const TypeSet<MultiID>& wllids,
				       const Well::ExtractParams& pars,
				       int nrtrcs=1);
			LogCubeCreator(const BufferStringSet& lognms,
				       const Well::LogSet& logset,
				       const MultiID& wllid,
				       const Well::ExtractParams& pars,
				       int nrtrcs=1);
			~LogCubeCreator();

			//Returns false if an output already exists
    bool		setOutputNm(const char* postfix=0,
				    bool withwllnm=false);
    void		getOutputNames(BufferStringSet&) const;

    const uiString&	errMsg() const { return errmsg_; }
    bool		isOK() const { return errmsg_.isEmpty(); }
    void		resetMsg() { errmsg_.setEmpty(); }

    uiString		uiNrDoneText() const override
			{ return tr("Wells handled"); }
    od_int64		totalNr() const override
			{ return nrIterations(); }
    bool		stopAllOnFailure() const override
			{ return false; }

protected:
    const Well::LogSet*		logset_ = nullptr;

    mStruct(WellAttrib) LogCube
    {
				LogCube(const BufferString& lognm)
				    : lognm_(lognm)
				    , fnm_(lognm)
				    , seisioobj_(0)
				{}

	const uiString&		errMsg() const { return errmsg_; }
	bool			doWrite(const SeisTrcBuf&) const;

	bool			makeWriteReady();
	bool			mkIOObj();

	const BufferString&	lognm_;
	BufferString		fnm_;
	IOObj*			seisioobj_;
	mutable uiString	errmsg_;
    };

    mStruct(WellAttrib) WellData : public CallBacker
    {
				WellData(const MultiID& wid);
				~WellData();

	bool			isOK() const { return errmsg_.isEmpty(); }
	const uiString&		errMsg() const { return errmsg_; }

	RefMan<Well::Data>	wd_;
	TypeSet<BinID>		binidsalongtrack_;
	ObjectSet<SeisTrcBuf>	trcs_;
	uiString		errmsg_;
    };

    ObjectSet<LogCube>		logcubes_;
    ObjectSet<WellData>		welldata_;
    Well::ExtractParams		extractparams_;
    int				stepout_;

    uiString			errmsg_;

    od_int64			nrIterations() const override
				{ return welldata_.size(); }
    od_int64			nrdone_;

    bool			init(const BufferStringSet& lognms,
				     const TypeSet<MultiID>& wllids);
    bool			doPrepare(int) override;
    bool			doWork(od_int64,od_int64,int) override;
    bool			doFinish(bool) override;

    bool			makeLogTraces(int iwell);
    void			getLogNames(BufferStringSet&) const;
    void			addUniqueTrace(const SeisTrc&,SeisTrcBuf&)const;
};

