#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Bert
 Date:		17-5-1995
 Contents:	Generalized stream opener.
________________________________________________________________________

-*/

#include "basicmod.h"
#include "strmdata.h"
#include "bufstring.h"
#include "oscommand.h"
#include "datapack.h"
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

    const char*	fileName() const	{ return fname_.buf(); }
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

    void	addPathIfNecessary(const char*);
		//!< adds given path if stored filename is relative

    bool	isFile() const		{ return !mc_; }
    bool	isCommand() const	{ return mc_; }

    static const char*	sStdIO();
    static const char*	sStdErr();

    // Pre-load interface
    static bool		isPreLoaded(const char*,bool isid);
			    //!< If isid, a single hit will return true
    static bool		preLoad(const char*,TaskRunner&,const char* id);
			    //!< id can be anything, usually DBKey though
    static bool		preLoad(const BufferStringSet&,TaskRunner&,
				const char* id);
			    //!< id can be anything, usually DBKey though
    static void		getPreLoadedIDs(BufferStringSet&);
    static void		getPreLoadedFileNames(const char* id,BufferStringSet&);
			    //!< pass null id for all files
    static BufferString getPreLoadedDataPackID(const char*);
    static void		unLoad(const char*,bool isid=false);
			    //!< If isid, unload all with this id
    static void		unLoadAll();

protected:

    BufferString	fname_;
    OS::MachineCommand*	mc_ = nullptr;
    BufferString	workingdir_;

    static StreamData	makePLIStream(int);

    static void		sendCBMsg(const CallBack*,const char*);

private:
			StreamProvider(const StreamProvider&)	= delete;
    StreamProvider&	operator=(const StreamProvider&)	= delete;

public:

    mDeprecated void set(const char* inp);

};
