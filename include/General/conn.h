#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "generalmod.h"
#include "multiid.h"
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

    virtual		~Conn()			{}
    virtual void	close()			{}

    virtual bool	isBad() const		= 0;
    virtual const char* creationMessage() const { return 0; }

    virtual const char* connType() const	= 0;
    virtual bool	forRead() const		= 0;
    virtual bool	forWrite() const	{ return !forRead(); }

    virtual StreamConn* getStream()		{ return 0; }
    inline bool		isStream() const
			{ return const_cast<Conn*>(this)->getStream(); }

    inline Conn*	conn()			{ return gtConn(); }
    inline const Conn*	conn() const		{ return gtConn(); }
			//!< Returns the actual connection doing the work

    const MultiID&	linkedTo() const	{ return ioobjid_; }
    void		setLinkedTo( const MultiID& id ) { ioobjid_ = id; }

			// to fill 'forread' variables
    static const bool	Read;	// true
    static const bool	Write;	// false

protected:

			Conn()			{}

    MultiID		ioobjid_;

    virtual Conn*	gtConn() const	{ return const_cast<Conn*>(this); }

};


/*!
\brief Connection implemented in terms of another Conn object.
*/

mExpClass(General) XConn  : public Conn
{

    friend class	IOX;

public:

			XConn() : conn_(0), mine_(true) {}
			~XConn()	{ if ( mine_ ) delete conn_; }

    bool		isBad() const override
			{ return conn_ ? conn_->isBad() : true; }
    const char*		creationMessage() const override
			{ return conn_ ? conn_->creationMessage() : 0; }
    bool		forRead() const override
			{ return conn_ && conn_->forRead(); }
    bool		forWrite() const override
			{ return conn_ && conn_->forWrite(); }
    void		close() override
			{ if ( conn_ ) conn_->close(); }
    StreamConn*		getStream() override
			{ return conn_ ? conn_->getStream() : 0; }

    void		setConn( Conn* c, bool becomesmine=true )
			{ if ( mine_ ) delete conn_;
			  conn_ = c; mine_ = becomesmine; }

    const char*		connType() const override	{ return sType(); }
    static const char*	sType();

protected:

    Conn*		conn_;
    bool		mine_;

    Conn*		gtConn() const override
			{ return const_cast<Conn*>(conn_); }

};
