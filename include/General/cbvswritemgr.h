#ifndef cbvswritemgr_h
#define cbvswritemgr_h

/*+
________________________________________________________________________

 CopyRight:	(C) de Groot-Bril Earth Sciences B.V.
 Author:	A.H.Bril
 Date:		12-3-2001
 Contents:	Common Binary Volume Storage format writer
 RCS:		$Id: cbvswritemgr.h,v 1.5 2002-07-24 17:08:12 bert Exp $
________________________________________________________________________

-*/

#include <cbvsio.h>
#include <cbvsinfo.h>
#include <iosfwd>

class CBVSWriter;


/*!\brief Writer for CBVS file packs */

class CBVSWriteMgr : public CBVSIOMgr
{
public:

			CBVSWriteMgr(const char* basefname,const CBVSInfo&,
					const PosAuxInfo* =0);
			//!< See CBVSWriter for last two parameters
			~CBVSWriteMgr();

    unsigned long	bytesPerFile() const;
			//!< After this amount of bytes, a new file will
			//!< be created for the next inline.
			//!< The default is 1.8 GB, 0 = unlimited
    void		setBytesPerFile(unsigned long);

    bool		put(void**);
			//!< See CBVSWriter::put, only now succeeds or fails
    void		close();
			//!< See CBVSWriter::close

    int			nrComponents() const;
    const BinID&	binID() const;

protected:

    CBVSWriter*		writer_;
    CBVSInfo		info_;
    BufferString	fname_;

    const char*		errMsg_() const;

    ostream*		mkStrm();

};


#endif
