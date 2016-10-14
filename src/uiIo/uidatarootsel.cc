/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          Oct 2016
________________________________________________________________________

-*/

#include "uidatarootsel.h"

#include "uicombobox.h"
#include "uibutton.h"
#include "uifiledlg.h"
#include "uimsg.h"
#include "dbman.h"
#include "uistrings.h"
#include "file.h"
#include "filepath.h"
#include "settings.h"
#include "oddirs.h"
#include "od_ostream.h"


#define mIsUsable( dirnm ) DBMan::isValidDataRoot( dirnm ).isOK()


uiDataRootSel::uiDataRootSel( uiParent* p, const char* def )
    : uiGroup(p,"Data Root Selector")
    , selectionChanged(this)
{
    const BufferString defdir = mIsUsable(def) ? def
			      : Settings::common().find( sKeyDefRootDir() );
    BufferStringSet dirs;
    Settings::common().get( sKeyRootDirs(), dirs );
    if ( !defdir.isEmpty() && !dirs.isPresent(defdir) )
	dirs.insertAt( new BufferString(defdir), 0 );

    BufferStringSet boxitms;
    for ( int idx=0; idx<dirs.size(); idx++ )
    {
	const BufferString dirnm = dirs.get( idx );
	if ( mIsUsable(dirnm) )
	    boxitms.add( dirnm );
    }

    uiLabeledComboBox* fulldirfld = new uiLabeledComboBox( this,
						userDataRootString() );
    setHAlignObj( fulldirfld );
    dirfld_ = fulldirfld->box();
    dirfld_->addItems( boxitms );
    dirfld_->setEditable( true );
    mAttachCB( dirfld_->editTextChanged, uiDataRootSel::dirChgCB );
    mAttachCB( dirfld_->selectionChanged, uiDataRootSel::dirChgCB );

    uiButton* selbut = uiButton::getStd( fulldirfld, OD::Select,
				     mCB(this,uiDataRootSel,selButCB), false );
    selbut->attach( rightOf, dirfld_ );
}


uiString uiDataRootSel::userDataRootString()
{
    return tr( "OpendTect Data Root Directory" );
}


BufferString uiDataRootSel::getInput() const
{
    return BufferString( dirfld_->text() );
}


void uiDataRootSel::selButCB( CallBacker* )
{
    const BufferString defdir = getInput();
    const char* defdirnm = mIsUsable(defdir) ? defdir.str() : 0;
    uiFileDialog dlg( this, uiFileDialog::DirectoryOnly, defdirnm, 0,
		      uiStrings::phrSelect(userDataRootString()) );
    if ( dlg.go() )
	checkAndSetCorrected( dlg.fileName() );
}


void uiDataRootSel::dirChgCB( CallBacker* )
{
    checkAndSetCorrected( getInput() );
}


void uiDataRootSel::checkAndSetCorrected( const char* dirnm )
{
    BufferString resdirnm = dirnm;
    if ( getUsableDir(resdirnm) && getInput() != resdirnm )
    {
	NotifyStopper ns1( dirfld_->editTextChanged );
	NotifyStopper ns2( dirfld_->selectionChanged );
	dirfld_->setText( resdirnm );
	selectionChanged.trigger();
    }
}


#define mErrRet(s) { uiMSG().error( s ); return false; }

bool uiDataRootSel::getUsableDir( BufferString& dirnm ) const
{
    uiRetVal uirv = DBMan::isValidDataRoot( dirnm );
    if ( uirv.isOK() )
	return isValidFolder( dirnm );

    if ( !File::isWritable(dirnm) )
	mErrRet( uirv )

    File::Path survfnamefp( dirnm, ".survey" );
    if ( File::exists(survfnamefp.fullPath()) )
    {
	const BufferString pardirnm( File::Path(dirnm).pathOnly() );
	uiRetVal newuirv = DBMan::isValidDataRoot( pardirnm );
	if ( newuirv.isOK() )
	    dirnm.set( pardirnm );
	else
	    mErrRet( uirv );
    }

    if ( !isValidFolder(dirnm) )
	return false;

    // OK, so the dir seems suitable, but it hasn't got a .omf.
    // why ask? let's just put one.
    const BufferString stdomf( mGetSetupFileName("omf") );
    const BufferString omffnm = File::Path(dirnm,".omf").fullPath();
    File::copy( stdomf, omffnm );

    return true;
}


