#ifndef streamconn_H
#define streamconn_H

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	A.H.Bril
 Date:		21-10-1995
 RCS:		$Id: streamconn.h,v 1.8 2008-01-08 11:53:52 cvsbert Exp $
________________________________________________________________________

-*/


#include "conn.h"
#include "strmdata.h"


/*!\brief Connection with an underlying iostream. */

class StreamConn : public Conn
{
public:

    enum Type		{ File, Device, Command };
			DeclareEnumUtils(Type)

			StreamConn();
			StreamConn(StreamData&);
				//!< MY stream: Input StreamData will be zero-ed
			StreamConn(std::istream*);
				//!< MY stream: I will delete on destruct
			StreamConn(std::ostream*);
				//!< MY stream: I will delete on destruct
			StreamConn(const char*,State);
				//!< MY stream: I will delete on destruct
			StreamConn(std::istream&,bool close_on_delete=false);
				//!< YOUR stream: I may close only
			StreamConn(std::ostream&,bool close_on_delete=false);
				//!< YOUR stream: I may close only

    virtual		~StreamConn();

    std::istream&	iStream() const
    				{ return *const_cast<std::istream*>(sd.istrm); }
    std::ostream&	oStream() const
    				{ return *const_cast<std::ostream*>(sd.ostrm); }
    FILE*		fp() const
    				{ return const_cast<FILE*>(sd.fp); }
    StreamData&		streamData() const
				{ return *const_cast<StreamData*>(&sd); }

    virtual State	state() const		{ return state_; }
    virtual bool	bad() const;

    bool		doIO(void*,unsigned int nrbytes);
    void		clearErr();

    void		close();

    const char*		fileName() const	{ return sd.fileName(); }
    void		setFileName( const char* s ) { sd.setFileName(s); }

    const char*		connType() const	{ return sType; }
    bool		isStream() const	{ return true; }
    static const char*	sType;

private:

    StreamData		sd;
    State		state_;
    bool		mine;
    bool		closeondel;

};


#endif
