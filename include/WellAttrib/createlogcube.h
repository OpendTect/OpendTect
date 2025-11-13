#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
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
    uiRetVal		setOutputNm(const char* postfix,bool withwllnm,
				    uiStringSet& existimpls);

    bool		isOK() const;

    uiString		uiNrDoneText() const override;
    uiString		uiMessage() const override	{ return msg_; }
    uiRetVal		details() const			{ return uirv_; }
    uiRetVal		allMessages() const;

    void		getOutputNames(BufferStringSet&) const;

protected:
    const Well::LogSet*		logset_ = nullptr;

    mStruct(WellAttrib) LogCube
    {
				LogCube(const char* lognm);
				~LogCube();

	const uiString&		errMsg() const { return errmsg_; }
	bool			doWrite(const SeisTrcBuf&) const;

	bool			makeWriteReady();
	bool			mkIOObj();

	const BufferString	lognm_;
	BufferString		fnm_;
	IOObj*			seisioobj_ = nullptr;
	mutable uiString	errmsg_;
    };

    mStruct(WellAttrib) WellData : public CallBacker
    {
				WellData(const MultiID&,
					 const BufferStringSet* lognms);
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

    uiString			msg_;
    uiRetVal			uirv_;

    od_int64			nrIterations() const override
				{ return welldata_.size(); }

    void			init(const BufferStringSet& lognms,
				     const TypeSet<MultiID>& wllids);
    bool			doPrepare(int) override;
    bool			doWork(od_int64,od_int64,int) override;
    bool			doFinish(bool) override;

    uiString			makeLogTraces(int iwell);
    void			getLogNames(BufferStringSet&) const;

    static void			addUniqueTrace(const SeisTrc&,SeisTrcBuf&);
};
