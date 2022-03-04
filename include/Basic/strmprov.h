#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	A.H.Bril
 Date:		17-5-1995
 Contents:	Generalized stream opener.
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

Examples:
 - foo.bar
	File foo.bar in current directory (whatever that may be!).
 - C:\\tmp\xx.txt
   /tmp/xx.txt

  Files on remote servers are not supported.

*/

mExpClass(Basic) StreamProvider
{
public:
		StreamProvider(const char* filenm=nullptr);
		StreamProvider(const OS::MachineCommand&,
			       const char* workdir);
		~StreamProvider();

    void	setFileName(const char*);
    void	setCommand(const OS::MachineCommand&,const char* workdir);

    bool	isBad() const;

    const char* fileName() const	{ return fname_.buf(); }
    bool	exists(bool forread) const;
    bool	remove(bool recursive=true) const;
    bool	setReadOnly(bool yn) const;
    bool	isReadOnly() const;
    bool	rename(const char*,const CallBack* cb=0);
		    //!< renames if file. if successful, does a set()
		    //!< The callback will be called with a const char* capsule

    static StreamData	createIStream(const char*,bool binary=true);
			/*!< keep binary==true also for text files unless you
			     know what you are doing. win32 thing only. */
    static StreamData	createOStream(const char*,bool binary=true,
				    bool inplaceedit=false);
			/*!< keep binary==true also for text files unless you
			     know what you are doing. win32 thing only. */
    static StreamData	createCmdIStream(const OS::MachineCommand&,
					 const char* workdir,
					 bool fromstderr=false);
    static StreamData	createCmdOStream(const OS::MachineCommand&,
					 const char* workdir);

    void	addPathIfNecessary(const char*);
		//!< adds given path if stored filename is relative

    bool	isFile() const		{ return !mc_; }
    bool	isCommand() const	{ return mc_; }

    static const char*	sStdIO();
    static const char*	sStdErr();

    mDeprecatedDef void set(const char* inp);

    mDeprecated ("Use StreamProvider::createIStream or createCmdIStream")
    StreamData	makeIStream(bool binary=true,bool dummyarg=true) const;

    mDeprecated ("Use StreamProvider::createOStream or createCmdOStream")
    StreamData	makeOStream(bool binary=true,bool inplaceedit=false) const;

protected:

    BufferString	fname_;
    OS::MachineCommand* mc_ = nullptr;
    BufferString	workingdir_;

    static void		sendCBMsg(const CallBack*,const char*);

private:
			StreamProvider(const StreamProvider&)	= delete;
    StreamProvider&	operator=(const StreamProvider&)	= delete;


};

