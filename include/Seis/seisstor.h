#ifndef seisstor_h
#define seisstor_h

/*+
________________________________________________________________________

 CopyRight:	(C) de Groot-Bril Earth Sciences B.V.
 Author:	A.H. Bril
 Date:		20-1-98
 RCS:		$Id: seisstor.h,v 1.6 2001-06-18 13:56:37 bert Exp $
________________________________________________________________________

Trace storage objects handle seismic data storage.

-*/


#include <conn.h>
#include <seisinfo.h>
#include <storlayout.h>
class IOObj;
class Conn;
class Executor;
class SeisTrc;
class SeisTrcTranslator;
class SeisTrcSel;


/*!\brief base class for seis reader and writer. */

class SeisStorage
{
public:

    virtual		~SeisStorage();
    void		close();

    Conn::State		connState() const
			{ return conn ? conn->state() : Conn::Bad; }
    const char*		errMsg() const
			{ return errmsg; }
    SeisTrcTranslator*	translator()
			{ return trl; }
    const SeisTrcTranslator* translator() const
			{ return trl; }
    int			tracesHandled() const
			{ return nrtrcs; }

    const IOObj*	ioObj() const
			{ return ioobj; }
    void		setIOObj(const IOObj*);
    const SeisTrcSel*	trcSel() const
			{ return trcsel; }
    void		setTrcSel(SeisTrcSel*);
			//!< This hands over mem management
    int			selectedComponent() const
			{ return selcomp; }
			//!< default = -1 is all components
    void		setSelectedComponent( int i )
			{ selcomp = i; }

    virtual void	usePar(const IOPar&);
				// After usePar(), check connState()
    virtual void	fillPar(IOPar&) const;

    static const char*	sNrTrcs;

protected:

			SeisStorage(const IOObj*);
    virtual void	init()			{}
    void		cleanUp(bool alsoioobj=true);

    IOObj*		ioobj;
    Conn*		conn;
    int			nrtrcs;
    int			selcomp;
    SeisTrcTranslator*	trl;
    SeisTrcSel*		trcsel;
    BufferString	errmsg;

};


#endif
