#ifndef cbvsreader_h
#define cbvsreader_h

/*+
________________________________________________________________________

 CopyRight:	(C) de Groot-Bril Earth Sciences B.V.
 Author:	A.H.Bril
 Date:		12-3-2001
 Contents:	Common Binary Volume Storage format header
 RCS:		$Id: cbvsreader.h,v 1.2 2001-04-04 11:13:27 bert Exp $
________________________________________________________________________

-*/

#include <cbvsio.h>
#include <cbvsinfo.h>
#include <datainterp.h>
#include <iostream.h>


/*!\brief Reader for CBVS format

The stream it works on will be deleted on destruction or closed
if close() is explicitly called.

*/

class CBVSReader : public CBVSIO
{
public:

			CBVSReader(istream*);
			~CBVSReader()	{ close(); }

    const CBVSInfo&	info() const	{ return info_; }
    void		close();

    bool		goTo(const BinID&);
    bool		toNext(bool skip_trcs_at_same_position=false);
    BinID		binID() const	{ return curbinid; }

    bool		getHInfo(CBVSInfo::ExplicitData&);
    bool		fetch(void**);

    static const char*	check(istream&);
			//!< Determines whether a file is a CBVS file
			//!< returns an error message or null if OK.

protected:

    istream&		strm_;
    CBVSInfo		info_;

    void		getExplicits(const unsigned char*);
    bool		readComps();
    bool		readGeom();
    bool		readTrailer();

private:

    int			nrxlines;
    int			bytespertrace;
    BinID		curbinid;
    bool		hinfofetched;
    int			lastinlinfnr;
    int			lastsegnr;
    int			posidx;
    int			explicitnrbytes;
    DataInterpreter<int> iinterp;
    DataInterpreter<float> finterp;
    DataInterpreter<double> dinterp;
    bool		isclosed;

    streampos		readInfo();

    streampos		lastposfo;
			// Do not move datastartfo declaration upward!
    const streampos	datastartfo;

};


#endif
