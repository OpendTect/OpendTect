#ifndef cbvsreader_h
#define cbvsreader_h

/*+
________________________________________________________________________

 CopyRight:	(C) de Groot-Bril Earth Sciences B.V.
 Author:	A.H.Bril
 Date:		12-3-2001
 Contents:	Common Binary Volume Storage format header
 RCS:		$Id: cbvsreader.h,v 1.5 2001-04-06 16:38:36 bert Exp $
________________________________________________________________________

-*/

#include <cbvsio.h>
#include <cbvsinfo.h>
#include <datainterp.h>
#include <iostream.h>
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
    bool		skip(bool force_next_position=false);
			//!< if force_next_position, will skip all traces
			//!< at current position.

    bool		getHInfo(CBVSInfo::ExplicitData&);
    bool		fetch(void**,const Interval<int>* samps=0);
			//!< If 'samps' is non-null, it should hold start
			//!< and end sample for each component.
			//!< If start > stop component is skipped.

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
    void		nextPosIdx();

    streampos		lastposfo;
    streampos		datastartfo;

};


#endif
