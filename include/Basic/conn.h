#ifndef conn_H
#define conn_H

/*+
________________________________________________________________________

 CopyRight:	(C) de Groot-Bril Earth Sciences B.V.
 Author:	A.H.Bril
 Date:		21-10-1995
 Contents:	Connections with data providers (Streams, databases)
 RCS:		$Id: conn.h,v 1.1 2000-03-02 15:24:27 bert Exp $
________________________________________________________________________

-*/


#include <defobj.h>
#include <enums.h>
#include <strmdata.h>
class IOObj;


class Conn : public DefObject
{	     hasFactory(Conn)
public:
    enum State		{ Bad, Read, Write };
			DeclareEnumUtilsWithVar(State,state)

			Conn()	: state_(Bad), ioobj(0)	{}
    virtual		~Conn()				{}

    bool		forRead() const		{ return state_ == Read; }
    bool		forWrite() const	{ return state_ == Write; }

    virtual int		nrRetries()		{ return 0; }
    virtual int		retryDelay()		{ return 0; }

    virtual bool	bad() const		= 0;

    IOObj*		ioobj;

};


class StreamConn : public Conn
{		   isProducable(StreamConn)
public:
    enum Type		{ File, Device, Command };
			DeclareEnumUtils(Type)

			StreamConn();
			StreamConn(const StreamData&);

			StreamConn(istream*); // My mem man
			StreamConn(ostream*);
			StreamConn(const char*,State);
			StreamConn(istream&); // Your mem man
			StreamConn(ostream&);

    virtual		~StreamConn();

    istream&		iStream() const  { return (istream&)*sd.istrm; }
    ostream&		oStream() const  { return (ostream&)*sd.ostrm; }
    FILE*		fp() const	 { return (FILE*)sd.fp; }

    virtual int		nrRetries() const	{ return nrretries; }
    virtual int		retryDelay() const	{ return retrydelay; }
    void		setNrRetries( int n )	{ nrretries = n; }
    void		setRetryDelay( int n )	{ retrydelay = n; }

    bool		doIO(void*,unsigned int nrbytes);
    void		clearErr();

    virtual bool	bad() const;
    const char*		name() const	 { return fname; }

private:
    StreamData		sd;

    bool		mine;
    char*		fname;

    int			nrretries;
    int			retrydelay;

};


class XConn  : public Conn
{	       isProducable(XConn)
public:
			XConn() : conn(0)	{}
			~XConn()		{ delete conn; }
    virtual bool	bad() const		{ return conn ? NO : YES; }
    virtual int		nrRetries() const	{ return conn->nrRetries(); }
    virtual int		retryDelay() const	{ return conn->retryDelay(); }

protected:

    Conn*		conn;

};


#endif
