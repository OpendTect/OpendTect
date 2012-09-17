#ifndef conn_h
#define conn_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	A.H.Bril
 Date:		21-10-1995
 Contents:	Connections with data providers (Streams, databases)
 RCS:		$Id: conn.h,v 1.15 2009/07/22 16:01:13 cvsbert Exp $
________________________________________________________________________

-*/


#include "enums.h"
class IOObj;


/*!\brief Data connection.

Data can be found in files and data stores. To access these data sources,
some kind of connection must be set up. This class defines a simple
interface common to these connections.

*/

mClass Conn
{
public:

    enum State		{ Bad, Read, Write, RW };

			Conn()	: ioobj(0)	{}
    virtual		~Conn()			{}

    virtual State	state() const		= 0;
    virtual const char*	connType() const	= 0;
    virtual bool	bad() const		{ return state() == Bad; }
    virtual bool	forRead() const		{ return (int)state() % 2; }
    virtual bool	forWrite() const	{ return (int)state() >= Write;}
    virtual void	close()			{}
    virtual bool	isStream() const	{ return false; }

    inline Conn*	conn()			{ return gtConn(); }
    inline const Conn*	conn() const		{ return gtConn(); }
			//!< Returns the actual connection doing the work

    IOObj*		ioobj;
			//!< Some objects require this IOObj
			//!< It is normally the IOObj that created the Conn

protected:

    virtual Conn*	gtConn() const	{ return const_cast<Conn*>(this); }

};


/*!\brief Connection implemented in terms of another Conn object. */

mClass XConn  : public Conn
{

    friend class	IOX;

public:
			XConn()
			: conn_(0), mine_(true)		{}
			~XConn()
			{ if ( mine_ ) delete conn_; }

    virtual State	state() const
			{ return conn_ ? conn_->state() : Conn::Bad; }

    void		setConn( Conn* c, bool becomesmine=true )
			{ if ( mine_ ) delete conn_;
			  conn_ = c; mine_ = becomesmine; }
    void		close()
			{ if ( conn_ ) conn_->close(); }

    const char*		connType() const	{ return sType(); }
    static const char*	sType()			{ return "X-Group"; }

protected:

    Conn*		conn_;
    bool		mine_;

    Conn*		gtConn() const	{ return const_cast<Conn*>(conn_); }

};


#endif
