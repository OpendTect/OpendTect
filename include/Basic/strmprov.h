#ifndef strmprov_H
#define strmprov_H

/*+
________________________________________________________________________

 CopyRight:	(C) de Groot-Bril Earth Sciences B.V.
 Author:	A.H.Bril
 Date:		17-5-1995
 Contents:	Generalized stream opener.
 RCS:		$Id: strmprov.h,v 1.9 2002-12-13 17:05:55 bert Exp $
________________________________________________________________________

-*/
 
#include <idobj.h>
#include <fixstring.h>
#include <streamconn.h>


/*!\brief provides I/O stream for disk or tape file or system command.

StreamProvider provides a stream with requested source attached:
 - starting with '@' --> OS command that produces the data on stdin/stdout
 - having a ':' --> remote machinename preceding (e.g. dgb1:/dev/exabyte)
 - tape devices are assumed to be on /dev
Thus:
 - dgb1:/dev/exabyte
	tape device on remote host dgb1
 - dgb1:@handle_data
	Executable handle_data on remote host dgb1 will get/put on stdin/stdout
 - dgb1:/foo/bar
	File /foo/bar on remote host dgb1
 - foo.bar
	file foo.bar in current directory

 A null string or StreamProvider::sStdIO will select std input and output.
*/

class StreamProvider : public IDObject
{			isConcreteClass
public:
		StreamProvider(const char* nm=0);
		StreamProvider(const char*,const char*,StreamConn::Type);
    void	set(const char*);

    bool	skipFiles(int) const;
		//!< Skips files if tape
    bool	rewind() const;
		//!< Rewinds if tape
    bool	offline() const;
		//!< Checks whether tape is offline
    bool	bad() const				{ return isbad; }

    bool	exists(int forread) const;
    bool	remove(bool recursive=true) const;

    StreamData	makeOStream(bool inbg=false) const;
		//!< 'inbg' will execute in background if remote
    StreamData	makeIStream(bool inbg=false) const;
		//!< see makeOStream remark
    bool	executeCommand(bool inbg=false) const;
    		//!< If type is Command, execute command without opening pipe

    const char*	fullName() const;
    const char*	hostName() const			{ return hostname; }
    const char*	fileName() const			{ return fname; }
    const char*	command() const				{ return fname; }
    int		blockSize() const			{ return blocksize; }

    void	setHostName( const char* hname )	{ hostname = hname; }
    void	setFileName( const char* fn )		{ fname = fn; }
    void	setCommand( const char* fn )		{ fname = fn; }
    void	setBlockSize( long bs )			{ blocksize = bs; }
    void	addPathIfNecessary(const char*);
		//!< adds given path if stored filename is relative
    void	setRemExec( const char* s )		{ rshcomm = s; }

    StreamConn::Type	type()				{ return type_; }
    bool		isNormalFile() const;

    static const char*	sStdIO;
    static const char*	sStdErr;

protected:

    FixedString<256>	fname;
    FixedString<32>	hostname;
    FixedString<16>	rshcomm;

    long		blocksize;
    bool		isbad;
    StreamConn::Type	type_;

    void		mkOSCmd(bool,bool) const;

};

bool ExecOSCmd(const char*);


#endif
