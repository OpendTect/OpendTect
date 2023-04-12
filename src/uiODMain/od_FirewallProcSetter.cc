/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "commandlineparser.h"
#include "file.h"
#include "moddepmgr.h"
#include "oddirs.h"
#include "prog.h"
#include "pythonaccess.h"
#include "procdescdata.h"
#include "uifirewallprocsetterdlg.h"
#include "uidialog.h"
#include "uifileinput.h"
#include "uimain.h"
#include "uimsg.h"


//Parameter dialog box

mClass(uiODMain) FirewallParameterDlg : public uiDialog
{ mODTextTranslationClass(FirewallParameterDlg)
protected:
    uiFileInput*    odpthfld_ = nullptr;
    uiFileInput*    pypthfld_ = nullptr;
    uiGenInput*	    acttypefld_ = nullptr;

    BufferString    odpth_;
    BufferString    pypth_;
    BufferString    actcmd_;

public:
    FirewallParameterDlg( uiParent* p, const BufferString& odpath,
			    const BufferString& pypath,
			    const BufferString& cmd )
    : uiDialog(p,uiDialog::Setup(tr("Missing Parameters"),dlgTitle(),
			    mNoHelpKey).oktext(tr("Update Parameters")))
    {
	uiFileInput::Setup flsu;
	flsu .forread( true );
	flsu.directories( true );
	odpthfld_ = new uiFileInput( this, tr("OpendTect Folder Path"),
								    flsu );
	odpthfld_->setFileName( odpath );

	pypthfld_ = new uiFileInput( this, tr("Python Folder Path"),
								    flsu );
	pypthfld_->attach( alignedBelow, odpthfld_ );
	odpthfld_->setFileName( odpath );

	if ( !ProcDesc::DataEntry::isCMDOK(cmd) )
	{
	    acttypefld_ = new uiGenInput( this, tr("Action"),
		StringListInpSpec
		(ProcDesc::DataEntry::ActionTypeDef().strings()) );
	    PDE::ActionType type = PDE::getActionTypeForCMDKey( cmd );
	    acttypefld_->setText( PDE::ActionTypeDef().getKey(type) );
	    acttypefld_->attach( alignedBelow, pypthfld_ );
	}
	else
	    actcmd_ = cmd;
    }

    bool acceptOK( CallBacker* )
    {
	odpth_ = odpthfld_->text();
	pypth_ = pypthfld_->text();
	if ( acttypefld_ )
	{
	    const int idx = acttypefld_->getIntValue();
	    actcmd_ = PDE::getCMDActionKey(
			    PDE::ActionTypeDef().getEnumForIndex(idx) );
	}
	return true;
    }

    static uiString dlgTitle()
    {
	uiString txt = tr("Please input the value of following parameters.");
	txt.append(tr("These seem to be incorrectly read or\n"
	    "location they are directed to is unavailable"), true );

	return txt;
    }

    const BufferString& getODPath() const { return odpth_; }
    const BufferString& getPyPath() const { return pypth_; }
    const BufferString& getActCmd() const { return actcmd_; }
};

static const char* odflag()
{
    return ProcDesc::DataEntry::getTypeFlag( ProcDesc::DataEntry::OD );
}


static const char* pyflag()
{
    return ProcDesc::DataEntry::getTypeFlag( ProcDesc::DataEntry::Python );
}


static void printBatchUsage()
{
    od_ostream& strm = od_ostream::logStream();
    strm << "Usage: " << "od_FilewallProcSetter ";
    strm << "[--help] ";
    strm << "[--" << odflag() << " <path till the OpendTect installation folder"
		" ([...]/OpendTect/6.6.0)>] ";
    strm << "[--" << pyflag() << " <path till python folder>]";
    strm << "It uses administrative rights to "
	    "launch FirewallProcSetter Dialog\n";
    strm << "\nThese are common actions that dialog can do\n";
    strm << "\t"
	 << ProcDesc::DataEntry::getCMDActionKey( ProcDesc::DataEntry::Add )
	 << "\t to add rules to firewall\n";
    strm << "\t"
	 << ProcDesc::DataEntry::getCMDActionKey( ProcDesc::DataEntry::Remove )
	 << "\t to remove rules from firewall\n";
    strm << "\t"
	 << ProcDesc::DataEntry::getCMDActionKey(
					ProcDesc::DataEntry::AddNRemove )
	 << "\t  to add and remove rules to firewall\n";
    strm << "\n Use short and quoted paths";
    strm << od_endl;
}

#define mErrRet() \
{ \
    printBatchUsage(); \
    return 1; \
}


int mProgMainFnName( int argc, char** argv )
{
    mInitProg( OD::UiProgCtxt )
    SetProgramArgs( argc, argv, false );
    uiMain app( argc, argv );

    OD::ModDeps().ensureLoaded( "uiTools" );

    const CommandLineParser parser( argc, argv );
    if ( parser.hasKey("help") || parser.hasKey("h") )
    {
	printBatchUsage();
	return 0;
    }

    parser.setKeyHasValue( odflag() );
    parser.setKeyHasValue( pyflag() );
    BufferStringSet proctyp;
    parser.getNormalArguments( proctyp );
    bool errocc = false;
    if ( proctyp.isEmpty() )
	errocc = true;

    BufferString type;
    if ( !errocc )
	type = proctyp.get( 0 );

    BufferString path;
    if ( !parser.getVal(odflag(),path) && !File::isDirectory(path) )
	errocc = true;

    BufferString pythonpath;
    if ( !parser.getVal(pyflag(),pythonpath) && !File::isDirectory(pythonpath) )
	errocc = true;

    PIM().loadAuto( false );
    OD::ModDeps().ensureLoaded( "uiODMain" );
    PtrMan<uiFirewallProcSetter> topdlg = nullptr;
    if ( errocc )
    {
	topdlg = new uiFirewallProcSetter( nullptr );
	app.setTopLevel( topdlg );
	PIM().loadAuto( true );

	PtrMan<FirewallParameterDlg> dlg = new FirewallParameterDlg( topdlg,
						    path, pythonpath, type );
	dlg->setActivateOnFirstShow();
	dlg->showAlwaysOnTop();
	if ( dlg->go() == uiDialog::Rejected )
	    return 1;

	type = dlg->getActCmd();
	path = dlg->getODPath();
	pythonpath = dlg->getPyPath();
	ePDD().setPath( path );
	const ProcDesc::DataEntry::ActionType opertype =
			ProcDesc::DataEntry::getActionTypeForCMDKey( type );
	const bool isrem = PDE::Remove == opertype;
	if ( !ePDD().hasWorkToDo(pythonpath,!isrem) )
	{
	    const uiString msg = toUiString("No executables for "
	    "%1 Firewall rules").arg(isrem ? "removal from" : "adding to" );
	    uiMSG().warning( msg );
	    return 0;
	}

	topdlg->updateUI( path, pythonpath, opertype );
	topdlg->setActivateOnFirstShow();
    }
    else
    {
	ePDD().setPath( path );
	const ProcDesc::DataEntry::ActionType opertype =
			ProcDesc::DataEntry::getActionTypeForCMDKey( type );
	const bool isrem = PDE::Remove == opertype;
	if ( !ePDD().hasWorkToDo(pythonpath,!isrem) )
	    return 0;

	topdlg = new uiFirewallProcSetter( nullptr, opertype, &path,
					   &pythonpath );
	app.setTopLevel( topdlg );
	PIM().loadAuto( true );
    }

    topdlg->show();

    return app.exec();
}
