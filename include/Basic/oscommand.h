#ifndef oscommand_h
#define oscommand_h

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
#include "bufstring.h"


/*!\brief Can execute OS commands.

Executes command without opening pipe.
The default remote execution command is ssh.

 */

mExpClass(Basic) OSCommand
{
public:

			OSCommand(const char* comm=0,const char* hnm=0);

    inline const char*	command() const			{ return comm_; }
    inline void		setCommand( const char* cm )	{ comm_ = cm; }
    inline const char*	hostName() const		{ return hname_; }
    inline void		setHostName( const char* hnm )	{ hname_ = hnm; }
    inline const char*	remExec() const			{ return remexec_; }
    inline void		setRemExec( const char* sh )	{ remexec_ = sh; }

    inline bool		isBad() const		{ return comm_.isEmpty(); }

    bool		set(const char*,bool lookforhostname=true);
			//!< returns !isBad()
    const char*		get() const;

    bool		execute(bool inconsole=false,bool inbg=false) const;

    static const char*	defaultRemExec()	{ return defremexec_; }
    static void		setDefaultRemExec( const char* s ) { defremexec_ = s; }

    static const char*	extractHostName(const char*,BufferString&);
			//!< returns remaining part

protected:

    BufferString	comm_;
    BufferString	hname_;
    BufferString	remexec_;
    static BufferString	defremexec_;

    void		mkOSCmd(bool,BufferString&) const;
#ifdef __win__
    void		mkConsoleCmd(BufferString& comm) const;
#endif

};

//! Execute command on local host
mGlobal(Basic) bool ExecOSCmd(const char*,bool inconsole=false,bool inbg=false);

//! Execute od script
mGlobal(Basic) bool ExecODProgram(const char* prognm,const char* filenm);


#endif
