/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        J.C. Glas
 Date:          February 2009
________________________________________________________________________

-*/
static const char* rcsID mUnusedVar = "$Id: command.cc,v 1.1 2012-09-17 12:37:41 cvsjaap Exp $";

#include "command.h"

#include "cmddriverbasics.h"
#include "cmddriver.h"

#include "canvascommands.h"
#include "drivercommands.h"
#include "inputcommands.h"
#include "listcommands.h"
#include "menubutcommands.h"
#include "qtcommands.h"
#include "tablecommands.h"
#include "treecommands.h"

#include "uimenu.h"

namespace CmdDrive
{

#define mGetCmdClass( key, cmdclass, cmddrv ) \
\
     if ( mMatchCI(key,cmdclass::keyWord()) ) \
	return new cmdclass( cmddrv );

Command* Command::factory( const char* key, CmdDriver& cmddrv )
{
    mGetCmdClass( key, AssignCmd,	cmddrv );
    mGetCmdClass( key, BreakCmd,	cmddrv );
    mGetCmdClass( key, ButtonCmd,	cmddrv );
    mGetCmdClass( key, ButtonMenuCmd,	cmddrv );
    mGetCmdClass( key, CallCmd,		cmddrv );
    mGetCmdClass( key, CanvasMenuCmd,	cmddrv );
    mGetCmdClass( key, CancelCmd,	cmddrv );
    mGetCmdClass( key, CaseCmd,		cmddrv );
    mGetCmdClass( key, CloseCmd,	cmddrv );
    mGetCmdClass( key, ColorOkCmd,	cmddrv );
    mGetCmdClass( key, ComboCmd,	cmddrv );
    mGetCmdClass( key, CommentCmd,	cmddrv );
    mGetCmdClass( key, ContinueCmd,	cmddrv );
    mGetCmdClass( key, DefCmd,		cmddrv );
    mGetCmdClass( key, DoCmd,		cmddrv );
    mGetCmdClass( key, DoWhileCmd,	cmddrv );
    mGetCmdClass( key, ElseCmd,		cmddrv );
    mGetCmdClass( key, ElseIfCmd,	cmddrv );
    mGetCmdClass( key, EndCmd,		cmddrv );
    mGetCmdClass( key, FedCmd,		cmddrv );
    mGetCmdClass( key, FiCmd,		cmddrv );
    mGetCmdClass( key, FileOkCmd,	cmddrv );
    mGetCmdClass( key, ForCmd,		cmddrv );
    mGetCmdClass( key, GreyOutsCmd,	cmddrv );
    mGetCmdClass( key, GuideCmd,	cmddrv );
    mGetCmdClass( key, IfCmd,		cmddrv );
    mGetCmdClass( key, InputCmd,	cmddrv );
    mGetCmdClass( key, ListButtonCmd,	cmddrv );
    mGetCmdClass( key, ListClickCmd,	cmddrv );
    mGetCmdClass( key, ListMenuCmd,	cmddrv );
    mGetCmdClass( key, ListSelectCmd,	cmddrv );
    mGetCmdClass( key, LogModeCmd,	cmddrv );
    mGetCmdClass( key, MenuCmd,		cmddrv );
    mGetCmdClass( key, OdCmd,		cmddrv );
    mGetCmdClass( key, OdUntilCmd,	cmddrv );
    mGetCmdClass( key, OkCmd,		cmddrv );
    mGetCmdClass( key, OnErrorCmd,	cmddrv );
    mGetCmdClass( key, OnOffCheckCmd,	cmddrv );
    mGetCmdClass( key, PauseCmd,	cmddrv );
    mGetCmdClass( key, ReturnCmd,	cmddrv );
    mGetCmdClass( key, RofCmd,		cmddrv );
    mGetCmdClass( key, ShowCmd,		cmddrv );
    mGetCmdClass( key, SleepCmd,	cmddrv );
    mGetCmdClass( key, SliderCmd,	cmddrv );
    mGetCmdClass( key, SnapshotCmd,	cmddrv );
    mGetCmdClass( key, SpinCmd,		cmddrv );
    mGetCmdClass( key, TabCmd,		cmddrv );
    mGetCmdClass( key, TableClickCmd,	cmddrv );
    mGetCmdClass( key, TableExecCmd,	cmddrv );
    mGetCmdClass( key, TableFillCmd,	cmddrv );
    mGetCmdClass( key, TableMenuCmd,	cmddrv );
    mGetCmdClass( key, TableSelectCmd,	cmddrv );
    mGetCmdClass( key, TreeButtonCmd,	cmddrv );
    mGetCmdClass( key, TreeClickCmd,	cmddrv );
    mGetCmdClass( key, TreeExpandCmd,	cmddrv );
    mGetCmdClass( key, TreeMenuCmd,	cmddrv );
    mGetCmdClass( key, TryCmd,		cmddrv );
    mGetCmdClass( key, WaitCmd,		cmddrv );
    mGetCmdClass( key, WinAssertCmd,	cmddrv );
    mGetCmdClass( key, WindowCmd,	cmddrv );
//    mGetCmdClass( key, WheelCmd,	cmddrv );

    mGetCmdClass( key, GetButtonCmd,		cmddrv );
    mGetCmdClass( key, IsButtonOnCmd,		cmddrv );
    mGetCmdClass( key, GetButtonMenuItemCmd,	cmddrv );
    mGetCmdClass( key, IsButtonMenuItemOnCmd,	cmddrv );
    mGetCmdClass( key, NrButtonMenuItemsCmd,	cmddrv );
    mGetCmdClass( key, GetCanvasMenuItemCmd,	cmddrv );
    mGetCmdClass( key, IsCanvasMenuItemOnCmd,	cmddrv );
    mGetCmdClass( key, NrCanvasMenuItemsCmd,	cmddrv );
    mGetCmdClass( key, CurComboItemCmd,		cmddrv );
    mGetCmdClass( key, GetComboItemCmd,		cmddrv );
    mGetCmdClass( key, IsComboItemOnCmd,	cmddrv );
    mGetCmdClass( key, NrComboItemsCmd,		cmddrv );
    mGetCmdClass( key, GetInputCmd,		cmddrv );

    mGetCmdClass( key, IsListButtonOnCmd,	cmddrv );
    mGetCmdClass( key, CurListItemCmd,		cmddrv );
    mGetCmdClass( key, GetListItemCmd,		cmddrv );
    mGetCmdClass( key, IsListItemOnCmd,		cmddrv );
    mGetCmdClass( key, NrListItemsCmd,		cmddrv );
    mGetCmdClass( key, GetListMenuItemCmd,	cmddrv );
    mGetCmdClass( key, IsListMenuItemOnCmd,	cmddrv );
    mGetCmdClass( key, NrListMenuItemsCmd,	cmddrv );

    mGetCmdClass( key, IsMatchCmd,		cmddrv );
    mGetCmdClass( key, GetMenuItemCmd,		cmddrv );
    mGetCmdClass( key, IsMenuItemOnCmd,		cmddrv );
    mGetCmdClass( key, NrMenuItemsCmd,		cmddrv );
    mGetCmdClass( key, IsShownCmd,		cmddrv );
    mGetCmdClass( key, GetSliderCmd,		cmddrv );
    mGetCmdClass( key, GetSpinCmd,		cmddrv );
    mGetCmdClass( key, CurTabCmd,		cmddrv );
    mGetCmdClass( key, GetTabCmd,		cmddrv );
    mGetCmdClass( key, IsTabOnCmd,		cmddrv );
    mGetCmdClass( key, NrTabsCmd,		cmddrv );

    mGetCmdClass( key, CurTableColCmd,		cmddrv );
    mGetCmdClass( key, CurTableItemCmd,		cmddrv );
    mGetCmdClass( key, CurTableRowCmd,		cmddrv );
    mGetCmdClass( key, GetTableColCmd,		cmddrv );
    mGetCmdClass( key, GetTableItemCmd,		cmddrv );
    mGetCmdClass( key, GetTableRowCmd,		cmddrv );
    mGetCmdClass( key, IsTableItemOnCmd,	cmddrv );
    mGetCmdClass( key, NrTableColsCmd,		cmddrv );
    mGetCmdClass( key, NrTableRowsCmd,		cmddrv );
    mGetCmdClass( key, GetTableMenuItemCmd,	cmddrv );
    mGetCmdClass( key, IsTableMenuItemOnCmd,	cmddrv );
    mGetCmdClass( key, NrTableMenuItemsCmd,	cmddrv );

    mGetCmdClass( key, CurTreeColCmd,		cmddrv );
    mGetCmdClass( key, CurTreeItemCmd,		cmddrv );
    mGetCmdClass( key, CurTreePathCmd,		cmddrv );
    mGetCmdClass( key, GetTreeColCmd,		cmddrv );
    mGetCmdClass( key, GetTreeItemCmd,		cmddrv );
    mGetCmdClass( key, GetTreePathCmd,		cmddrv );
    mGetCmdClass( key, IsTreeItemOnCmd,		cmddrv );
    mGetCmdClass( key, IsTreeItemExpandedCmd,	cmddrv );
    mGetCmdClass( key, NrTreeColsCmd,		cmddrv );
    mGetCmdClass( key, NrTreeItemsCmd,		cmddrv );
    mGetCmdClass( key, IsTreeButtonOnCmd,	cmddrv );
    mGetCmdClass( key, GetTreeMenuItemCmd,	cmddrv );
    mGetCmdClass( key, IsTreeMenuItemOnCmd,	cmddrv );
    mGetCmdClass( key, NrTreeMenuItemsCmd,	cmddrv );

//    mGetCmdClass( key, GetWheelCmd,		cmddrv );
    mGetCmdClass( key, IsWindowCmd,		cmddrv );
    return 0;
}


bool Command::isQuestionName( const char* name, CmdDriver& cmddrv )
{
    PtrMan<Command> cmd = factory( name, cmddrv );
    if ( !cmd )
	return false;

    return SearchKey("Is*",false).isMatching(name) ||
	   SearchKey("Nr*",false).isMatching(name) ||
	   SearchKey("Cur*",false).isMatching(name) ||
	   SearchKey("Get*",false).isMatching(name);
}


uiMainWin* Command::applWin()
{ return drv_.applWin(); }

const char* Command::outputDir() const
{ return drv_.outputDir(); }

bool Command::switchCurWin( uiMainWin* uimw )	
{ return drv_.switchCurWin( uimw ); }

const uiMainWin* Command::curWin() const
{ return drv_.curWin(); }

bool Command::openQDlg() const
{ return drv_.openQDlg(); }

CmdDriver::OnErrorTag Command::onError() const
{ return drv_.onError(); }

void Command::setOnError( CmdDriver::OnErrorTag tag )
{ drv_.setOnError( tag ); }

bool Command::verifyWinAssert( const char* newwinstr )
{ return drv_.verifyWinAssert( newwinstr ); }

bool Command::verifyWinState( const char* newwinstr, WinStateType newwinstate )
{ return drv_.verifyWinState( newwinstr, newwinstate ); }

void Command::setRecoveryStep( CmdDriver::RecoveryTag tag )
{ drv_.setRecoveryStep( tag ); }

void Command::setCaseSensitive( bool yn )
{ drv_.setCaseSensitive( yn ); }

bool Command::isCaseSensitive() const
{ return drv_.isCaseSensitive(); }

void Command::skipGreyOuts( bool yn )
{ drv_.skipGreyOuts( yn ); }

bool Command::greyOutsSkipped() const
{ return drv_.greyOutsSkipped(); }

bool Command::goingToChangeUiObj() const
{ return drv_.goingToChangeUiObj(); }

void Command::setSleep( float time, bool regular )
{ return drv_.setSleep( time, regular ); }

void Command::setWait( float time, bool regular )
{ return drv_.setWait( time, regular ); }

const uiObject* Command::localSearchEnv() const
{ return drv_.localSearchEnv(); }

bool Command::doLocalAction( uiObject* localenv, const char* actstr )
{ return drv_.doLocalAction( localenv, actstr ); }

bool Command::tryAction( const char* identname, const char* actstr )
{ return drv_.tryAction( identname, actstr ); }

bool Command::prepareActivate(Activator* activator)
{ return drv_.prepareActivate( activator ); }

void Command::finishActivate()
{ return drv_.finishActivate(); }

void Command::waitForClearance()
{ drv_.waitForClearance(); }

void Command::prepareIntercept( const FileMultiString& menupath, int onoff,
				CmdDriver::InterceptMode mode )
{ drv_.prepareIntercept( menupath, onoff, mode ); }

bool Command::didInterceptSucceed( const char* objnm )
{ return drv_.didInterceptSucceed( objnm ); }

const MenuInfo& Command::interceptedMenuInfo() const
{ return drv_.interceptedMenuInfo(); }

void Command::interact( const InteractSpec* ispec )
{ drv_.interact( ispec ); }

WildcardManager& Command::wildcardMan()
{ return drv_.wildcardMan(); }

IdentifierManager& Command::identifierMan()
{ return drv_.identifierMan(); }

ExprInterpreter& Command::exprInterpreter()
{ return drv_.exprInterpreter(); }

void Command::end()
{ drv_.end(); }

void Command::jump( int extralines )
{ drv_.jump( extralines ); }

int Command::lastActionIdxMove() const
{ return drv_.lastActionIdxMove(); }

int Command::curActionIdx() const
{ return drv_.curActionIdx(); }

bool Command::insertProcedure( int defidx )
{ return drv_.insertProcedure( defidx ); }


//====== Menu tracer ==========================================================


bool MenuTracer::greyOutsSkipped() const
{ return drv_.greyOutsSkipped(); }


bool MenuTracer::goingToChangeUiObj() const
{ return drv_.goingToChangeUiObj(); }


bool MenuTracer::findItem( const FileMultiString& menupath,
			   const uiMenuItem*& curitem, int* curitmidx ) const
{
    const uiMenuItemContainer* curmenu = &startmenu_;
    curitem = 0;
    FileMultiString pathstr;
    bool disabledpath = false;
    for ( int level=0; level<mSepStrSize(menupath); level++ )
    {
	BufferString itemstr( menupath[level] );
	mParDisambiguatorRet("menu item name", itemstr, itmselnr, false);
	if ( level > 0 )
	{
	    mDynamicCastGet( const uiPopupItem*, popupitm, curitem );
	    curmenu = popupitm ? &popupitm->menu() : 0;
	}
	if ( !curmenu || !curmenu->nrItems() )
	{
	    mWinErrStrm << "No subitems in menu"
		    << (level>0 ? " item \"" : "") << pathstr.unescapedStr()
		    << (level>0 ? "\"" : "") << std::endl;
	    return false;
	}

	ObjectSet<uiMenuItem> items( curmenu->items() );
	int nrgrey = 0;
	for ( int itmidx=items.size()-1; itmidx>=0; itmidx-- )
	{
	    mGetAmpFilteredStr( itmtxt, items[itmidx]->text() );
	    if ( mSearchKey(itemstr).isMatching(itmtxt) )
	    {
		if ( !items[itmidx]->isEnabled() )
		{
		    nrgrey++;
		    if ( greyOutsSkipped() )
			items.remove( itmidx );
		}
	    }
	    else
		items.remove( itmidx );
	}
	FileMultiString tmpstr( pathstr ); tmpstr += itemstr;
	mParStrPreRet( "menu item", items, nrgrey, tmpstr.unescapedStr(),
		       itmselnr, "path", true, false );
	curitem = items[0];
	mGetAmpFilteredStr( curitmtxt, curitem->text() );
	drv_.wildcardMan().check( mSearchKey(itemstr), curitmtxt );
	mDressNameString( curitmtxt, sMenuPath );
	pathstr += curitmtxt;
	disabledpath = disabledpath || !curitem->isEnabled();
    }

    mDynamicCastGet( const uiPopupItem*, popupitm, curitem );
    if ( popupitm && goingToChangeUiObj() )
    {
	mWinErrStrm << "No subitem specified for menu \"" << pathstr
		    << "\"" << std::endl;
	return false;
    }
    mDisabilityCheck( "menu item", 1, disabledpath );

    if ( curitmidx )
    {
	ObjectSet<uiMenuItem> items( curmenu->items() );
	*curitmidx = 0;
	for ( int idx=items.size()-1; idx>=0; idx-- )
	{
	    if ( greyOutsSkipped() && !items[idx]->isEnabled() )
		items.remove( idx );
	}
	*curitmidx = items.indexOf( curitem );
    }
    return true;
}


int MenuTracer::nrItems( const FileMultiString& menupath ) const
{
    ObjectSet<uiMenuItem> items;

    if ( menupath.isEmpty() )
	items = startmenu_.items();
    else
    {
	const uiMenuItem* mnuitm;
	if ( !findItem(menupath, mnuitm) )
	    return -1;

	mDynamicCastGet( const uiPopupItem*, popupitm, mnuitm );
	if ( popupitm )
	    items = popupitm->menu().items();
    }

    int nritems = 0;
    for ( int idx=0; idx<items.size(); idx++ )
    {
	if ( items[idx]->isEnabled() || !greyOutsSkipped() )
	    nritems++;
    }

    return nritems;
}


bool MenuTracer::getMenuInfo( const FileMultiString& menupath, bool allowroot,
			      MenuInfo& menuinfo ) const
{
    menuinfo.siblingnr_ = 0; 
    menuinfo.ison_ = -1; 
    menuinfo.text_.setEmpty(); 
    menuinfo.nrchildren_ = nrItems( menupath );

    if ( menuinfo.nrchildren_ < 0 )
	return false;

    if ( allowroot && menupath.isEmpty() )
	return true;

    const uiMenuItem* mnuitm;
    if ( !findItem(menupath, mnuitm, &menuinfo.siblingnr_) )
	return false;

    menuinfo.ison_ = !mnuitm->isCheckable() ? -1 : mnuitm->isChecked() ? 1 : 0;
    menuinfo.siblingnr_++;
    menuinfo.text_ = mnuitm->text();
    StringProcessor(menuinfo.text_).filterAmpersands();
    return true;
}


}; // namespace CmdDrive
