#ifndef cbvsreader_h
#define cbvsreader_h

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	A.H.Bril
 Date:		12-3-2001
 Contents:	Common Binary Volume Storage format header
 RCS:		$Id: cbvsreader.h,v 1.21 2004-04-27 15:51:15 bert Exp $
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

			CBVSReader(std::istream*);
			~CBVSReader();

    const CBVSInfo&	info() const		{ return info_; }
    void		close();

    BinID		nextBinID() const;

    bool		goTo(const BinID&);
    bool		toStart();
    bool		toNext()	{ return skip(false); }
    bool		skip(bool force_next_position=false);
			//!< if force_next_position, will skip all traces
			//!< at current position.

    bool		hasAuxInfo() const		{ return auxnrbytes; }
    void		fetchAuxInfo( bool yn=true )	{ needaux = yn; }
    bool		getAuxInfo(PosAuxInfo&);
    			//!< Gets the aux info. Follow by
			//!< fetch() to get the sample data.
    bool		fetch(void** buffers,const bool* comps=0,
				const Interval<int>* samps=0,
				int offs=0);
    			//!< Gets the sample data.
			//!< 'comps', if provided, selects the components.
			//!< If 'samps' is non-null, it should hold start
			//!< and end sample to read. offs is an offset
			//!< in the buffers.

    static const char*	check(std::istream&);
			//!< Determines whether a file is a CBVS file
			//!< returns an error message, or null if OK.

    int			trcNrAtPosition() const		{ return posidx; }
    const BinIDRange&	binIDRange() const		{ return bidrg; }

protected:

    std::istream&	strm_;
    CBVSInfo		info_;

    void		getAuxInfoSel(const char*);
    bool		readComps();
    bool		readGeom();
    bool		readTrailer();
    void		getText(int,BufferString&);
    void		toOffs(std::streampos);
    bool		getNextBinID(BinID&,int&,int&);
    int			getPosNr(const BinID&,int&,int&) const;
    bool		goTo(int posnr,const BinID&,int,int);
    void		mkPosNrs();

private:

    bool		hinfofetched;
    int			bytespertrace;
    BinID		firstbinid;
    BinID		lastbinid;
    int			curinlinfnr_;
    int			cursegnr_;
    int			posidx;
    int			auxnrbytes;
    bool		needaux;
    DataInterpreter<int> iinterp;
    DataInterpreter<float> finterp;
    DataInterpreter<double> dinterp;
    BinIDRange&		bidrg;
    Interval<int>	samprg;
    TypeSet<int>	posnrs;

    bool		readInfo();
    bool		nextPosIdx();

    std::streampos	lastposfo;
    std::streampos	datastartfo;

    friend class	CBVSReadMgr;
    void		setCurBinID( const BinID& b )
			{ curbinid_ = b; }

};


#endif
