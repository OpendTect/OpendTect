/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        J.C. Glas
 Date:          February 2009
________________________________________________________________________

-*/
static const char* rcsID mUnusedVar = "$Id$";

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

mImplFactory1Param( Command, CmdDriver&, Command::factory );


BufferString Command::factoryKey( const char* name )
{
    BufferString fackey = name;
    StringProcessor(fackey).capitalize();
    return fackey;
}


BufferString Command::createFactoryKey( const char* keyword )
{
    const BufferString fackey = factoryKey( keyword );

    if ( factory().hasName( fackey ) )
    {
	BufferString errmsg( "Redefining command \"" );
	errmsg += keyword; errmsg += "\"";
	pFreeFnErrMsg( errmsg, "CmdDrive::Command" );
    }

    return fackey; 
}



void Command::initStandardCommands()
{
    static bool done = false;
    if ( done ) return; 
    done = true;

    AssignCmd::initClass();
    BreakCmd::initClass();
    ButtonCmd::initClass();
    ButtonMenuCmd::initClass();
    CallCmd::initClass();
    CanvasMenuCmd::initClass();
    CancelCmd::initClass();
    CaseCmd::initClass();
    CloseCmd::initClass();
    ColorOkCmd::initClass();
    ComboCmd::initClass();
    CommentCmd::initClass();
    ContinueCmd::initClass();
    DefCmd::initClass();
    DoCmd::initClass();
    DoWhileCmd::initClass();
    ElseCmd::initClass();
    ElseIfCmd::initClass();
    EndCmd::initClass();
    FedCmd::initClass();
    FiCmd::initClass();
    FileOkCmd::initClass();
    ForCmd::initClass();
    GreyOutsCmd::initClass();
    GuideCmd::initClass();
    IfCmd::initClass();
    InputCmd::initClass();
    ListButtonCmd::initClass();
    ListClickCmd::initClass();
    ListMenuCmd::initClass();
    ListSelectCmd::initClass();
    LogModeCmd::initClass();
    MenuCmd::initClass();
    OdCmd::initClass();
    OdUntilCmd::initClass();
    OkCmd::initClass();
    OnErrorCmd::initClass();
    OnOffCheckCmd::initClass();
    PauseCmd::initClass();
    ReturnCmd::initClass();
    RofCmd::initClass();
    ShowCmd::initClass();
    SleepCmd::initClass();
    SliderCmd::initClass();
    SnapshotCmd::initClass();
    SpinCmd::initClass();
    TabCmd::initClass();
    TableClickCmd::initClass();
    TableExecCmd::initClass();
    TableFillCmd::initClass();
    TableMenuCmd::initClass();
    TableSelectCmd::initClass();
    TreeButtonCmd::initClass();
    TreeClickCmd::initClass();
    TreeExpandCmd::initClass();
    TreeMenuCmd::initClass();
    TryCmd::initClass();
    WaitCmd::initClass();
    WinAssertCmd::initClass();
    WindowCmd::initClass();

    GetButtonCmd::initClass();
    IsButtonOnCmd::initClass();
    GetButtonMenuItemCmd::initClass();
    IsButtonMenuItemOnCmd::initClass();
    NrButtonMenuItemsCmd::initClass();
    GetCanvasMenuItemCmd::initClass();
    IsCanvasMenuItemOnCmd::initClass();
    NrCanvasMenuItemsCmd::initClass();
    CurComboItemCmd::initClass();
    GetComboItemCmd::initClass();
    IsComboItemOnCmd::initClass();
    NrComboItemsCmd::initClass();
    GetInputCmd::initClass();

    IsListButtonOnCmd::initClass();
    CurListItemCmd::initClass();
    GetListItemCmd::initClass();
    IsListItemOnCmd::initClass();
    NrListItemsCmd::initClass();
    GetListMenuItemCmd::initClass();
    IsListMenuItemOnCmd::initClass();
    NrListMenuItemsCmd::initClass();

    IsMatchCmd::initClass();
    GetMenuItemCmd::initClass();
    IsMenuItemOnCmd::initClass();
    NrMenuItemsCmd::initClass();
    IsShownCmd::initClass();
    GetSliderCmd::initClass();
    GetSpinCmd::initClass();
    CurTabCmd::initClass();
    GetTabCmd::initClass();
    IsTabOnCmd::initClass();
    NrTabsCmd::initClass();

    CurTableColCmd::initClass();
    CurTableItemCmd::initClass();
    CurTableRowCmd::initClass();
    GetTableColCmd::initClass();
    GetTableItemCmd::initClass();
    GetTableRowCmd::initClass();
    IsTableItemOnCmd::initClass();
    NrTableColsCmd::initClass();
    NrTableRowsCmd::initClass();
    GetTableMenuItemCmd::initClass();
    IsTableMenuItemOnCmd::initClass();
    NrTableMenuItemsCmd::initClass();

    CurTreeColCmd::initClass();
    CurTreeItemCmd::initClass();
    CurTreePathCmd::initClass();
    GetTreeColCmd::initClass();
    GetTreeItemCmd::initClass();
    GetTreePathCmd::initClass();
    IsTreeItemOnCmd::initClass();
    IsTreeItemExpandedCmd::initClass();
    NrTreeColsCmd::initClass();
    NrTreeItemsCmd::initClass();
    IsTreeButtonOnCmd::initClass();
    GetTreeMenuItemCmd::initClass();
    IsTreeMenuItemOnCmd::initClass();
    NrTreeMenuItemsCmd::initClass();
    IsWindowCmd::initClass();
}


bool Command::isQuestionName( const char* name, CmdDriver& cmddrv )
{
    PtrMan<Command> cmd = factory().create( factoryKey(name), cmddrv );
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
