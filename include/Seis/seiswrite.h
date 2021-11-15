#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	A.H. Bril
 Date:		27-1-98
________________________________________________________________________

-*/


/*!\brief writes to a seismic data store.

Before first usage, the prepareWork() will be called if you don't
do that separately. If you want to do some component and range selections
using the SeisTrcTranslator (3D only), you must do that before prepareWork
is done.

The auxPars() can be use for line or cube pars that may be of interest. For 2D,
there can be aux pars for the line, for 3D it is used to give a hint for the
output cube's setup and extent.

*/

#include "seismod.h"
#include "seisstor.h"
#include "fixedstring.h"
#include "linekey.h"
#include "uistring.h"
class SeisTrc;
class SeisPSWriter;
class Seis2DLinePutter;
namespace Threads { class ConditionVar; }
namespace PosInfo { class Line2DData; }


mExpClass(Seis) SeisTrcWriter : public SeisStoreAccess
{ mODTextTranslationClass(SeisTrcWriter)
public:

			SeisTrcWriter(const MultiID&,Seis::GeomType,
				      const GeomIDProvider* =nullptr);
				//!< Write to real user entries from '.omf' file
				//!< Can be anything: SEGY - CBVS - database
			SeisTrcWriter(const IOObj&,
				      const Seis::GeomType* =nullptr,
				      const GeomIDProvider* =nullptr);
				//!< Write to real user entries from '.omf' file
				//!< Can be anything: SEGY - CBVS - database
			SeisTrcWriter(const IOObj&,Pos::GeomID,
				      const Seis::GeomType* =nullptr,
				      const GeomIDProvider* =nullptr);
				//!< Restricted to a given Pos::GeomID
			SeisTrcWriter(const SeisStoreAccess::Setup&);
			~SeisTrcWriter();
    virtual bool	close();

    bool		prepareWork(const SeisTrc&);
    virtual bool	put(const SeisTrc&);
    int			nrWritten() const		{ return nrwritten_; }

    bool		isMultiComp() const;
    bool		isMultiConn() const;
    IOPar&		auxPars()			{ return auxpars_; }

    Seis2DLinePutter*	linePutter()			{ return putter_; }
    const Seis2DLinePutter* linePutter() const		{ return putter_; }

    SeisPSWriter*	psWriter()			{ return pswriter_; }
    const SeisPSWriter*	psWriter() const		{ return pswriter_; }

			// 2D only
    const GeomIDProvider* geomIDProvider() const	{ return gidp_; }
    Pos::GeomID		geomID() const override;
    void		setGeomIDProvider(const GeomIDProvider*);
    void		setSelData(Seis::SelData*);
				//!< If no GeomIDProvider set,
				//!< seldata's GeomID will be used
    void		setAttrib( const char* a )	{ attribnm_ = a; }
				//!< if set, overrules attrib in linekey
    void		setDataType( const char* dt )	{ datatype_ = dt; }

    void		setCrFrom( const char* str )	{ crfrom_ = str; }
    void		setCrUserInfo( const char* str ) { crusrinfo_ = str; }
    void		setComponentNames(const BufferStringSet&);

protected:

    int			nrtrcs_ = 0;
    int			nrwritten_ = 0;
    SeisTrc&		worktrc_;
    int			firstns_ = mUdf(int);
    SamplingData<float>	firstsampling_;
    IOPar&		auxpars_;

    void		startWork();

    // PS only
    SeisPSWriter*	pswriter_ = nullptr;

    // 3D only
    Conn*		crConn(int,bool);
    bool		ensureRightConn(const SeisTrc&,bool);
    bool		start3DWrite(Conn*,const SeisTrc&);

    // 2D only
    BufferString	attribnm_;
    Seis2DLinePutter*	putter_ = nullptr;
    PosInfo::Line2DData* linedata_ = nullptr;
    TypeSet<float>	spnrs_;
    Pos::GeomID		prevgeomid_;
    const GeomIDProvider* gidp_ = nullptr;
    BufferString	datatype_;
    void		updateLineData();
    bool		next2DLine();
    bool		put2D(const SeisTrc&);

    BufferString	crfrom_;
    BufferString	crusrinfo_;
    BufferStringSet	compnames_;

			SeisTrcWriter(const MultiID&)		= delete;

public:
    mDeprecated("Provide an existing IOObj")
			SeisTrcWriter(const IOObj*,
				      const GeomIDProvider* =nullptr);

    mDeprecated("Make a temporary IOObj")
			SeisTrcWriter(const char* fnm,bool is_2d,bool is_ps);
				//!< Write 'loose' CBVS files
				//!< (or prestack: directories) only.

};


/*!Writes traces that are submitted in random order in the correct sequence. */



mExpClass(Seis) SeisSequentialWriter
{ mODTextTranslationClass(SeisSequentialWriter);
public:
			SeisSequentialWriter( SeisTrcWriter*, int buffsize=-1 );
			/*!<Writer is owned by caller, not mine. Default bufsize
			    is 2xnrprocessors. */
			~SeisSequentialWriter();
    bool		announceTrace(const BinID&);
			/*!<Tell the writer that this binid will be submitted
			    later. */

    bool		submitTrace( SeisTrc*, bool waitforbuffer=true );
			/*!<Trc becomes mine. If waitforbuffer is true and
			    buffer is full, wait until buffer gets smaller. */

    bool		submitGather( ObjectSet<SeisTrc>&,
				      bool waitforbuffer=true );
			/*!<Trcs become mine. All traces are assumed to be from
			    the same cdp. Traces will be written out in the same
			    order as in the set.
			    If waitforbuffer is true and buffer is full, wait
			    until buffer gets smaller. */

    bool		finishWrite();
			/*!<Wait for everything to be written. Should be
			    after final submitTrace, before closure.*/

    uiString		errMsg() const { return errmsg_; }

protected:

    bool			iterateBuffer(bool waitforbuffer);
    void			reportWrite(const char*);
    friend class		SeisSequentialWriterTask;

    SeisTrcWriter*		writer_;
    TypeSet<BinID>		announcedtraces_;
    Threads::ConditionVar&	lock_;
    ObjectSet<SeisTrc>		outputs_;
    const int			maxbuffersize_;

    BinID			latestbid_;

    int				queueid_;
    uiString			errmsg_;

};


