#ifndef conn_h
#define conn_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	A.H.Bril
 Date:		21-10-1995
 Contents:	Connections with data providers (Streams, databases)
 RCS:		$Id$
________________________________________________________________________

-*/


#include "generalmod.h"
#include "gendefs.h"
class IOObj;
class StreamConn;

/*!
\brief Data connection.
  
  Data can be found in files and data stores. To access these data sources,
  some kind of connection must be set up. This class defines a simple
  interface common to these connections.
*/

mExpClass(General) Conn
{
public:

			Conn() : ioobj(0)	{}
    virtual		~Conn()			{}
    virtual const char*	connType() const	= 0;

    virtual bool	bad() const		= 0;
    virtual bool	forRead() const		= 0;
    virtual bool	forWrite() const	{ return !forRead(); }
    virtual void	close()			{}
    virtual StreamConn*	getStream()		{ return 0; }
    inline bool		isStream() const
    			{ return const_cast<Conn*>(this)->getStream(); }

    inline Conn*	conn()			{ return gtConn(); }
    inline const Conn*	conn() const		{ return gtConn(); }
			//!< Returns the actual connection doing the work

    const IOObj*	ioobj;
			//!< Some objects require this IOObj
			//!< It is normally the IOObj that created the Conn

    			// to fill 'forread' variables
    static const bool	Read;	// true
    static const bool	Write;	// false

protected:

    virtual Conn*	gtConn() const	{ return const_cast<Conn*>(this); }

};


/*!
\brief Connection implemented in terms of another Conn object.
*/

mExpClass(General) XConn  : public Conn
{

    friend class	IOX;

public:

			XConn() : conn_(0), mine_(true)	{}
			~XConn()	{ if ( mine_ ) delete conn_; }

    virtual bool	bad() const
			{ return conn_ ? conn_->bad() : true; }
    virtual bool	forRead() const	
    			{ return conn_ && conn_->forRead(); }
    virtual bool	forWrite() const
    			{ return conn_ && conn_->forWrite(); }
    virtual void	close()
    			{ if ( conn_ ) conn_->close(); }
    virtual StreamConn*	getStream()
			{ return conn_ ? conn_->getStream() : 0; }

    void		setConn( Conn* c, bool becomesmine=true )
			{ if ( mine_ ) delete conn_;
			  conn_ = c; mine_ = becomesmine; }

    const char*		connType() const	{ return sType(); }
    static const char*	sType();

protected:

    Conn*		conn_;
    bool		mine_;

    Conn*		gtConn() const	{ return const_cast<Conn*>(conn_); }

};


#endif