bool uiDataRootSel::isValidFolder( const char* dirnm ) const
{
    const File::Path fp( dirnm );

    // Inside the OpendTect installation
    File::Path fpodinst( GetSoftwareDir(0) );
    const int nrodinstlvls = fpodinst.nrLevels();
    if ( fp.nrLevels() >= nrodinstlvls )
    {
	const BufferString dirnm_at_odinstlvl( fp.dirUpTo(nrodinstlvls-1) );
	if ( dirnm_at_odinstlvl == fpodinst.fullPath() )
	    mErrRet( tr("The directory you have chosen is"
		    "\n*INSIDE*\nthe software installation directory."
		    "\nThis leads to many problems, and we cannot support this."
		    "\n\nPlease choose another directory") )
    }

    return true;
}


BufferString uiDataRootSel::getDir()
{
    BufferString dirnm( getInput() );
    if ( getUsableDir(dirnm) )
	addDirNameToSettingsIfNew( dirnm, false );
    else
	dirnm.setEmpty();
    return dirnm;
}


void uiDataRootSel::addDirNameToSettingsIfNew( const char* dirnm, bool mkcur )
{
    bool chgd = false;
    BufferStringSet dirs;
    Settings::common().get( sKeyRootDirs(), dirs );
    if ( !dirs.isPresent(dirnm) )
    {
	dirs.add( dirnm );
	Settings::common().set( sKeyRootDirs(), dirs );
	chgd = true;
    }
    if ( mkcur )
    {
	BufferString curdef = Settings::common().find( sKeyDefRootDir() );
	if ( curdef != dirnm )
	{
	    Settings::common().set( sKeyDefRootDir(), dirnm );
	    chgd = true;
	}
    }

    if ( chgd )
	Settings::common().write();
}


extern "C" { mGlobal(Basic) void SetBaseDataDir(const char*); }


bool uiDataRootSel::setRootDirOnly( const char* dirnm )
{
    uiRetVal uirv = DBMan::isValidDataRoot( dirnm );
    if ( !uirv.isOK() )
	{ uiMSG().error( uirv ); return false; }

    SetBaseDataDir( dirnm );
    addDirNameToSettingsIfNew( dirnm, true );
    return true;
}


uiRetVal uiDataRootSel::setSurveyDirTo( const char* dirnm )
{
    if ( !dirnm || !*dirnm )
	return uiRetVal::OK();

    const File::Path fp( dirnm );
    const BufferString newdataroot = fp.pathOnly();
    uiRetVal uirv = DBMan::isValidDataRoot( newdataroot );
    if ( !uirv.isOK() )
	return uirv;

    const BufferString newsurvdir = fp.fullPath();
    uirv = DBMan::isValidSurveyDir( newsurvdir );
    if ( !uirv.isOK() )
	return uirv;

    const BufferString curdataroot = GetBaseDataDir();
    const BufferString cursurveydir = DBM().survDir();
    if ( curdataroot == newdataroot && cursurveydir == newsurvdir )
	return uiRetVal::OK();

    uirv = DBM().setDataSource( newsurvdir );
    if ( !uirv.isOK() )
	return uirv;

    const BufferString newsurvdirnm = fp.fileName();
    addDirNameToSettingsIfNew( newdataroot, true );
    writeDefSurvFile( fp.fileName() );

    return uiRetVal::OK();
}


void uiDataRootSel::writeDefSurvFile( const char* dirnm )
{
    od_ostream strm( GetLastSurveyFileName() );
    if ( strm.isOK() )
	strm << dirnm;
}
