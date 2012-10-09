#ifndef cbvsreadmgr_h
#define cbvsreadmgr_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	A.H.Bril
 Date:		14-4-2001
 Contents:	Common Binary Volume Storage read manager
 RCS:		$Id$
________________________________________________________________________

-*/

#include "cbvsio.h"
#include "cbvsinfo.h"
#include "datainterp.h"
#include <iosfwd>
class CBVSReader;
class CBVSInfo;
class CubeSampling;


/*!\brief Manager for reading CBVS file-packs.

*/

mClass CBVSReadMgr : public CBVSIOMgr
{
public:

			CBVSReadMgr(const char*,const CubeSampling* cs=0,
				    bool single_file=false,
				    bool glob_info_only=false,
				    bool forceusecbvsinfo=false);
			//!< glob_info_only: I am useless except for inspecting
			//!< global info. See also CBVSReader.
			~CBVSReadMgr();

    const CBVSInfo&	info() const		{ return info_; }
    void		close();

    BinID		nextBinID() const;
    bool		goTo(const BinID&);
    bool		toNext();
    bool		toStart();

    bool		getAuxInfo(PosAuxInfo&);
    bool		fetch(void**,const bool* comps=0,
				const Interval<int>* samps=0);
			//!< See CBVSReader::fetch comments

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
    int			pruneReaders(const CubeSampling&);
    			//!< returns number of readers left.

    void		dumpInfo(std::ostream&,bool include_compinfo) const;
    const TypeSet<Coord>& trailerCoords() const;
    void		getPositions(TypeSet<BinID>&) const;
    void		getPositions(TypeSet<Coord>&) const;
    			//!< This may actually reset the position to the first

protected:

    ObjectSet<CBVSReader> readers_;
    CBVSInfo&		info_;
    bool		vertical_;
    int			rdr1firstsampnr_;

    bool		addReader(std::istream*,const CubeSampling*,bool,bool);
    bool		addReader(const char*,const CubeSampling*,bool,bool);
    int			nextRdrNr(int) const;
    const char*		errMsg_() const;

private:

    void		createInfo();
    bool		handleInfo(CBVSReader*,int);

};


#endif
