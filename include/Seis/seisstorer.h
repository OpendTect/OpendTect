#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Bert
 Date:		27-1-98
________________________________________________________________________

-*/

#include "seiscommon.h"
#include "uistring.h"
#include "iopar.h"
#include "geomid.h"
#include "binid.h"
class Conn;
class Scaler;
class Seis2DDataSet;
class Seis2DLinePutter;
class SeisPSIOProvider;
class SeisPSWriter;
class SeisTrc;
class SeisTrcTranslator;
namespace Threads { class ConditionVar; }
namespace PosInfo { class Line2DData; }


namespace Seis
{

class SelData;
class SingleTraceStorer;
class StatsCollector;


/* stores seismic data to persistent store.

Note:

* The traces to be written have to be of the same type as the storage.
    Thus, the TrcKey needs to be of the same GeomSystem, or Synthetic.
    Synthetic traces can be written to 2D or 3D. If you need 2D, then set a
    fixed GeomID. If input traces lack a GeomID, the 3D GeomID will be used.

* For 2D storage, the 2D Geometry for the GeomID in the TrcKey
    has to *already exist*. If it is empty when the first trace is put, we'll
    detect the geometry and write it on close. If it is not empty, then
    we'll keep the original 2D Geometry.

*  prepareWork() is optional, if you don't call it yourself it will be called
    on the first put().

   */

mExpClass(Seis) Storer
{ mODTextTranslationClass(Seis::Storer);
public:

    mUseType( Pos,	GeomID );
    typedef int		idx_type;
    typedef idx_type	size_type;

			Storer();
			Storer(const DBKey&);
			Storer(const IOObj&);
			~Storer();
    uiRetVal		close();

    bool		is2D() const;
    bool		isPS() const			{ return psioprov_; }
    bool		is2DLine() const		{ return dataset2d_; }
    GeomType		geomType() const
			{ return geomTypeOf(is2D(),isPS()); }
    const IOObj*	ioObj() const			{ return ioobj_; }

    bool		setOutput(const IOObj&);
    bool		setOutput(const DBKey&);
    bool		isUsable() const;
    uiString		errNotUsable() const;
    void		setFixedGeomID( GeomID gid )	{ fixedgeomid_ = gid; }
    void		setScaler(Scaler*);		//!< becomes mine

    IOPar&		auxPars()			{ return auxpars_; }
    uiRetVal		prepareWork(const SeisTrc&); //!< optional
    uiRetVal		put(const SeisTrc&);
    size_type		nrWritten() const		{ return nrwritten_; }

    void		setCrFrom( const char* str )	{ crfrom_ = str; }
    void		setCrUserInfo( const char* str ) { crusrinfo_ = str; }

    void		fillPar(IOPar&) const;
    void		usePar(const IOPar&);

    GeomID		lastGeomID() const;
    GeomID		geomID(const SeisTrc&) const;

    SeisTrcTranslator*	translator()			{ return trl_; }
    Seis2DLinePutter*	linePutter()			{ return lineputter_; }
    SeisPSWriter*	psWriter()			{ return pswriter_; }

protected:

    bool		prepared_		= false;
    bool		is2dps_			= false;
    size_type		nrwritten_		= 0;
    StatsCollector&	statscollector_;
    TrcKey&		prevtrcky_;
    IOPar		auxpars_;
    BufferString	crfrom_;
    BufferString	crusrinfo_;
    GeomID		fixedgeomid_;

    IOObj*		ioobj_			= 0;
    Scaler*		scaler_			= 0;
    SeisTrc*		worktrc_		= 0;
    SeisTrcTranslator*	trl_			= 0;
    const SeisPSIOProvider* psioprov_		= 0;
    SeisPSWriter*	pswriter_		= 0;
    Seis2DLinePutter*	lineputter_		= 0;
    Seis2DDataSet*	dataset2d_		= 0;
    PosInfo::Line2DData* linedata_		= 0;

    void		setIOObj(IOObj*);
    void		startWork();
    bool		writeCollectedLineGeometry(uiRetVal&) const;
    void		writeCollectedStats() const;
    bool		isMultiConn() const;
    void		setPrevTrcKey(const SeisTrc&);

    // 3D only
    Conn*		crConn(int,bool,uiRetVal&);
    bool		ensureRightConn(const SeisTrc&,bool,uiRetVal&);
    bool		start3DWrite(Conn*,const SeisTrc&,uiRetVal&);
    bool		putCubeTrc(const SeisTrc&,uiRetVal&);

    // 2D only
    bool		selectLine(const SeisTrc&,uiRetVal&);
    bool		putLineTrc(const SeisTrc&,uiRetVal&);
    bool		putPSTrc(const SeisTrc&,uiRetVal&);

};


/*! stores traces that are submitted in random order in the correct sequence. */


mExpClass(Seis) SequentialStorer
{ mODTextTranslationClass(Seis::SequentialStorer);
public:
			SequentialStorer(Storer&,int buffsize=-1);
			    /*!< Storer is owned by caller, not mine.
				 Default bufsize is 2xnrprocessors. */
			~SequentialStorer();

    bool		announceTrace(const BinID&);
			    /*!< Tell the writer that this position will be
				 submitted later. */
    bool		announceTrace(Pos::GeomID,Pos::TraceNr_Type);
    bool		announceTrace(const TrcKey&);

    bool		submitTrace(SeisTrc*,bool waitforbuffer=true);
			    /*!< Trc becomes mine. If waitforbuffer and buffer
				 is full, wait until buffer gets smaller. */

    bool		finishWrite();
			    /*!< Wait for everything to be written. Should be
				 after final submitTrace, before closure.*/

    const uiRetVal&	result() const { return uirv_; }

protected:

    bool		iterateBuffer(bool waitforbuffer);
    void		reportWrite(const uiRetVal&);

    Storer&		storer_;
    TypeSet<BinID>	announcedtraces_;
    Threads::ConditionVar& lock_;
    ObjectSet<SeisTrc>	outputs_;
    const int		maxqueuesize_;

    BinID		latestbid_;

    int			queueid_;
    uiRetVal		uirv_;

    friend class	SingleTraceStorer;

};

} // namespace Seis
