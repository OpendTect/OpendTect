#ifndef wellreader_h
#define wellreader_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Bert Bril
 Date:		Aug 2003
 RCS:		$Id$
________________________________________________________________________


-*/

#include "wellmod.h"
#include "executor.h"
#include "ranges.h"
#include "bufstring.h"
#include "uistring.h"
class IOObj;
class BufferStringSet;


namespace Well
{
class Data;
class ReadAccess;
class LogInfo;


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
    bool		getLogs() const;	//!< Read logs only
    bool		getMarkers() const;	//!< Read Markers only
    bool		getD2T() const;		//!< Read D2T model parts
    bool		getCSMdl() const;	//!< Read Checkshot model parts
    bool		getDispProps() const;	//!< Read display props only
    bool		getLog(const char* lognm) const; //!< Read this one only
    bool		getLogInfo() const;
    void		getLogInfo(BufferStringSet& lognms) const;

    const OD::String&	errMsg() const		{ return errmsg_; }
    Well::Data*		data();
    const Well::Data*	data() const
			{ return const_cast<Reader*>(this)->data(); }

    bool		getMapLocation(Coord&) const;

protected:

    ReadAccess*		ra_;
    mutable BufferString errmsg_;

private:

    void		init(const IOObj&,Data&);

};

} // namespace Well


mExpClass(Well) MultiWellReader : public Executor
{ mODTextTranslationClass(MultiWellReader)
public:

    mExpClass(Well) Setup
    {
    public:
			Setup()
			: track_(false)
			, logs_(false)
			, markers_(false)
			, D2T_(false)
			, csmdl_(false)
			, loginfo_(false)
			{}

	mDefSetupMemb(bool,track)
	mDefSetupMemb(bool,logs)
	mDefSetupMemb(bool,markers)
	mDefSetupMemb(bool,D2T)
	mDefSetupMemb(bool,csmdl)
	mDefSetupMemb(bool,loginfo)
    };

			MultiWellReader(const TypeSet<MultiID>&,
					ObjectSet<Well::Data>&,	
					const Setup&);

    int				nextStep();
    od_int64			totalNr() const;
    od_int64			nrDone() const;
    uiString			uiMessage() const;
    uiString			uiNrDoneText() const;
    bool			allWellsRead() const { return allwellsread_; }
				//Can then be used to launch a warning locally
    const uiString&		errMsg() const		{ return errmsg_; }

protected:
    ObjectSet<Well::Data>&	wds_;
    TypeSet<MultiID>		keys_;
    od_int64			nrwells_;
    od_int64			nrdone_;
    uiString			errmsg_;
    bool			allwellsread_ = true;
    const Setup&		su_;
};

#endif
