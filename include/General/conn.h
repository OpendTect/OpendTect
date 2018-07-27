#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Bert
 Date:		21-10-1995
 Contents:	Connections with data providers (Streams, databases)
________________________________________________________________________

-*/


#include "generalmod.h"
#include "dbkey.h"
class IOObj;
class StreamConn;

/*!
\brief Data connection.

  Data can be found in files and data stores. To access these data sources,
  some kind of connection must be set up. This class defines a simple
  interface common to these connections.
*/

#define mCloseRollBack true


mExpClass(General) Conn
{
public:

    virtual		~Conn()			{ close(); }
    virtual void	close(bool failed=false) {}
    inline void		rollback()		{ close( mCloseRollBack ); }

    virtual bool	isBad() const		= 0;
    virtual const char*	creationMessage() const	{ return 0; }

    virtual const char*	connType() const	= 0;
    virtual bool	forRead() const		= 0;
    virtual bool	forWrite() const	{ return !forRead(); }

    virtual StreamConn*	getStream()		{ return 0; }
    inline bool		isStream() const
			{ return const_cast<Conn*>(this)->getStream(); }

    inline Conn*	conn()			{ return gtConn(); }
    inline const Conn*	conn() const		{ return gtConn(); }
			//!< Returns the actual connection doing the work

    const DBKey&	linkedTo() const	{ return ioobjid_; }
    void		setLinkedTo( const DBKey& id ) { ioobjid_ = id; }

			// to fill 'forread' variables
    static const bool	Read;	// true
    static const bool	Write;	// false

protected:

			Conn()			{}

    DBKey		ioobjid_;

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

    virtual bool	isBad() const
			{ return conn_ ? conn_->isBad() : true; }
    virtual const char*	creationMessage() const
			{ return conn_ ? conn_->creationMessage() : 0; }
    virtual bool	forRead() const
			{ return conn_ && conn_->forRead(); }
    virtual bool	forWrite() const
			{ return conn_ && conn_->forWrite(); }
    virtual void	close( bool failed=false )
			{ if ( conn_ ) conn_->close(failed); }
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
