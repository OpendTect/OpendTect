#ifndef seisstor_h
#define seisstor_h

/*+
________________________________________________________________________

 CopyRight:	(C) de Groot-Bril Earth Sciences B.V.
 Author:	A.H. Bril
 Date:		20-1-98
 RCS:		$Id: seisstor.h,v 1.7 2003-02-18 16:32:21 bert Exp $
________________________________________________________________________

Trace storage objects handle seismic data storage.

-*/


#include <seisinfo.h>
class Conn;
class IOObj;
class Executor;
class SeisTrcSel;
class SeisTrcTranslator;


/*!\brief base class for seis reader and writer. */

class SeisStorage
{
public:

    virtual		~SeisStorage();
    void		close();

    Conn*		curConn();
    const Conn*		curConn() const;

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
				// Afterwards check whether curConn is still OK.
    virtual void	fillPar(IOPar&) const;

    static const char*	sNrTrcs;

protected:

			SeisStorage(const IOObj*);
    virtual void	init()			{}
    void		cleanUp(bool alsoioobj=true);

    IOObj*		ioobj;
    int			nrtrcs;
    int			selcomp;
    SeisTrcTranslator*	trl;
    SeisTrcSel*		trcsel;
    BufferString	errmsg;

};


#endif
