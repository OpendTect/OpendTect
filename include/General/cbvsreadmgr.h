#ifndef cbvsreadmgr_h
#define cbvsreadmgr_h

/*+
________________________________________________________________________

 CopyRight:	(C) de Groot-Bril Earth Sciences B.V.
 Author:	A.H.Bril
 Date:		14-4-2001
 Contents:	Common Binary Volume Storage read manager
 RCS:		$Id: cbvsreadmgr.h,v 1.9 2002-07-24 17:08:12 bert Exp $
________________________________________________________________________

-*/

#include <cbvsio.h>
#include <cbvsinfo.h>
#include <datainterp.h>
#include <iosfwd>
class CBVSReader;
class CBVSInfo;
class CubeSampling;


/*!\brief Manager for reading CBVS file-packs.

*/

class CBVSReadMgr : public CBVSIOMgr
{
public:

			CBVSReadMgr(const char*,const CubeSampling* cs=0);
			~CBVSReadMgr();

    const CBVSInfo&	info() const		{ return info_; }
    void		close();

    BinID		nextBinID() const;
    bool		goTo(const BinID&);
    bool		toNext();
    bool		toStart();
    bool		skip(bool force_next_position=false);
			//!< See CBVSReader::skip comments

    bool		getAuxInfo(PosAuxInfo&);
    bool		fetch(void**,const bool* comps=0,
				const Interval<int>* samps=0);
			//!< See CBVSReader::fetch comments

    static const char*	check(const char*);
			//!< Determines whether this is a CBVS file pack.
			//!< returns an error message, or null if OK.

    int			nrComponents() const;
    const BinID&	binID() const;
    bool		hasAuxInfo() const		{ return haveaux_; }
    void		fetchAuxInfo(bool yn=true);
    			//!< Single shot. Second time, may not work properly.

    const char*		baseFileName() const
			{ return (const char*)basefname_; }

    int			nrReaders() const
			{ return readers_.size(); }
    const CBVSReader&	reader( int idx ) const
			{ return *readers_[idx]; }
    int			pruneReaders(const CubeSampling&);
    			//!< returns number of readers left.

protected:

    ObjectSet<CBVSReader> readers_;
    ObjectSet<BufferString> fnames_;
    CBVSInfo&		info_;
    bool		vertical_;
    DataInterpreter<int> iinterp;
    DataInterpreter<float> finterp;
    DataInterpreter<double> dinterp;

    bool		addReader(istream*,const CubeSampling*);
    bool		addReader(const char*,const CubeSampling*);
    int			nextRdrNr(int) const;
    const char*		errMsg_() const;

    bool		haveaux_;
    istream*		auxstrm_;
    int			auxinlidx_;
    int			auxcrlidx_;
    ObjectSet<AuxInlInf> auxinlinfs_;
    int			auxnrbytes_;
    unsigned char	auxflgs_;

    void		getAuxFromFile(PosAuxInfo&);

private:

    void		createInfo();
    bool		handleInfo(CBVSReader*,int);
    void		handleAuxFile();
    void		handleAuxTrailer();

};


#endif
