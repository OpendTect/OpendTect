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

    bool		isOldFormat() const;	//!< Just checks if log hdrs
			//have dah ranges. If not, need to write first.

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
			MultiWellReader(const TypeSet<MultiID>&,
					ObjectSet<Well::Data>&);

    int			nextStep();
    od_int64		totalNr() const;
    od_int64		nrDone() const;
    uiString		uiMessage() const;
    uiString		uiNrDoneText() const;

protected:
    ObjectSet<Well::Data>&	wds_;
    TypeSet<MultiID>		keys_;
    od_int64			nrwells_;
    od_int64			nrdone_;
};

#endif
