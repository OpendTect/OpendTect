#ifndef conn_H
#define conn_H

/*+
________________________________________________________________________

 CopyRight:	(C) de Groot-Bril Earth Sciences B.V.
 Author:	A.H.Bril
 Date:		21-10-1995
 Contents:	Connections with data providers (Streams, databases)
 RCS:		$Id: conn.h,v 1.6 2001-06-07 09:42:38 bert Exp $
________________________________________________________________________

-*/


#include <defobj.h>
#include <enums.h>
#include <strmdata.h>
class IOObj;


/*!\brief Data connection.

Data can be found in files and data stores. To access these data sources,
some kind of connection must be set up. This class defines a simple
interface common to these connections.

*/

class Conn : public DefObject
{	     hasFactory(Conn)
public:

    enum State		{ Bad, Read, Write };

			Conn()	: ioobj(0)	{}
    virtual		~Conn()			{}

    virtual State	state() const		= 0;
    virtual bool	bad() const		{ return state() == Bad; }
    virtual bool	forRead() const		{ return state() == Read; }
    virtual bool	forWrite() const	{ return state() == Write; }
    virtual void	close()			{}

    virtual int		nrRetries() const	{ return 0; }
    virtual int		retryDelay() const	{ return 0; }


    inline Conn*	conn()			{ return gtConn(); }
    inline const Conn*	conn() const		{ return gtConn(); }
			//!< Returns the actual connection doing the work

    IOObj*		ioobj;

protected:

    virtual Conn*	gtConn() const	{ return const_cast<Conn*>(this); }

};


/*!\brief Connection with an underlying iostream. */

class StreamConn : public Conn
{		   isProducable(StreamConn)
public:

    enum Type		{ File, Device, Command };
			DeclareEnumUtils(Type)

			StreamConn();
			StreamConn(const StreamData&);
				//!< MY stream: this will delete on destruct
			StreamConn(istream*);
				//!< MY stream: this will delete on destruct
			StreamConn(ostream*);
				//!< MY stream: this will delete on destruct
			StreamConn(const char*,State);
				//!< MY stream: this will delete on destruct
			StreamConn(istream&,bool close_on_delete=false);
				//!< YOUR stream: this may close only
			StreamConn(ostream&,bool close_on_delete=false);
				//!< YOUR stream: this may close only

    virtual		~StreamConn();

    istream&		iStream() const  { return (istream&)*sd.istrm; }
    ostream&		oStream() const  { return (ostream&)*sd.ostrm; }
    FILE*		fp() const	 { return (FILE*)sd.fp; }

    virtual State	state() const		{ return state_; }
    virtual int		nrRetries() const	{ return nrretries; }
    virtual int		retryDelay() const	{ return retrydelay; }
    void		setNrRetries( int n )	{ nrretries = n; }
    void		setRetryDelay( int n )	{ retrydelay = n; }

    bool		doIO(void*,unsigned int nrbytes);
    void		clearErr();

    virtual bool	bad() const;
    const char*		name() const	 { return fname; }
    void		close();

private:

    StreamData		sd;
    State		state_;
    bool		mine;
    bool		closeondel;
    char*		fname;
    int			nrretries;
    int			retrydelay;

};


/*!\brief Connection implemented in terms of another Conn object. */

class XConn  : public Conn
{	       isProducable(XConn)

    friend class	IOX;

public:
			XConn()
			: conn_(0), mine_(true)		{}
			~XConn()
			{ if ( mine_ ) delete conn_; }

    virtual State	state() const
			{ return conn_ ? conn_->state() : Conn::Bad; }
    virtual int		nrRetries() const
			{ return conn_ ? conn_->nrRetries() : 0; }
    virtual int		retryDelay() const
			{ return conn_ ? conn_->retryDelay() : 0; }

    void		setConn( Conn* c, bool becomesmine=true )
			{ if ( mine_ ) delete conn_;
			  conn_ = c; mine_ = becomesmine; }
    void		close()
			{ if ( conn_ ) conn_->close(); }

protected:

    Conn*		conn_;
    bool		mine_;

    Conn*		gtConn() const	{ return const_cast<Conn*>(conn_); }

};


#endif
