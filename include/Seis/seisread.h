#ifndef seisread_h
#define seisread_h

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	A.H. Bril
 Date:		27-1-98
 RCS:		$Id: seisread.h,v 1.12 2004-08-25 12:27:06 bert Exp $
________________________________________________________________________

-*/

#include "seisstor.h"
class Executor;
class BinIDRange;
class SeisTrcBuf;


/*!\brief reads from a seismic data store.

If you don't want all of the stored data, you must set use the
SeisTrcTranslator facilities (SeisSelData and ComponentData) after calling
prepareWork(). If you don't call prepareWork(), the reader will do that but
you cannot use SeisTrcTranslator facilities then.

Then, the routine is: get(trc.info()) possibly followed by get(trc). Not keeping
this sequence is at your own risk.

*/

class SeisTrcReader : public SeisStoreAccess
{
public:

			SeisTrcReader(const IOObj* =0);
			~SeisTrcReader();

    void		forceFloatData( bool yn=true )	{ forcefloats = yn; }
    			//!< Only effective if called before prepareWork()
    bool		prepareWork();
    			//!< After this, you can set stuff on the translator

    int			get(SeisTrcInfo&);
			/*!< -1 = Error. errMsg() will return a message.
			      0 = End
			      1 = Usable info
			      2 = Not usable (skipped the trace)
			      If get(SeisTrc) is not called, get(SeisTrcInfo)
			      will automatically skip over the trace data
			      if necessary. */
			
    bool		get(SeisTrc&);
			/*!< You should call this function only if
			     get(trc.info()) returned 1. If you don't,
			     the trace selections may be ignored. */

    void		fillPar(IOPar&) const;

    bool		isPrepared() const		{ return prepared; }
    int			curLineNumber() const		{ return curlinenr; }
    							//!< 2D only

protected:

    bool		foundvalidinl, foundvalidcrl;
    bool		new_packet, needskip;
    bool		forcefloats;
    bool		prepared;
    bool		inforead;
    int			prev_inl;
    int			curlinenr;
    BinIDRange*		outer;
    SeisTrcBuf*		tbuf;
    Executor*		fetcher;
    BufferString	linename;

    void		init();
    Conn*		openFirst();
    bool		initRead(Conn*);
    void		trySkipConns();
    int			nextConn(SeisTrcInfo&);
    bool		doStart();

    bool		binidInConn(int) const;
    bool		isMultiConn() const;
    void		startWork();

    bool		get2D(SeisTrcInfo&);
    bool		get2D(SeisTrc&);
    bool		mkNextFetcher();
    bool		readNext2D();

};


#endif
