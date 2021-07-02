/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Wayne Mogg
 Date:		April 2021
________________________________________________________________________

-*/
#include "commanddefs.h"
#include "file.h"
#include "plfdefs.h"
#include "ptrman.h"


class TerminalCommands : public CommandDefs
{ mODTextTranslationClass(TerminalCommands);
public:
    TerminalCommands( const BufferStringSet& paths )
    {
	if ( __iswin__ )
	{
	    addCmd( "wt.exe", tr("Windows Terminal"), "terminal-wt.png",
		    tr("Windows Terminal"), paths );
	    addCmd( "powershell.exe", tr("Power Shell"), "powershell.png",
		    tr("Power Shell"), paths );
	    addCmd( "cmd.exe", tr("Command Prompt"), "cmd.png",
		    tr("Command Prompt"), paths );
	}
	else
	{
	    addCmd( "konsole", tr("KDE Konsole"), "terminal.png",
		    tr("KDE Konsole"), paths );
	    addCmd( "gnome-terminal", tr("Gnome Terminal"), "terminal.png",
		    tr("Gnome Terminal"), paths );
	    addCmd( "terminator", tr("Terminator"), "terminal.png",
		    tr("Terminator"), paths );
	    addCmd( "quake", tr("Quake"), "terminal.png", tr("Quake"), paths );
	    addCmd( "yakuake", tr("Yakuake"), "terminal.png", tr("Yakuake"),
		    paths);
	    addCmd( "tilda", tr("Tilda"), "terminal.png", tr("Tilda"), paths );
	    addCmd( "macterm", tr("MacTerm"), "terminal.png", tr("MacTerm"),
		    paths );
	    addCmd( "Terminal", tr("Terminal"), "terminal.png", tr("Terminal"),
		    paths );
	    addCmd( "xterm", tr("X Terminal"), "terminal.png", tr("X Terminal"),
		    paths );
	}
    }
};


CommandDefs::CommandDefs()
{}


CommandDefs::CommandDefs( const CommandDefs& oth )
    : BufferStringSet(oth)
{
    *this = oth;
}


bool CommandDefs::checkCommandExists( const char* command,
				      const BufferStringSet& paths )
{
    const bool usesyspath = paths.isEmpty();
    const BufferString tmp = File::findExecutable( command, paths, usesyspath );
    return !tmp.isEmpty();
}


const CommandDefs& CommandDefs::getTerminalCommands(
						const BufferStringSet& paths )
{
    mDefineStaticLocalObject( PtrMan<TerminalCommands>, term_cmds,
			      = new  TerminalCommands( paths ) )

    return *term_cmds;
}


CommandDefs& CommandDefs::operator=( const CommandDefs& oth )
{
    BufferStringSet::operator=( oth );
    uinames_ = oth.uinames_;
    iconnms_ = oth.iconnms_;
    tooltips_ = oth.tooltips_;
    return *this;
}


void CommandDefs::erase()
{
    BufferStringSet::erase();
    uinames_.setEmpty();
    iconnms_.setEmpty();
    tooltips_.setEmpty();
}


bool CommandDefs::addCmd( const char* command, const uiString& uinm,
			  const char* iconnm, const uiString& tooltip,
			  const BufferStringSet& paths )
{
    if ( !checkCommandExists(command, paths) )
	return false;

    add( command );
    uinames_.add( uinm );
    iconnms_.add( iconnm );
    tooltips_.add( tooltip );
    return true;
}


uiString CommandDefs::getUiName( int idx ) const
{
    return validIdx( idx ) ? uinames_.get( idx ) : uiString::empty();
}


BufferString CommandDefs::getIconName( int idx ) const
{
    return validIdx( idx ) ? iconnms_.get( idx ) : BufferString::empty();
}


uiString CommandDefs::getToolTip( int idx ) const
{
    return validIdx( idx ) ? tooltips_.get( idx ) : uiString::empty();
}


const uiStringSet& CommandDefs::getUiNames() const
{
    return uinames_;
}
