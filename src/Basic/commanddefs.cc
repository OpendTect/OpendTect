/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Wayne Mogg
 Date:		April 2021
________________________________________________________________________

-*/
#include "commanddefs.h"
#include "oddirs.h"
#include "file.h"
#include "filepath.h"
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
	    addCmd( "konsole", tr("KDE Konsole"), "terminal-kde.png",
		    tr("KDE Konsole"), paths );
	    addCmd( "gnome-terminal", tr("Gnome Terminal"),
		    "terminal-gnome.png", tr("Gnome Terminal"), paths );
	    addCmd( "terminator", tr("Terminator"), "terminal-terminator.png",
		    tr("Terminator"), paths );
	    addCmd( "quake", tr("Quake"), "terminal-quake.png",
			tr("Quake"), paths );
	    addCmd( "yakuake", tr("Yakuake"), "terminal-yakuake.png",
			tr("Yakuake"), paths);
	    addCmd( "tilda", tr("Tilda"), "terminal-tilda.png",
			tr("Tilda"), paths );
	    addCmd( "macterm", tr("MacTerm"), "terminal-mac.png",
			tr("MacTerm"), paths );
	    addCmd( "Terminal", tr("Terminal"), "terminal.png",
			tr("Terminal"), paths );
	    addCmd( "xterm", tr("X Terminal"), "terminal-xterm.png",
			tr("X Terminal"), paths );
	}
    }
};


CommandDefs::CommandDefs()
{
    progargs_.setNullAllowed();
}


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
    prognames_ = oth.prognames_;
    deepCopy( progargs_, oth.progargs_ );
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
    prognames_.setEmpty();
    deepErase( progargs_ );

}


bool CommandDefs::addCmd( const char* appnm, const uiString& uinm,
			  const char* iconnm, const uiString& tooltip,
			  const BufferStringSet& paths,
			  const BufferStringSet* cmdargs )
{
    BufferStringSet usedpaths( paths );
    addHints( usedpaths, appnm );
    if ( !checkCommandExists(appnm,usedpaths) )
	return false;

    addApplication( appnm, cmdargs );
    uinames_.add( uinm );
    iconnms_.add( iconnm );
    tooltips_.add( tooltip );
    return true;
}


void CommandDefs::addApplication( const char* appnm,
				  const BufferStringSet* cmdargs )
{
    add( appnm );
    BufferStringSet* args = cmdargs
			  ? new BufferStringSet( *cmdargs ) : nullptr;
    if ( __iswin__ )
    {
	prognames_.add( appnm );
	if ( FixedString(appnm).startsWith("cmd",CaseInsensitive) )
	{
	    if ( !args )
		args = new BufferStringSet();
	    args->add( "/D" ).add( "/K" );
	    const BufferString cmdstring(
		"prompt $COpendTect$F $P$G && title Command Prompt");
	    args->add( cmdstring );
	}
    }
    else if ( __ismac__ )
    {
	prognames_.add( "open" );
	if ( !args )
	    args = new BufferStringSet();
	args->add( "-a" ).add( appnm ).add( GetPersonalDir() );
    }
    else
	prognames_.add( appnm );

    progargs_.add( args );
}


const char* CommandDefs::program( int progidx ) const
{
    if ( !prognames_.validIdx(progidx) )
	return nullptr;

    return prognames_.get( progidx ).buf();
}


const BufferStringSet* CommandDefs::args( int argidx ) const
{
    if ( !progargs_.validIdx(argidx) )
	return nullptr;

    return progargs_.get( argidx );
}


void CommandDefs::addHints( BufferStringSet& usedpaths, const char* appnm )
{
    if ( __ismac__ )
    {
	BufferString macappnm( appnm );
	macappnm.add( ".app" );
	FilePath fp( "/Applications", "Utilities", macappnm, "Contents",
		     "MacOS" );
	usedpaths.add( fp.fullPath() );
	usedpaths.add( FilePath("/System", fp.fullPath()).fullPath() );
    }
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
