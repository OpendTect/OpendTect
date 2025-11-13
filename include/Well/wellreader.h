#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "wellmod.h"

#include "paralleltask.h"
#include "welldata.h"

class BufferStringSet;
class DBKeySet;
class IOObj;
class SurveyChanger;

namespace Well
{
class LogID;
class ReadAccess;


/*!\brief Reads Well::Data from any data store */

mExpClass(Well) Reader
{ mODTextTranslationClass(Reader)
public:

			Reader(const MultiID&,Data&);
			Reader(const IOObj&,Data&);
			~Reader();

    bool		isUsable() const	{ return ra_; }

    bool		get() const;		//!< Just read all
			// Should use Well::MGR().get instead to get all
    uiRetVal		readReqData(const LoadReqs&) const;

    bool		getInfo() const;	//!< Read Info only
    bool		getTrack() const;	//!< Read Track only
    bool		getMarkers() const;	//!< Read Markers only

    bool		getD2T() const;		//!< Read D2T model parts
    bool		getCSMdl() const;	//!< Read Checkshot model parts

    bool		getLogs(bool needjustinfo) const;
						//!< Read logs only
    bool		getLogs(const BufferStringSet& lognms) const;
    bool		getLog(const char* lognm) const; //!< Read this one only
    bool		getLogByID(const LogID&) const; //!< Read this one only
    void		getLogNames(BufferStringSet& lognms) const;
    bool		getDefLogs() const;	//!< Read list of default logs

    bool		getDispProps() const;	//!< Read display props only
						//!< for a particular mnemonic

    const uiString&	errMsg() const		{ return errmsg_; }

    Well::Data*		data();
    const Well::Data*	data() const;

    bool		getMapLocation(Coord&) const;

    static bool		canReadInParallel(const MultiID&);

protected:

    ReadAccess*		ra_ = nullptr;
    mutable uiString	errmsg_;

    uiString		sCannotReadFileHeader() const;

private:

    void		init(const IOObj&,Data&);

public:
			mDeprecated("Use getLogNames")
    void		getLogInfo(BufferStringSet& lognms) const;

};

} // namespace Well


mExpClass(Well) MultiWellReader : public ParallelTask
{ mODTextTranslationClass(MultiWellReader)
public:

				MultiWellReader(const TypeSet<MultiID>&,
						RefObjectSet<Well::Data>&,
				    const Well::LoadReqs =Well::LoadReqs());
				MultiWellReader(const DBKeySet&,
						RefObjectSet<Well::Data>&,
				    const Well::LoadReqs =Well::LoadReqs());
				~MultiWellReader();

    MultiWellReader&		setReqs(const Well::LoadReqs&);
    MultiWellReader&		forceRead(bool yn);
				//!< Default: false
    MultiWellReader&		allowMissingLogs(bool yn);
				//!< Default: true if logs are required

    uiString			uiNrDoneText() const override;
    uiString			uiMessage() const override	{ return msg_; }
    uiRetVal			details() const			{ return uirv_;}
    uiRetVal			allMessages() const;

    bool			hasFails() const;
				//Can then be used to launch a warning locally

protected:
    void			init();
    void			setMaxNrThreads();
    od_int64			nrIterations() const override;
    int				maxNrThreads() const override;
    bool			stopAllOnFailure() const override
				{ return false; }

    bool			doPrepare(int) override;
    bool			doWork(od_int64,od_int64,int) override;
    bool			doFinish(bool success) override;

    RefMan<Well::Data>		getWD(const DBKey&,bool islocal) const;

    DBKeySet&			keys_;
    BoolTypeSet			islocal_;
    RefObjectSet<Well::Data>&	wds_;
    od_int64			nrwells_;
    Threads::Atomic<od_int64>	nrsuccess_;
    od_int64			nrdone_			= 0;
    uiString			msg_;
    uiRetVal			uirv_;
    bool			allsamesurvey_;
    int				maxnrthreads_		= mUdf(int);
    Well::LoadReqs		reqs_;
    SurveyChanger*		chgr_			= nullptr;
    bool			forceread_		= false;
};
