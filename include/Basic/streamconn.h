#ifndef streamconn_H
#define streamconn_H

/*+
________________________________________________________________________

 CopyRight:	(C) de Groot-Bril Earth Sciences B.V.
 Author:	A.H.Bril
 Date:		21-10-1995
 RCS:		$Id: streamconn.h,v 1.1 2001-08-31 16:38:18 bert Exp $
________________________________________________________________________

-*/


#include <conn.h>
#include <strmdata.h>


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


#endif
