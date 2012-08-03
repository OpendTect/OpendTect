#ifndef cbvsreader_h
#define cbvsreader_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	A.H.Bril
 Date:		12-3-2001
 Contents:	Common Binary Volume Storage format header
 RCS:		$Id: cbvsreader.h,v 1.34 2012-08-03 13:00:21 cvskris Exp $
________________________________________________________________________

-*/

#include "generalmod.h"
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

mClass(General) CBVSReader : public CBVSIO
{
public:

			CBVSReader(std::istream*,bool glob_info_only=false,
					bool forceusecbvsinfo=false);
			~CBVSReader();

    const CBVSInfo&	info() const		{ return info_; }
    void		close();
    BinID		nextBinID() const;	//! returns 0/0 at end

    bool		goTo(const BinID&);
    bool		toStart();
    bool		toNext();

    bool		hasAuxInfo() const	{ return auxnrbytes_; }
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

    int			trcNrAtPosition() const	{ return idxatpos_; }

    const TypeSet<Coord>& trailerCoords() const	{ return trailercoords_; }

protected:

    std::istream&	strm_;
    CBVSInfo		info_;

    void		getAuxInfoSel(const char*);
    bool		readComps();
    bool		readGeom(bool);
    bool		readTrailer();
    void		getText(int,BufferString&);
    void		toOffs(od_int64);
    int			getPosNr(const PosInfo::CubeDataPos&,bool) const;
    Coord		getTrailerCoord(const BinID&) const;
    void		mkPosNrs();

private:

    bool		hinfofetched_;
    int			bytespertrace_;
    BinID		firstbinid_;
    int			idxatpos_;
    int			auxnrbytes_;
    DataInterpreter<int> iinterp_;
    DataInterpreter<float> finterp_;
    DataInterpreter<double> dinterp_;
    HorSampling		hs_;
    Interval<int>	samprg_;
    TypeSet<int>	posnrs_;

    bool		readInfo(bool,bool);
    od_int64		lastposfo_;
    od_int64		datastartfo_;

    friend class	CBVSReadMgr;
    mutable PosInfo::CubeDataPos curgeomcubepos_;
    mutable PosInfo::CubeDataPos curldscubepos_;
    CoordPol		coordPol() const	{ return coordpol_; }

    void		setCubePos(bool fromgeom) const;
    void		updCurBinID() const;

};


#endif

