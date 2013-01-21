#ifndef strmprov_h
#define strmprov_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	A.H.Bril
 Date:		17-5-1995
 Contents:	Generalized stream opener.
 RCS:		$Id$
________________________________________________________________________

-*/
 
#include "basicmod.h"
#include "strmdata.h"
#include "bufstring.h"
class CallBack;
class TaskRunner;
class BufferStringSet;


/*!
\ingroup Basic
\brief Provides I/O stream for file or system command.

  StreamProvider provides a stream with requested source attached:
   - starting with '@' --> OS command that produces the data on stdin/stdout
   - Hostname may preceed before a ':' (UNIX variants) or '\\' (Windows).
  
  Thus:
   - dgb1:@handle_data
        Executable handle_data on remote host dgb1 will get/put on stdin/stdout.   - \\winserv\foo\bar
	File \foo\bar on remote host winserv.
   - foo.bar
	File foo.bar in current directory.

  A null string or StreamProvider::sStdIO will select std input and output.
*/

mExpClass(Basic) StreamProvider
{
public:
		StreamProvider(const char* nm=0);
		StreamProvider(const char* hostnm,const char* fnm,bool iscomm);
    void	set(const char*);
    bool	rename(const char*,const CallBack* cb=0);
    		//!< renames if file. if successful, does a set()

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
    void	setRemExec( const char* s )	{ rshcomm_ = s; }

    bool	isCommand() const		{ return iscomm_; }
    bool	isNormalFile() const;

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

    BufferString		fname_;
    BufferString		hostname_;
    BufferString		rshcomm_;

    long			blocksize_;
    bool			isbad_;
    bool			iscomm_;

    void			mkOSCmd(bool,BufferString&) const;
    static StreamData		makePLIStream(int);

    static void	sendCBMsg(const CallBack*,const char*);
    		//!< The callback will be called with a const char* capsule

};

//! Execute command in OS
mGlobal(Basic) bool ExecOSCmd(const char*,bool inconsloe=false,bool inbg=false);

//! Execute command
mGlobal(Basic) bool ExecuteScriptCommand(const char* prognm,const char* filenm);

#endif

