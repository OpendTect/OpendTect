#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "generalmod.h"
#include "conn.h"
#include "od_iosfwd.h"
#include "bufstring.h"


/*!
\brief Connection with an underlying iostream.
*/

mExpClass(General) StreamConn : public Conn
{
public:
			StreamConn();
			StreamConn(od_istream*); //!< strm becomes all mine
			StreamConn(od_ostream*); //!< strm becomes all mine
			StreamConn(od_istream&); //!< strm remains all yours
			StreamConn(od_ostream&); //!< strm remains all yours
			StreamConn(const char* fnm,bool forread);
    virtual		~StreamConn();

    bool		isBad() const override;
    const char*		creationMessage() const override
			{ return creationmsg_; }
    bool		forRead() const override;
    bool		forWrite() const override;
    StreamConn*		getStream() override		{ return this; }
    void		close() override;

    void		setStream(od_istream*); //!< strm becomes all mine
    void		setStream(od_ostream*); //!< strm becomes all mine
    void		setStream(od_istream&); //!< strm remains all yours
    void		setStream(od_ostream&); //!< strm remains all yours

    od_stream&		odStream();
    od_istream&		iStream();
    od_ostream&		oStream();
    void		setFileName(const char*,bool forread);
    const char*		fileName() const;

    const char*		connType() const override	{ return sType(); }
    static const char*	sType();


private:

    od_stream*		strm_;
    bool		mine_;

    BufferString	creationmsg_;

    void		fillCrMsg(od_stream*);

};
