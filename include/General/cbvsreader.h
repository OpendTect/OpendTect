#ifndef cbvsreader_h
#define cbvsreader_h

/*+
________________________________________________________________________

 CopyRight:	(C) de Groot-Bril Earth Sciences B.V.
 Author:	A.H.Bril
 Date:		12-3-2001
 Contents:	Common Binary Volume Storage format header
 RCS:		$Id: cbvsreader.h,v 1.14 2002-07-21 23:17:42 bert Exp $
________________________________________________________________________

-*/

#include <cbvsio.h>
#include <cbvsinfo.h>
#include <datainterp.h>
#include <iostream>

class BinIDRange;


/*!\brief Reader for CBVS format

The stream it works on will be deleted on destruction or if close() is
explicitly called.

*/

class CBVSReader : public CBVSIO
{
public:

			CBVSReader(istream*);
			~CBVSReader();

    const CBVSInfo&	info() const	{ return info_; }
    void		close();

    BinID		nextBinID() const;

    bool		goTo(const BinID&);
    bool		toStart();
    bool		toNext()	{ return skip(false); }
    bool		skip(bool force_next_position=false);
			//!< if force_next_position, will skip all traces
			//!< at current position.

    bool		getHInfo(CBVSInfo::ExplicitData&);
    			//!< Gets the explicit header info. Follow by
			//!< fetch() to get the sample data.
    bool		fetch(void** buffers,const bool* comps=0,
				const Interval<int>* samps=0,
				int offs=0);
    			//!< Gets the sample data.
			//!< 'comps', if provided, selects the components.
			//!< If 'samps' is non-null, it should hold start
			//!< and end sample to read. offs is an offset
			//!< in the buffers.

    static const char*	check(istream&);
			//!< Determines whether a file is a CBVS file
			//!< returns an error message, or null if OK.

protected:

    istream&		strm_;
    CBVSInfo		info_;

    void		getExplicits(const unsigned char*);
    bool		readComps();
    bool		readGeom();
    bool		readTrailer();
    void		getText(int,BufferString&);
    void		toOffs(streampos);
    bool		getNextBinID(BinID&,bool);

private:

    bool		hinfofetched;
    int			bytespertrace;
    BinID		firstbinid;
    BinID		lastbinid;
    int			curinlinfnr;
    int			cursegnr;
    int			posidx;
    int			explicitnrbytes;
    DataInterpreter<int> iinterp;
    DataInterpreter<float> finterp;
    DataInterpreter<double> dinterp;
    BinIDRange&		bidrg;
    Interval<int>	samprg;

    bool		readInfo();
    bool		nextPosIdx();

    streampos		lastposfo;
    streampos		datastartfo;

};


#endif
