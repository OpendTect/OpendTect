#ifndef cbvsreadmgr_h
#define cbvsreadmgr_h

/*+
________________________________________________________________________

 CopyRight:	(C) de Groot-Bril Earth Sciences B.V.
 Author:	A.H.Bril
 Date:		14-4-2001
 Contents:	Common Binary Volume Storage read manager
 RCS:		$Id: cbvsreadmgr.h,v 1.1 2001-04-18 16:24:18 bert Exp $
________________________________________________________________________

-*/

#include <cbvsinfo.h>
class CBVSReader;


/*!\brief Manager for reading CBVS file-packs.

*/

class CBVSReadMgr
{
public:

			CBVSReadMgr(const char*);
			~CBVSReadMgr();

    bool		failed() const		{ return *(const char*)errmsg_;}
    const char*		errMsg() const		{ return (const char*)errmsg_; }

    void		close();

    BinID		nextBinID() const;
    bool		goTo(const BinID&);
    bool		toNext();
    bool		skip(bool force_next_position=false);
			//!< See CBVSReader::skip comments

    bool		getHInfo(CBVSInfo::ExplicitData&);
    bool		fetch(void**,const Interval<int>* samps=0);
			//!< See CBVSReader::fetch comments

    static const char*	check(const char*);
			//!< Determines whether this is a CBVS file pack.
			//!< returns an error message, or null if OK.

    const char*		baseFileName() const
			{ return (const char*)basefname_; }
    const CBVSInfo&	info();

protected:

    BufferString	basefname_;
    ObjectSet<CBVSReader> readers_;
    ObjectSet<BufferString> fnames_;
    int			curreader_;
    CBVSInfo&		info_;
    BufferString	errmsg_;

    bool		addReader(const char*);

private:

    void		createInfo();
    bool		handleInfo(CBVSReader*,int);
    bool		mergeIrreg(const CBVSInfo::SurvGeom&,int);

};


#endif
