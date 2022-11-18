#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "seismod.h"
#include "seisstor.h"
#include "seistype.h"
#include "linekey.h"
#include "uistring.h"

class Executor;
class RegularSeisDataPack;
class Scaler;
class SeisPS2DReader;
class SeisPS3DReader;
class SeisTrc;
class SeisTrcBuf;
class TaskRunner;
class TraceData;
class TrcKeySampling;
namespace Seis
{
    class Bounds;
    class Bounds2D;
}
namespace PosInfo
{
    class CubeData;
    class CubeDataIterator;
    class Line2DData;
    class Line2DDataIterator;
}


/*!\brief reads from a seismic data store.

If you don't want all of the stored data, you must set use the
SeisTrcTranslator facilities (SelData and ComponentData) after calling
prepareWork(). If you don't call prepareWork(), the reader will do that but
you cannot use SeisTrcTranslator facilities then.

Then, the routine is: get(trc.info()) possibly followed by get(trc).
Not keeping this sequence is at your own risk.

Note: 2D Prestack data cannot (yet) be read via this class.

*/

mExpClass(Seis) SeisTrcReader : public SeisStoreAccess
{ mODTextTranslationClass(SeisTrcReader);
public:

			SeisTrcReader(const MultiID&,Seis::GeomType);
				//!< Open 'real user entries from '.omf' file
				//!< Can be anything: SEGY - CBVS - database
			SeisTrcReader(const IOObj&,
				      const Seis::GeomType* =nullptr);
				//!< Open 'real user entries from '.omf' file
				//!< Can be anything: SEGY - CBVS - database
			SeisTrcReader(const IOObj&,Pos::GeomID,
				      const Seis::GeomType* =nullptr);
				//!< Restricted to a given Pos::GeomID
			SeisTrcReader(const SeisStoreAccess::Setup&);
			~SeisTrcReader();

    void		forceFloatData( bool yn=true )	{ forcefloats = yn; }
			//!< Only effective if called before prepareWork()
    bool		prepareWork(Seis::ReadMode =Seis::Prod);
			//!< After this, you can set stuff on the translator
			//!< If not called, will be done automatically

    int			expectedNrTraces() const;
			/*!< Not the number of positions, as Pre-Stack
			     datasets have many traces per position */

    int			get(SeisTrcInfo&);
			/*!< -1 = Error. errMsg() will return a message.
			      0 = End
			      1 = Usable info
			      2 = Not usable (trace needs to be skipped)
			      If 1 is returned, then you should also call
			      get(SeisTrc&). */

    bool		getData(TraceData&);
    bool		getDataPack(RegularSeisDataPack&,TaskRunner* =nullptr);

    bool		get(SeisTrc&);
			/*!< It is possible to directly call this without
			     checking the get(SeisTrcInfo&) result. Beware that
			     the trace selections in the SelData may be
			     ignored then - depending on the Translator's
			     capabilities. */


    void		fillPar(IOPar&) const override;

    Seis::Bounds*	getBounds() const;
			//!< use after prepareWork(). If not avail: survinfo
    bool		get3DGeometryInfo(PosInfo::CubeData&) const;
    void		setComponent( int ic )		{ selcomp_ = ic; }
			//!< use before startWork()
			//!< -1 (default) is all components

			// 2D only
    int			curLineIdx() const		{ return curlineidx; }
    StepInterval<int>	curTrcNrRange() const		{ return curtrcnrrg; }
    Pos::GeomID		geomID() const override;
    GeomIDProvider*	geomIDProvider() const;
    int			getNrOffsets(int maxnrpostobechecked=10) const;

    const SeisTrcTranslator*	seis2Dtranslator();

    const Scaler*	getTraceScaler() const;

protected:

    bool		foundvalidinl = false;
    bool		foundvalidcrl = false;
    bool		new_packet = false;
    bool		needskip = false;
    bool		forcefloats = false;
    bool		inforead = false;
    int			prev_inl = mUdf(int);
    int			curlineidx = -1;
    int			nrfetchers = 0;
    TrcKeySampling*	outer;
    SeisTrcBuf*		tbuf_ = nullptr;
    Executor*		fetcher = nullptr;
    Seis::ReadMode	readmode = Seis::Prod;
    bool		entryis2d = false;
    StepInterval<int>	curtrcnrrg;
    SeisPS2DReader*	psrdr2d_ = nullptr;
    SeisPS3DReader*	psrdr3d_ = nullptr;
    PosInfo::CubeDataIterator* pscditer_ = nullptr;
    PosInfo::Line2DDataIterator* pslditer_ = nullptr;
    BinID		curpsbid_ = BinID(0,0);
    int			selcomp_ = -1;

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
    bool		get2DData(TraceData&);
    bool		mkNextFetcher();
    bool		readNext2D();

    Seis::Bounds*	get3DBounds(const StepInterval<int>&,
				    const StepInterval<int>&,
				    const StepInterval<float>&) const;
    bool		initBounds2D(const PosInfo::Line2DData&,
				     Seis::Bounds2D&) const;

			SeisTrcReader(const MultiID&)		= delete;

public:
    mDeprecated("Provide an existing IOObj")
			SeisTrcReader(const IOObj* =nullptr);

    explicit		SeisTrcReader(const char* fnm);
				//!< Open 'loose' CBVS files only.
};
