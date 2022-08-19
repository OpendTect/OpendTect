#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "wellmod.h"
#include "executor.h"
#include "ranges.h"
#include "bufstring.h"
#include "manobjectset.h"
#include "uistring.h"
#include "wellman.h"
class IOObj;
class BufferStringSet;
class DBKeySet;
class SurveyChanger;


namespace Well
{
class Data;
class ReadAccess;
class LoadReqs;


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

    bool		getInfo() const;	//!< Read Info only
    bool		getTrack() const;	//!< Read Track only
    bool		getLogs(bool needjustinfo=false) const;
						//!< Read logs only
    bool		getMarkers() const;	//!< Read Markers only
    bool		getD2T() const;		//!< Read D2T model parts
    bool		getCSMdl() const;	//!< Read Checkshot model parts
    bool		getDispProps() const;	//!< Read display props only
    bool		getLog(const char* lognm) const; //!< Read this one only
    void		getLogInfo(BufferStringSet& lognms) const;
    bool		getDefLogs() const;	//!< Read list of default logs
						//!< for a particular mnemonic

    const uiString&	errMsg() const		{ return errmsg_; }

    Well::Data*		data();
    const Well::Data*	data() const
			{ return const_cast<Reader*>(this)->data(); }

    bool		getMapLocation(Coord&) const;

protected:

    ReadAccess*		ra_ = nullptr;
    mutable uiString	errmsg_;

private:

    void		init(const IOObj&,Data&);

};

} // namespace Well


mExpClass(Well) MultiWellReader : public Executor
{ mODTextTranslationClass(MultiWellReader)
public:

				MultiWellReader(const TypeSet<MultiID>&,
						RefObjectSet<Well::Data>&,
				    const Well::LoadReqs reqs=Well::LoadReqs());
				MultiWellReader(const DBKeySet&,
						RefObjectSet<Well::Data>&,
				    const Well::LoadReqs reqs=Well::LoadReqs());
				~MultiWellReader();

    int				nextStep() override;
    od_int64			totalNr() const override;
    od_int64			nrDone() const override;
    uiString			uiMessage() const override;
    uiString			uiNrDoneText() const override;
    bool			allWellsRead() const { return allwellsread_; }
				//Can then be used to launch a warning locally
    const uiString&		errMsg() const		{ return errmsg_; }

protected:

    RefObjectSet<Well::Data>&	wds_;
    DBKeySet&			keys_;
    od_int64			nrwells_;
    od_int64			nrdone_ = 0;
    uiString			errmsg_;
    bool			allwellsread_ = true;
    const Well::LoadReqs	reqs_;
    SurveyChanger*		chgr_ = nullptr;
};
