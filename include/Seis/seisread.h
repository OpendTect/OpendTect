#ifndef seisread_h
#define seisread_h

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	A.H. Bril
 Date:		27-1-98
 RCS:		$Id: seisread.h,v 1.10 2003-11-07 12:21:52 bert Exp $
________________________________________________________________________

-*/

#include <seisstor.h>
class BinIDRange;


/*!\brief reads from a seismic data store.

If you don't want all of the stored data, you must set use the
SeisTrcTranslator facilities (SeisTrcSel and ComponentData) after executing
the starter. If you don't execute the starter, the reader will do that but
you cannot use SeisTrcTranslator facilities then.

Then, the routine is: get(trc.info()) possibly followed by get(trc). Not keeping
this sequence is at your own risk.

*/

class SeisTrcReader : public SeisStorage
{

    friend class	SeisReadStarter;

public:

			SeisTrcReader(const IOObj* =0);
    Executor*		starter();

    int			get(SeisTrcInfo&);
			/*!< -1 = Error. errMsg() will return a message.
			      0 = End
			      1 = Usable info
			      2 = Not usable (skipped the trace)
			      If get(SeisTrc) is not called, get(SeisTrcInfo)
			      should automatically skip over the trace data
			      if necessary. */
			
    bool		get(SeisTrc&);
			/*!< You should call this function only if
			     get(trc.info()) returned 1. If you don't,
			     the trace selections may be ignored. */

    void		forceFloatData( bool yn=true )	{ forcefloats = yn; }
    void		fillPar(IOPar&) const;

protected:

    bool		foundvalidinl, foundvalidcrl;
    bool		new_packet, needskip;
    bool		forcefloats;
    bool		started;
    int			prev_inl;
    BinIDRange*		outer;

    void		init();
    Conn*		openFirst();
    bool		initRead(Conn*);
    void		trySkipConns();
    int			nextConn(SeisTrcInfo&);
    bool		doStart();

    bool		binidInConn(int) const;
    bool		multiConn() const;

};


#endif
