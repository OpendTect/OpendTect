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
#include "oscommand.h"
class CallBack;
class TaskRunner;
class BufferStringSet;


/*!
\brief Provides I/O stream for file or system command.

To specify a command, start with '@'. Hostname may be added as in OSCommand.

Examples:
 - dgb1:@handle_data
   @dgb1:handle_data
   @dgb1:@handle_data
   @\\dgb1:handle_data
	Executable handle_data on remote host dgb1 that gets/puts data to/from
	stdin/stdout.
 - foo.bar
	File foo.bar in current directory (whatever that may be!).
 - C:\\tmp\xx.txt
   /tmp/xx.txt

  Files on remote servers are not supported.

*/

mExpClass(Basic) StreamProvider
{
public:
		StreamProvider(const char* nm=0);
    void	set(const char*);

    inline bool	isBad() const			{ return fname_.isEmpty(); }

    bool	exists(bool forread) const;
    bool	remove(bool recursive=true) const;
    bool	setReadOnly(bool yn) const;
    bool	isReadOnly() const;
    bool	rename(const char*,const CallBack* cb=0);
		    //!< renames if file. if successful, does a set()
		    //!< The callback will be called with a const char* capsule

    StreamData	makeOStream(bool binary=true,bool editmode=false) const;
		    /*!< On win32, binary mode differs from text mode.
			Use binary=false when explicitly reading txt files.
			Use editmode=true when want to edit/modify existing data
			in a file.*/
    StreamData	makeIStream(bool binary=true,bool allowpreloaded=true) const;
		    //!< see makeOStream remark

    const char*	fullName() const;
    const char*	fileName() const	{ return fname_.buf(); }
    const char*	command() const		{ return fileName(); }
    const char*	hostName() const	{ return hostname_.buf(); }

    void	setFileName( const char* fn );
    void	setCommand(const char* cmd, const char* hostnm=0);
    void	addPathIfNecessary(const char*);
		//!< adds given path if stored filename is relative

    bool	isFile() const			{ return !iscomm_; }
    bool	isCommand() const		{ return iscomm_; }

    static const char*	sStdIO();
    static const char*	sStdErr();

    // Pre-load interface
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
    bool		iscomm_;
    BufferString	hostname_;

    static StreamData	makePLIStream(int);
    void		mkOSCmd(BufferString&) const;

    static void		sendCBMsg(const CallBack*,const char*);

};



#endif
