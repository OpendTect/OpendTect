#ifndef seisread_h
#define seisread_h

/*+
________________________________________________________________________

 CopyRight:	(C) de Groot-Bril Earth Sciences B.V.
 Author:	A.H. Bril
 Date:		27-1-98
 RCS:		$Id: seisread.h,v 1.6 2002-07-01 09:33:04 bert Exp $
________________________________________________________________________

-*/

#include <seisstor.h>


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

    bool		icfound;
    bool		new_packet;
    bool		needskip;
    bool		rdinited;
    bool		forcefloats;

    void		init();
    bool		openFirst();
    bool		initRead();
    void		trySkipConns();
    int			nextConn(SeisTrcInfo&);

    inline bool		validBidselRes( int r, bool clcrl ) const
			{ return r == 0 || (clcrl && r/256 != 2)
				        || (!clcrl && r%256 != 2); }
    bool		multiConn() const;

};


#endif
