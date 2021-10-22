#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	A.H.Bril
 Date:		14-4-2001
 Contents:	Common Binary Volume Storage read manager
________________________________________________________________________

-*/

#include "generalmod.h"
#include "cbvsio.h"
#include "cbvsinfo.h"
#include "datainterp.h"
class CBVSInfo;
class CBVSReader;
class TraceData;
class TrcKeyZSampling;
class od_ostream;


/*!\brief Manager for reading CBVS file-packs.

*/

mExpClass(General) CBVSReadMgr : public CBVSIOMgr
{
public:

			CBVSReadMgr(const char*,const TrcKeyZSampling* cs=0,
				    bool single_file=false,
				    bool glob_info_only=false,
				    bool forceusecbvsinfo=false);
			//!< glob_info_only: I am useless except for inspecting
			//!< global info. See also CBVSReader.
			~CBVSReadMgr();

    const CBVSInfo&	info() const		{ return info_; }
    int			bytesOverheadPerTrace() const;
    void		close();
    void		setSingleLineMode(bool yn=true);

    BinID		nextBinID() const;
    bool		goTo(const BinID&);
    bool		toNext();
    bool		toStart();

    bool		getAuxInfo(PosAuxInfo&);
    bool		fetch(void**,const bool* comps=0,
				const Interval<int>* samps=0);
    bool		fetch(TraceData&,const bool* comps,
				const Interval<int>* samps);
			//!< See CBVSReader::fetch comments
    bool		fetch(TraceData&,const bool* comps=nullptr,
				const StepInterval<int>* samps=nullptr);

    static const char*	check(const char*);
			//!< Determines whether this is a CBVS file pack.
			//!< returns an error message, or null if OK.

    int			nrComponents() const;
    const BinID&	binID() const;
    void		getIsRev(bool& inl, bool& crl) const;

    const char*		baseFileName() const
			{ return (const char*)basefname_; }

    int			nrReaders() const
			{ return readers_.size(); }
    const CBVSReader&	reader( int idx ) const
			{ return *readers_[idx]; }
    int			pruneReaders(const TrcKeyZSampling&);
			//!< returns number of readers left.

    void		dumpInfo(od_ostream&,bool include_compinfo) const;
    const TypeSet<Coord>& trailerCoords() const;
    void		getPositions(TypeSet<BinID>&) const;
    void		getPositions(TypeSet<Coord>&) const;
			//!< This may actually reset the position to the first

protected:

    ObjectSet<CBVSReader> readers_;
    CBVSInfo&		info_;
    bool		vertical_;
    int			rdr1firstsampnr_;

    bool		addReader(od_istream*,const TrcKeyZSampling*,bool,bool);
    bool		addReader(const char*,const TrcKeyZSampling*,bool,bool);
    int			nextRdrNr(int) const;
    const char*		errMsg_() const;

private:

    void		createInfo();
    bool		handleInfo(CBVSReader*,int);

};


