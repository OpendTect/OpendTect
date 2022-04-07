/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Wayne Mogg
 Date:		June 2021
________________________________________________________________________

-*/
#include "commanddefs.h"
#include "file.h"
#include "filepath.h"
#include "hiddenparam.h"
#include "oddirs.h"
#include "plfdefs.h"
#include "ptrman.h"

static HiddenParam<CommandDefs,BufferStringSet*> hp_prognames_(nullptr);
static HiddenParam<CommandDefs,
		   ObjectSet<BufferStringSet>* > hp_progargs_(nullptr);

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
    hp_prognames_.setParam( this, new BufferStringSet );
    auto* progargs = new ObjectSet<BufferStringSet>;
    hp_progargs_.setParam( this, progargs );
    progargs->setNullAllowed();
}


CommandDefs::CommandDefs( const CommandDefs& oth )
    : BufferStringSet(oth)
{
    hp_prognames_.setParam( this, new BufferStringSet );
    hp_progargs_.setParam( this, new ObjectSet<BufferStringSet> );
    *this = oth;
}


CommandDefs::~CommandDefs()
{
    hp_prognames_.removeAndDeleteParam( this );
    hp_progargs_.removeAndDeleteParam( this );
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
    getProgNames() = oth.getProgNames();
    deepCopy( getProgArgs(), oth.getProgArgs() );
    return *this;
}

BufferStringSet& CommandDefs::getProgNames() const
{
    return *hp_prognames_.getParam( this );
}


ObjectSet<BufferStringSet>& CommandDefs::getProgArgs() const
{
    return *hp_progargs_.getParam( this );
}


void CommandDefs::erase()
{
    BufferStringSet::erase();
    getProgNames().setEmpty();
    deepErase( getProgArgs() );
    uinames_.setEmpty();
    iconnms_.setEmpty();
    tooltips_.setEmpty();
}


bool CommandDefs::addCmd( const char* appnm, const uiString& uinm,
			  const char* iconnm, const uiString& tooltip,
			  const BufferStringSet& paths )
{
    return addCmd( appnm, uinm, iconnm, tooltip, paths, nullptr );
}


bool CommandDefs::addCmd( const char* appnm, const uiString& uinm,
			  const char* iconnm, const uiString& tooltip,
			  const BufferStringSet& paths,
			  const BufferStringSet* cmdargs )
{
    BufferStringSet usedpaths( paths );
    addHints( usedpaths, appnm );
    if ( !checkCommandExists(appnm, usedpaths) )
	return false;

    addApplication( appnm, cmdargs );
    uinames_.add( uinm );
    iconnms_.add( iconnm );
    tooltips_.add( tooltip );
    return true;
}


void CommandDefs::addApplication( const char* appnm )
{
    return addApplication( appnm, nullptr );
}


void CommandDefs::addApplication( const char* appnm,
				  const BufferStringSet* cmdargs )
{
    BufferStringSet& prognames = getProgNames();

    add( appnm );
    BufferStringSet* args = cmdargs
			  ? new BufferStringSet( *cmdargs ) : nullptr;
    if ( __iswin__ )
    {
	prognames.add( appnm );
	if ( FixedString(appnm).startsWith("cmd",CaseInsensitive) )
	{
	    if ( !args )
		args = new BufferStringSet();
	    args->add( "/D" ).add( "/K" );
	    const BufferString cmdstring(
		    "prompt $COpendTect$F $P$G && title Command Prompt" );
	    args->add( cmdstring );
	}
    }
    else if ( __ismac__ )
    {
	prognames.add( "open" );
	if ( !args )
	    args = new BufferStringSet();
	args->add( "-a" ).add( appnm ).add( GetPersonalDir() );
    }
    else
	prognames.add( appnm );

    getProgArgs().add( args );
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


const char* CommandDefs::program( int progidx ) const
{
    if ( !getProgNames().validIdx(progidx) )
	return nullptr;

    return getProgNames().get( progidx ).buf();
}


const BufferStringSet* CommandDefs::args( int argidx ) const
{
    if ( !getProgArgs().validIdx(argidx) )
	return nullptr;

    return getProgArgs().get( argidx );
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
