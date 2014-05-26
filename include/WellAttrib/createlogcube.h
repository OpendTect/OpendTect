#ifndef createlogcube_h
#define createlogcube_h

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
#include "task.h"
#include "horsampling.h"
#include "seisbuf.h"
#include "seistrc.h"
#include "wellextractdata.h"

class BinID;
namespace Well { class Data; }

mExpClass(WellAttrib) LogCubeCreator : public ParallelTask
{
public:
				LogCubeCreator(const BufferStringSet& lognms,
					       const MultiID& wllid,
					       const Well::ExtractParams& pars,
					       int nrtrcs=1);
				LogCubeCreator(const BufferStringSet& lognms,
					       const TypeSet<MultiID>& wllids,
					       const Well::ExtractParams& pars,
					       int nrtrcs=1);
				~LogCubeCreator();

				//Returns false is an output already exists
    bool			setOutputNm(const char* postfix=0,
					    bool withwllnm=false);

    void			resetMsg() { errmsg_.setEmpty(); }
    void			doMerge(bool yn) { domerge_ = yn; }
    const char*			errMsg() const;

    uiStringCopy		uiNrDoneText() const { return "Wells handled"; }
    od_int64			totalNr() const { return nrIterations(); }

protected:

    mStruct(WellAttrib) LogCubeData
    {
				LogCubeData(const BufferString& lognm, int iwll)
				    : lognm_(lognm)
				    , outfnm_(lognm)
				    , iwll_(iwll)
				    , seisioobj_(0)
				{}
				~LogCubeData() {};

	bool			isOK(); //Do not use, will be removed
	const char*		errMsg() const;
	bool			doWrite(const SeisTrcBuf&) const;

	bool			makeWriteReady();
	bool			mkIOObj();

	BufferString		lognm_;
	BufferString		outfnm_;
	int			iwll_;	//Do not use, will be removed
	IOObj*			seisioobj_;
	mutable BufferString	errmsg_;
    };

    mStruct(WellAttrib) WellData
    {
				WellData(const MultiID& wid);
				~WellData();

	bool			isOK();
	const char*		errMsg() const;

	const Well::Data*	wd_;
	TypeSet<BinID>		binids_;
	HorSampling		hrg_; //Do not use, will be removed
	mutable BufferString	errmsg_;
	ObjectSet<SeisTrcBuf>	trcs_;
    };

    ObjectSet<LogCubeData>	logdatas_;
    ObjectSet<WellData>		welldata_;
    Well::ExtractParams		extractparams_;
    int				nrduplicatetrcs_;

    bool			domerge_;  //Do not use, will be removed
    ObjectSet<IOObj>		seisioobjs_;  //Do not use, will be removed
    BufferStringSet		cbvsnms_;  //Do not use, will be removed

    mutable BufferString	errmsg_;

    od_int64			nrIterations() const { return welldata_.size();}
    od_int64			nrdone_;

    bool			init(const BufferStringSet& lognms,
				     const TypeSet<MultiID>& wllids);
    bool			doPrepare(int);
    bool			doWork(od_int64,od_int64,int);
    bool			doFinish(bool);

    bool			makeLogTraces(int iwell);
    void			getLogNames(BufferStringSet&) const;
    void			addUniqueTrace(const SeisTrc&,SeisTrcBuf&)const;

				// Do not use, will be removed
    bool                        writeLog2Cube(const LogCubeData&) const;
};

#endif


