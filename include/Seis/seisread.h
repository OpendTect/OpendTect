#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	A.H. Bril
 Date:		27-1-98
________________________________________________________________________

-*/

#include "seisstor.h"
#include "uistring.h"
class SeisTrc;
class TrcKeySampling;
class Seis2DTraceGetter;
class SeisTrcBuf;
class SeisPS3DReader;
class SeisPS2DReader;
namespace Seis { class Bounds; class Bounds2D; }
namespace PosInfo
{
    class CubeData; class CubeDataIterator;
    class Line2DData; class Line2DDataIterator;
}


/*!\brief read from a seismic data store. Deprecated. Use Seis::Provider. */

mExpClass(Seis) SeisTrcReader : public SeisStoreAccess
{ mODTextTranslationClass(SeisTrcReader);
public:

			mDeprecated SeisTrcReader(const IOObj* =0);
			mDeprecated SeisTrcReader(const char* fnm);
			~SeisTrcReader();

    void		forceFloatData( bool yn=true )	{ forcefloats = yn; }
    bool		prepareWork(Seis::ReadMode rm=Seis::Prod);
    int			get(SeisTrcInfo&);
    bool		get(SeisTrc&);
    void		fillPar(IOPar&) const;
    bool		isPrepared() const		{ return prepared; }
    Seis::Bounds*	getBounds() const;
			//!< use after prepareWork(). If not avail: survinfo
    bool		get3DGeometryInfo(PosInfo::CubeData&) const;
    void		setComponent( int ic )		{ selcomp_ = ic; }
			//!< use before startWork()
			//!< -1 (default) is all components

			// 2D only
    int			curLineIdx() const		{ return curlineidx; }
    StepInterval<int>	curTrcNrRange() const		{ return curtrcnrrg; }
    Pos::GeomID		geomID() const;
    int			getNrOffsets(int maxnrpostobechecked=10) const;

protected:

    bool		foundvalidinl, foundvalidcrl;
    bool		needskip;
    bool		forcefloats;
    bool		prepared;
    bool		inforead;
    int			prev_inl;
    int			curlineidx;
    int			nrlinegetters_;
    TrcKeySampling*	outer;
    SeisTrcBuf*		tbuf_;
    Seis2DTraceGetter*	tracegetter_;
    Seis::ReadMode	readmode;
    bool		entryis2d;
    StepInterval<int>	curtrcnrrg;
    SeisPS2DReader*	psrdr2d_;
    SeisPS3DReader*	psrdr3d_;
    PosInfo::CubeDataIterator* pscditer_;
    PosInfo::Line2DDataIterator* pslditer_;
    BinID		curpsbid_;
    int			selcomp_;

    void		init();
    Conn*		openFirst();
    bool		initRead(Conn*);
    int			nextConn(SeisTrcInfo&);
    bool		doStart();
    bool		ensureCurLineAttribOK(const BufferString&);

    bool		isMultiConn() const;
    bool		startWork();

    int			getPS(SeisTrcInfo&);
    bool		getPS(SeisTrc&);

    int			get2D(SeisTrcInfo&);
    bool		get2D(SeisTrc&);
    bool		mkNextGetter();
    bool		readNext2D();

    Seis::Bounds*	get3DBounds(const StepInterval<int>&,
				    const StepInterval<int>&,
				    const StepInterval<float>&) const;
    bool		initBounds2D(const PosInfo::Line2DData&,
				     Seis::Bounds2D&) const;
};
