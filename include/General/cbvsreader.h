#ifndef cbvsreader_h
#define cbvsreader_h

/*+
________________________________________________________________________

 CopyRight:	(C) de Groot-Bril Earth Sciences B.V.
 Author:	A.H.Bril
 Date:		12-3-2001
 Contents:	Common Binary Volume Storage format header
 RCS:		$Id: cbvsreader.h,v 1.11 2001-06-26 15:02:48 bert Exp $
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
    bool		fetch(void**,const bool* comps=0,
				const Interval<int>* samps=0);
			//!< 'comps',if provided, selects the compnents
			//!< If 'samps' is non-null, it should hold start
			//!< and end sample for each component.

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
    BinID		lastbinid;
    int			curinlinfnr;
    int			cursegnr;
    int			posidx;
    int			explicitnrbytes;
    DataInterpreter<int> iinterp;
    DataInterpreter<float> finterp;
    DataInterpreter<double> dinterp;
    BinIDRange&		bidrg;
    Interval<int>*	samprgs;

    bool		readInfo();
    bool		nextPosIdx();

    streampos		lastposfo;
    streampos		datastartfo;

};


#endif
