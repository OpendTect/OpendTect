#ifndef streamconn_H
#define streamconn_H

/*+
________________________________________________________________________

 CopyRight:	(C) de Groot-Bril Earth Sciences B.V.
 Author:	A.H.Bril
 Date:		21-10-1995
 RCS:		$Id: streamconn.h,v 1.4 2003-10-15 15:15:53 bert Exp $
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
			StreamConn(StreamData&);
				//!< MY stream: Input StreamData will be zero-ed
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

    istream&		iStream() const
    				{ return *const_cast<istream*>(sd.istrm); }
    ostream&		oStream() const
    				{ return *const_cast<ostream*>(sd.ostrm); }
    FILE*		fp() const
    				{ return const_cast<FILE*>(sd.fp); }
    StreamData&		streamData() const
				{ return *const_cast<StreamData*>(&sd); }

    virtual State	state() const		{ return state_; }
    virtual bool	bad() const;

    virtual int		nrRetries() const	{ return nrretries; }
    virtual int		retryDelay() const	{ return retrydelay; }
    void		setNrRetries( int n )	{ nrretries = n; }
    void		setRetryDelay( int n )	{ retrydelay = n; }
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
    int			nrretries;
    int			retrydelay;

};


#endif
