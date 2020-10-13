#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Bert
 Date:		Aug 2003
________________________________________________________________________


-*/

#include "wellcommon.h"
#include "wellmanager.h"
#include "dbkey.h"
#include "executor.h"
#include "ranges.h"
#include "bufstring.h"
#include "uistring.h"
class IOObj;
class BufferStringSet;

namespace Well
{
class ReadAccess;

/*!\brief Reads Well::Data from any data store */

mExpClass(Well) Reader
{ mODTextTranslationClass(Reader);
public:

			Reader(const DBKey&,Data&);
			Reader(const IOObj&,Data&);
			~Reader();
    bool		isUsable() const	{ return ra_; }

    bool		getInfo() const;	//!< Read Info only
    bool		getTrack() const;	//!< Read Track only
    bool		getLogs(bool needjustinfo=false) const;
						//!< Read logs only
    bool		getMarkers() const;	//!< Read Markers only
    bool		getD2T() const;		//!< Read D2T model parts
    bool		getCSMdl() const;	//!< Read Checkshot model parts
    bool		getDispProps() const;	//!< Read display props only
    bool		getLog(const char* lognm) const; //!< Read this one only
    void		getLogNames(BufferStringSet&) const;
    void		getLogInfo(ObjectSet<IOPar>&) const;

    const uiString&	errMsg() const		{ return errmsg_; }
    Well::Data*		data();
    const Well::Data*	data() const
			{ return const_cast<Reader*>(this)->data(); }

    bool		getMapLocation(Coord&) const;

protected:

    ReadAccess*		ra_;
    mutable uiString	errmsg_;

private:

    void		init(const IOObj&,Data&);

public:

    mDeprecated bool	get() const { return getAll(); /* use Well::MGR() */ }
    bool		getAll() const;
			    //!< probably you want to use Well::MGR().get()

};


}; // namespace Well


mExpClass(Well) MultiWellReader : public Executor
{ mODTextTranslationClass(MultiWellReader)
public:


			MultiWellReader(const DBKeySet&, Well::LoadReqs);

    int			nextStep();
    od_int64		totalNr() const;
    od_int64		nrDone() const;
    uiString		message() const;
    uiString		nrDoneText() const;
    void		getDoneWells(ObjectSet<ConstRefMan<Well::Data>>&) const;
    bool		allWellsRead() const { return allwellsread_; }
			//Can then be used to launch a warning locally

protected:
    ObjectSet<ConstRefMan<Well::Data>>		wds_;
    DBKeySet					keys_;
    od_int64					nrwells_;
    od_int64					nrdone_;
    uiString					msg_;
    bool					allwellsread_ = true;
    Well::LoadReqs				loadreq_;
};
