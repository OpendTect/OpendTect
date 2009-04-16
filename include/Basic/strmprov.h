#ifndef strmprov_h
#define strmprov_h

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	A.H.Bril
 Date:		17-5-1995
 Contents:	Generalized stream opener.
 RCS:		$Id: strmprov.h,v 1.34 2009-04-16 10:05:25 cvsranojay Exp $
________________________________________________________________________

-*/
 
#include "streamconn.h"
class CallBack;
class TaskRunner;
class BufferStringSet;


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

mClass StreamProvider
{
public:
		StreamProvider(const char* nm=0);
		StreamProvider(const char*,const char*,StreamConn::Type);
    void	set(const char*);
    bool	rename(const char*,const CallBack* cb=0);
    		//!< renames if file. if successful, does a set()

    bool	skipFiles(int) const;
		//!< Skips files if tape
    bool	rewind() const;
		//!< Rewinds if tape
    bool	offline() const;
		//!< Checks whether tape is offline
    bool	bad() const				{ return isbad_; }

    bool	exists(int forread) const;
    bool	remove(bool recursive=true) const;
    bool	setReadOnly(bool yn) const;
    bool	isReadOnly() const;

    StreamData	makeOStream(bool binary=true) const;
		/*!< On win32, binary mode differs from text mode. 
		    Use binary=false when explicitly reading txt files. */
    StreamData	makeIStream(bool binary=true,bool allowpreloaded=true) const;
		//!< see makeOStream remark
    bool	executeCommand(bool inbg=false,bool inconsole=false) const;
    		//!< If type is Command, execute command without opening pipe
    		//!< 'inbg' will execute in background if remote
    void	mkBatchCmd(BufferString& comm) const;

    const char*	fullName() const;
    const char*	hostName() const		{ return hostname_.buf(); }
    const char*	fileName() const		{ return fname_.buf(); }
    const char*	command() const			{ return fname_.buf(); }
    long	blockSize() const		{ return blocksize_; }

    void	setHostName( const char* hname ) { hostname_ = hname; }
    void	setFileName( const char* fn )	{ fname_ = fn; }
    void	setCommand( const char* fn )	{ fname_ = fn; }
    void	setBlockSize( long bs )		{ blocksize_ = bs; }
    void	addPathIfNecessary(const char*);
		//!< adds given path if stored filename is relative
    void	setRemExec( const char* s )		{ rshcomm_ = s; }

    StreamConn::Type	type()				{ return type_; }
    bool		isNormalFile() const;

    static const char*	sStdIO();
    static const char*	sStdErr();

    static bool		isPreLoaded(const char*,bool isid);
			    //!< If isid, a single hit will return true
    static bool		preLoad(const char*,TaskRunner&,const char* id);
    			    //!< id can be anything, usually MultiID though
    static bool		preLoad(const BufferStringSet&,TaskRunner&,
	    			const char* id);
    			    //!< id can be anything, usually MultiID though
    static void		getPreLoadedIDs(BufferStringSet&);
    static void		getPreLoadedFileNames(const char* id,BufferStringSet&);
			    //!< pass null id for all files
    static int		getPreLoadedDataPackID(const char*);
    static void		unLoad(const char*,bool isid=false);
			    //!< If isid, unload all with this id
    static void		unLoadAll();

protected:

    BufferString	fname_;
    BufferString	hostname_;
    BufferString	rshcomm_;

    long		blocksize_;
    bool		isbad_;
    StreamConn::Type	type_;

    void		mkOSCmd(bool) const;
    static StreamData	makePLIStream(int);

    static void	sendCBMsg(const CallBack*,const char*);
    		//!< The callback will be called with a const char* capsule

};

//! Execute command in OS
mGlobal bool ExecOSCmd(const char*,bool inbg=false);

//! Create Execute command
mGlobal const char* GetExecCommand(const char* prognm,const char* filenm);

//! Execute command
mGlobal bool ExecuteScriptCommand(const char* prognm,const char* filenm);

#endif
