#ifndef seisstor_h
#define seisstor_h

/*@+
________________________________________________________________________

 CopyRight:	(C) de Groot-Bril Earth Sciences B.V.
 Author:	A.H. Bril
 Date:		20-1-98
 RCS:		$Id: seisstor.h,v 1.3 2000-03-02 15:25:33 bert Exp $
________________________________________________________________________

Trace storage objects handle seismic data storage.

@$*/


#include <conn.h>
#include <seisinfo.h>
class IOObj;
class Conn;
class Executor;
class SeisTrc;
class SeisTrcTranslator;
class StorageLayout;


class SeisStorage
{
public:

    virtual		~SeisStorage();

    Conn::State		connState() const
			{ return conn ? conn->state() : Conn::Bad; }
    const SeisPacketInfo& packetInfo() const
			{ return spi; }
    const char*		errMsg() const
			{ return errmsg; }
    const SeisTrcTranslator* translator() const
			{ return trl; }
    const IOObj*	ioObj() const
			{ return ioobj; }

    virtual Executor*	starter()		= 0;
    virtual bool	prepareRetry()		= 0;
    void		close();
    virtual void	usePar(const IOPar&);
				// After usePar(), check connState()
				// Do you need starter() executor?
    virtual void	fillPar(IOPar&) const;

protected:

			SeisStorage(const IOObj*);
    virtual void	init()			= 0;

    SeisPacketInfo	spi;
    IOObj*		ioobj;
    Conn*		conn;
    SeisTrcTranslator*	trl;
    const char*		errmsg;

    const StorageLayout& storageLayout() const;

private:

    void		open(const IOObj*);

};


#endif
