#ifndef streamconn_h
#define streamconn_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	A.H.Bril
 Date:		21-10-1995
 RCS:		$Id$
________________________________________________________________________

-*/


#include "generalmod.h"
#include "conn.h"
#include "od_iosfwd.h"


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

    virtual bool	bad() const;
    virtual bool	forRead() const;
    virtual bool	forWrite() const;
    virtual StreamConn*	getStream()		{ return this; }
    virtual void	close();

    void		setStream(od_istream*); //!< strm becomes all mine
    void		setStream(od_ostream*); //!< strm becomes all mine
    void		setStream(od_istream&); //!< strm remains all yours
    void		setStream(od_ostream&); //!< strm remains all yours

    od_stream&		odStream();
    od_istream&		iStream();
    od_ostream&		oStream();
    void		setFileName(const char*,bool forread);
    const char*		fileName() const;

    const char*		connType() const	{ return sType(); }
    static const char*	sType();


private:

    od_stream*		strm_;
    bool		mine_;

};


#endif

