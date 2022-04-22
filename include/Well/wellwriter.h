#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Bert Bril
 Date:		Aug 2003
________________________________________________________________________


-*/

#include "wellmod.h"
#include "bufstring.h"
#include "bufstringset.h"
#include "executor.h"
#include "wellman.h"
class IOObj;

namespace Well
{
class Data;
class Log;
class WriteAccess;

/*!\brief Writes Well::Data to any data storage.

  It is essential that creating a writer does not imply writing any actual data.

 */

mExpClass(Well) Writer
{ mODTextTranslationClass(Well::Writer);
public:

			Writer(const MultiID&,const Data&);
			Writer(const IOObj&,const Data&);
			~Writer();
    bool		isUsable() const	{ return wa_; }

    bool		put() const;		//!< Just write all

    bool		putInfoAndTrack() const;//!< Write Info and track
    bool		putLogs() const;	//!< Write logs only
    bool		putMarkers() const;	//!< Write Markers only
    bool		putD2T() const;		//!< Write D2T model only
    bool		putCSMdl() const;	//!< Write Check shot model only
    bool		putDispProps() const;	//!< Write display pars only
    bool		putLog(const Log&) const;
    bool		putDefLogs() const;
    bool		swapLogs(const Log&,const Log&) const;
    bool		canSwapLogs()		{ return true; }
    bool		renameLog(const char* oldnm,const char* newnm);

    const uiString&	errMsg() const		{ return errmsg_; }

    bool		isFunctional() const;

    static bool		isFunctional(const MultiID&);
    static bool		isFunctional(const IOObj&);

protected:

    WriteAccess*	wa_;
    mutable uiString	errmsg_;

    bool		putTrack() const;

private:

    void		init(const IOObj&,const Data&);

};


}; // namespace Well


mExpClass(Well) MultiWellWriter : public Executor
{ mODTextTranslationClass(MultiWellWriter)
public:
    typedef Well::LoadReqs	StoreReqs;

	    MultiWellWriter(const ObjectSet<Well::Data>&,
			    const ObjectSet<StoreReqs>& reqs);

    int			nextStep() override;
    od_int64		totalNr() const override;
    od_int64		nrDone() const override;
    uiString		uiMessage() const override;
    uiString		uiNrDoneText() const override;
    bool		allWellsWritten() const { return allwellswritten_; }
				//Can then be used to launch a warning locally

protected:
    const ObjectSet<Well::Data>&	wds_;
    const ObjectSet<StoreReqs>&		reqs_;
    od_int64				nrwells_;
    od_int64				nrdone_;
    uiString				msg_;
    bool				allwellswritten_ = true;

    bool		store(const MultiID&, const Well::Data&,
					const StoreReqs);
};

