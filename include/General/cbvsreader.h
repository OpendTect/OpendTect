#ifndef cbvsreader_h
#define cbvsreader_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	A.H.Bril
 Date:		12-3-2001
 Contents:	Common Binary Volume Storage format header
 RCS:		$Id: cbvsreader.h,v 1.32 2010-12-16 13:08:58 cvsbruno Exp $
________________________________________________________________________

-*/

#include "cbvsio.h"
#include "cbvsinfo.h"
#include "datainterp.h"
#include "cubesampling.h"
#include <iostream>



/*!\brief Reader for CBVS format

The stream it works on will be deleted on destruction or if close() is
explicitly called.

If you construct with glob_info_only == true, you cannot use the reader. After
construction, the info() is then usable but no precise positioning is available.
In other words, the trailer is not read.

From OpendTect v2.2.1, The toNext() interface will always return ascending
inlines, no matter whether the data is stored with descending inlines.

*/

mClass CBVSReader : public CBVSIO
{
public:

			CBVSReader(std::istream*,bool glob_info_only=false,
					bool forceusecbvsinfo=false);
			~CBVSReader();

    const CBVSInfo&	info() const		{ return info_; }
    void		close();

    BinID		nextBinID() const;

    bool		goTo(const BinID&,bool nearest_is_ok=false);
    bool		toStart();
    bool		toNext();

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

    const TypeSet<Coord>& trailerCoords() const	 { return trailercoords_; }

protected:

    std::istream&	strm_;
    CBVSInfo		info_;

    void		getAuxInfoSel(const char*);
    bool		readComps();
    bool		readGeom(bool);
    bool		readTrailer();
    void		getText(int,BufferString&);
    void		toOffs(od_int64);
    int			getNextBinID(BinID&,int&,int&) const;
    			//!< see nextPosIdx() for ret value
    int			getPosNr(const BinID&,bool,bool) const;
    Coord		getTrailerCoord(const BinID&) const;
    void		mkPosNrs();
    bool		goToPosNrOffs(int posnr);
    void		setPos(int,const BinID&,int,int);

private:

    bool		hinfofetched;
    int			bytespertrace;
    BinID		firstbinid;
    BinID		lastbinid;
    int			posidx;
    int			auxnrbytes;
    bool		needaux;
    DataInterpreter<int> iinterp;
    DataInterpreter<float> finterp;
    DataInterpreter<double> dinterp;
    HorSampling		hs;
    Interval<int>	samprg;
    TypeSet<int>	posnrs;

    bool		readInfo(bool,bool);
    int			nextPosIdx();
    			//!< 0 = no more traces
    			//!< 1 = next trace adjacent to current
    			//!< 2 = next trace needs a jump
    od_int64		lastposfo;
    od_int64		datastartfo;

    friend class	CBVSReadMgr;
    mutable int		curinlinfnr_;
    mutable int		cursegnr_;
    CoordPol		coordPol() const	{ return coordpol_; }

};


#endif
