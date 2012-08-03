#ifndef streamconn_h
#define streamconn_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	A.H.Bril
 Date:		21-10-1995
 RCS:		$Id: streamconn.h,v 1.16 2012-08-03 13:00:15 cvskris Exp $
________________________________________________________________________

-*/


#include "basicmod.h"
#include "conn.h"
#include "strmdata.h"


/*!\brief Connection with an underlying iostream. */

mClass(Basic) StreamConn : public Conn
{
public:
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

    const std::istream&	iStream() const		{ return *sd_.istrm; }
    std::istream&	iStream()		{ return *sd_.istrm; }
    const std::ostream&	oStream() const		{ return *sd_.ostrm; }
    std::ostream&	oStream()		{ return *sd_.ostrm; }
    const StreamData&	streamData() const	{ return sd_; }
    StreamData&		streamData()		{ return sd_; }
    FILE*		fp() const		{ return sd_.filePtr(); }

    virtual State	state() const		{ return state_; }
    virtual bool	bad() const;
    void		clearErr();
    bool		doIO(void*,unsigned int nrbytes);
    void		close();

    const char*		fileName() const	{ return sd_.fileName(); }
    void		setFileName( const char* s ) { sd_.setFileName(s); }

    const char*		connType() const	{ return sType(); }
    bool		isStream() const	{ return true; }

    static const char*	sType()			{ return "Stream"; }	


private:

    StreamData		sd_;
    State		state_;
    bool		mine_;
    bool		closeondel_;

};


#endif

