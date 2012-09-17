#ifndef seiswrite_h
#define seiswrite_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	A.H. Bril
 Date:		27-1-98
 RCS:		$Id: seiswrite.h,v 1.34 2012/02/21 15:12:23 cvsbert Exp $
________________________________________________________________________

-*/


/*!\brief writes to a seismic data store.

Before first usage, the prepareWork() will be called if you don't
do that separately. If you want to do some component and range selections
using the SeisTrcTranslator (3D only), you must do that before prepareWork
is done.

*/

#include "seisstor.h"
#include "fixedstring.h"
#include "linekey.h"
#include "posinfo2d.h"
#include "thread.h"
class SeisTrc;
class SeisPSWriter;
class Seis2DLinePutter;


mClass SeisTrcWriter : public SeisStoreAccess
{
public:

			SeisTrcWriter(const IOObj*,
				      const LineKeyProvider* r=0);
				//!< Write to real user entries from '.omf' file
				//!< Can be anything: SEGY - CBVS - database
			SeisTrcWriter(const char*,bool is_2d,bool is_ps);
				//!< Write 'loose' CBVS files
				//!< (or pre-stack: directories) only.
			~SeisTrcWriter();
    virtual bool	close();

    bool		prepareWork(const SeisTrc&);
    virtual bool	put(const SeisTrc&);
    int			nrWritten() const		{ return nrwritten_; }

    bool		isMultiComp() const;
    bool		isMultiConn() const;

    void		writeBluntly( bool yn=true )	{ makewrready_ = !yn; }

    Seis2DLinePutter*	linePutter()			{ return putter_; }
    const Seis2DLinePutter* linePutter() const		{ return putter_; }

    SeisPSWriter*	psWriter()			{ return pswriter_; }
    const SeisPSWriter*	psWriter() const		{ return pswriter_; }

    			// 2D
    const LineKeyProvider* lineKeyProvider() const	{ return lkp_; }
    void		setLineKeyProvider( const LineKeyProvider* l )
							{ lkp_ = l; }
				//!< If no lineKeyProvider set,
				//!< seldata's linekey will be used
    void		setAttrib( const char* a )	{ attribnm_ = a; }
				//!< if set, overrules attrib in linekey
    IOPar&		lineAuxPars()			{ return lineauxiopar_;}
    void 		setDataType( const char* dt ) 	{ datatype_ = dt; } 

    static const char*	sKeyWriteBluntly();
    virtual void	usePar(const IOPar&);
    virtual void	fillPar(IOPar&) const;

protected:

    bool		prepared_;
    int			nrtrcs_;
    int			nrwritten_;
    SeisTrc&		worktrc_;
    bool		makewrready_;
    int			firstns_;
    SamplingData<float>	firstsampling_;

    void		init();
    void		startWork();

    // PS only
    SeisPSWriter*	pswriter_;

    // 3D only
    Conn*		crConn(int,bool);
    bool		ensureRightConn(const SeisTrc&,bool);
    bool		start3DWrite(Conn*,const SeisTrc&);

    // 2D only
    BufferString	attribnm_;
    Seis2DLinePutter*	putter_;
    PosInfo::Line2DData	geom_;
    IOPar&		lineauxiopar_;
    LineKey		prevlk_;
    const LineKeyProvider* lkp_;
    BufferString	datatype_;
    bool		next2DLine();
    bool		put2D(const SeisTrc&);

};


/*!Writes traces that are submitted in random order in the correct sequence. */



mClass SeisSequentialWriter
{
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

    const char*		errMsg() const { return errmsg_.str(); }

protected:

    bool			iterateBuffer(bool waitforbuffer);
    void			reportWrite(const char*);
    friend class		SeisSequentialWriterTask;

    SeisTrcWriter*		writer_;
    TypeSet<BinID>		announcedtraces_;
    Threads::ConditionVar	lock_;
    ObjectSet<SeisTrc>		outputs_;
    const int			maxbuffersize_;

    BinID			latestbid_;

    int				queueid_;
    BufferString		errmsg_;
};
    			

#endif
