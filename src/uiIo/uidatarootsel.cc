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


#define mIsUsable( dirnm ) DBMan::isValidDataRoot( dirnm ).isOK()


uiDataRootSel::uiDataRootSel( uiParent* p, const char* def )
    : uiGroup(p,"Data Root Selector")
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

    uiButton* selbut = uiButton::getStd( fulldirfld, OD::Select,
				     mCB(this,uiDataRootSel,selButCB), false );
    selbut->attach( rightOf, dirfld_ );
}


uiString uiDataRootSel::userDataRootString()
{
    return tr( "OpendTect Data Root Directory" );
}


void uiDataRootSel::selButCB( CallBacker* )
{
    const BufferString defdir( dirfld_->text() );
    const char* defdirnm = mIsUsable(defdir) ? defdir.str() : 0;
    uiFileDialog dlg( this, uiFileDialog::DirectoryOnly, defdirnm, 0,
		      uiStrings::phrSelect(userDataRootString()) );
    if ( dlg.go() )
    {
	BufferString newdirnm = dlg.fileName();
	if ( handleDirName(newdirnm) )
	    dirfld_->setText( newdirnm );
    }
}


#define mErrRet(s) { uiMSG().error( s ); return false; }

bool uiDataRootSel::handleDirName( BufferString& dirnm ) const
{
    uiRetVal uirv = DBMan::isValidDataRoot( dirnm );
    if ( uirv.isOK() )
	return isValidFolder(dirnm);

    if ( !File::isWritable(dirnm) )
	mErrRet( uirv )

    FilePath survfnamefp( dirnm, ".survey" );
    if ( File::exists(survfnamefp.fullPath()) )
    {
	const BufferString pardirnm( FilePath(dirnm).pathOnly() );
	uiRetVal newuirv = DBMan::isValidDataRoot( pardirnm );
	if ( newuirv.isOK() )
	    { dirnm.set( pardirnm ); return true; }
	mErrRet( uirv );
    }

    if ( !isValidFolder(dirnm) )
	return false;

    // OK, so the dir seems suitable, but it hasn't got a .omf.
    // why ask? let's just put one.
    const BufferString stdomf( mGetSetupFileName("omf") );
    const BufferString omffnm = FilePath(dirnm,".omf").fullPath();
    File::copy( stdomf, omffnm );
    return true;
}


bool uiDataRootSel::isValidFolder( const char* dirnm ) const
{
    const FilePath fp( dirnm );

    // Inside the OpendTect installation
    FilePath fpodinst( GetSoftwareDir(0) );
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


BufferString uiDataRootSel::getDir() const
{
    BufferString ret( dirfld_->text() );
    if ( !handleDirName(ret) )
	ret.setEmpty();
    else
	addDirNameToSettingsIfNew( ret );
    return ret;
}


void uiDataRootSel::addDirNameToSettingsIfNew( const char* dirnm )
{
    BufferStringSet dirs;
    Settings::common().get( sKeyRootDirs(), dirs );
    if ( dirs.isPresent(dirnm) )
	return;

    dirs.add( dirnm );
    Settings::common().set( sKeyRootDirs(), dirs );
    Settings::common().write();
}


extern "C" { mGlobal(Basic) void SetBaseDataDir(const char*); }

uiRetVal uiDataRootSel::setSurveyDirTo( const char* dirnm )
{
    if ( !dirnm || !*dirnm )
	return uiRetVal::OK();

    const FilePath fp( dirnm );
    const BufferString dataroot = fp.pathOnly();
    uiRetVal uirv = DBMan::isValidDataRoot( dataroot );
    if ( !uirv.isOK() )
	return uirv;

    const BufferString survdir = fp.fullPath();
    uirv = DBMan::isValidSurveyDir( survdir );
    if ( !uirv.isOK() )
	return uirv;

    const BufferString curdataroot = GetBaseDataDir();
    const BufferString cursurveydir = DBM().survDir();



    return uiRetVal::OK();
}

/*

static uiRetVal doSetRootDataDir( const char* dirnm )
{
    const BufferString dataroot = dirnm;



    SetBaseDataDir( dataroot );
    Settings::common().set( sKeyDefRootDir(), dataroot );
    if ( Settings::common().write() )
	return uiRetVal::OK();
    else
	return od_static_tr( "doSetRootDataDir",
			     "Cannot write user settings file" );


}

*/
