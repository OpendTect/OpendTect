#ifndef cbvsreadmgr_h
#define cbvsreadmgr_h

/*+
________________________________________________________________________

 CopyRight:	(C) de Groot-Bril Earth Sciences B.V.
 Author:	A.H.Bril
 Date:		14-4-2001
 Contents:	Common Binary Volume Storage read manager
 RCS:		$Id: cbvsreadmgr.h,v 1.3 2001-05-25 18:24:51 bert Exp $
________________________________________________________________________

-*/

#include <cbvsio.h>
#include <cbvsinfo.h>
class CBVSReader;
class CBVSInfo;


/*!\brief Manager for reading CBVS file-packs.

*/

class CBVSReadMgr : public CBVSIOMgr
{
public:

			CBVSReadMgr(const char*);
			~CBVSReadMgr();

    const CBVSInfo&	info() const		{ return info_; }
    void		close();

    BinID		nextBinID() const;
    bool		goTo(const BinID&);
    bool		toNext();
    bool		toStart();
    bool		skip(bool force_next_position=false);
			//!< See CBVSReader::skip comments

    bool		getHInfo(CBVSInfo::ExplicitData&);
    bool		fetch(void**,const Interval<int>* samps=0);
			//!< See CBVSReader::fetch comments

    static const char*	check(const char*);
			//!< Determines whether this is a CBVS file pack.
			//!< returns an error message, or null if OK.

    int			nrComponents() const;
    const BinID&	binID() const;

    const char*		baseFileName() const
			{ return (const char*)basefname_; }

protected:

    ObjectSet<CBVSReader> readers_;
    ObjectSet<BufferString> fnames_;
    CBVSInfo&		info_;

    bool		addReader(const char*);
    int			nextRdrNr(int) const;
    const char*		errMsg_() const;

private:

    void		createInfo();
    bool		handleInfo(CBVSReader*,int);

};


#endif
