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
#include "uifileselector.h"
#include "uimsg.h"
#include "dbman.h"
#include "uistrings.h"
#include "file.h"
#include "filepath.h"
#include "settings.h"
#include "survinfo.h"
#include "oddirs.h"
#include "od_ostream.h"


#define mIsUsable( dirnm ) DBMan::isValidDataRoot( dirnm ).isOK()


uiDataRootSel::uiDataRootSel( uiParent* p, const char* def )
    : uiGroup(p,"Data Root Selector")
    , selectionChanged(this)
{
    if ( !def || !*def )
	def = GetBaseDataDir();
    const BufferString defdir = mIsUsable(def) ? def
			      : Settings::common().find( sKeyDefRootDir() );
    BufferStringSet dirs;
    Settings::common().get( sKeyRootDirs(), dirs );
    if ( !defdir.isEmpty() && !dirs.isPresent(defdir) )
	dirs.insertAt( new BufferString(defdir), 0 );
    const BufferString curbasepath = SI().basePath();
    if ( !defdir.isEmpty() && !dirs.isPresent(defdir) )
	dirs.insertAt( new BufferString(defdir), 0 );

    BufferStringSet boxitms;
    for ( int idx=0; idx<dirs.size(); idx++ )
    {
	const BufferString dirnm = dirs.get( idx );
	if ( mIsUsable(dirnm) )
	{
	    boxitms.add( dirnm );
	    addDirNameToSettingsIfNew( dirnm, false );
	}
    }

    uiLabeledComboBox* fulldirfld = new uiLabeledComboBox( this,
						userDataRootString() );
    setHAlignObj( fulldirfld );
    dirfld_ = fulldirfld->box();
    dirfld_->addItems( boxitms );
    dirfld_->setEditable( true );
    fulldirfld->setStretch( 2, 0 );
    dirfld_->setStretch( 2, 0 );
    mAttachCB( dirfld_->selectionChanged, uiDataRootSel::dirChgCB );

    uiButton* selbut = uiButton::getStd( fulldirfld, OD::Select,
				     mCB(this,uiDataRootSel,selButCB), false );
    selbut->attach( rightOf, dirfld_ );

    setChoice( defdir );
}


uiString uiDataRootSel::userDataRootString()
{
    return tr("Survey Data Root");
}


BufferString uiDataRootSel::getInput() const
{
    return BufferString( dirfld_->text() );
}


void uiDataRootSel::selButCB( CallBacker* )
{
    const BufferString defdir = getInput();
    const char* defdirnm = mIsUsable(defdir) ? defdir.str() : 0;
    uiFileSelector::Setup fssu( defdirnm );
    fssu.selectDirectory();
    uiFileSelector uifs( this, fssu );
    uifs.caption() = uiStrings::phrSelect( userDataRootString() );
    if ( uifs.go() )
	addChoice( uifs.fileName(), true );
}


void uiDataRootSel::dirChgCB( CallBacker* )
{
   setChoice( getInput() );
}


void uiDataRootSel::setDir( const char* dirnm )
{
    BufferString resdirnm = addChoice( dirnm, false );
    if ( !resdirnm.isEmpty() )
	setChoice( resdirnm );
}


BufferString uiDataRootSel::addChoice( const char* dirnm, bool setcur )
{
    BufferString resdirnm = dirnm;
    uiRetVal uirv = getUsableDir( resdirnm );
    if ( uirv.isError() )
    {
	if ( setcur )
	    uiMSG().error( uirv );
	return BufferString::empty();
    }

    if ( !dirfld_->isPresent(resdirnm) )
    {
	NotifyStopper ns( dirfld_->selectionChanged );
	dirfld_->addItem( toUiString(resdirnm) );
    }

    if ( setcur )
	setChoice( resdirnm );

    return resdirnm;
}


void uiDataRootSel::setChoice( const char* dirnm )
{
    if ( previnput_ != dirnm )
    {
	NotifyStopper ns( dirfld_->selectionChanged );
	dirfld_->setCurrentItem( dirnm );
	selectionChanged.trigger();
	previnput_ = dirnm;
    }
}


#define mErrRet(s) { uiMSG().error( s ); return false; }

uiRetVal uiDataRootSel::getUsableDir( BufferString& dirnm ) const
{
    uiRetVal uirv = DBMan::isValidDataRoot( dirnm );
    if ( uirv.isOK() )
	return isValidFolder( dirnm );

    if ( !File::isWritable(dirnm) )
    {
	uirv = File::exists(dirnm) ? tr("Directory is not writable")
				   : tr("Directory does not exist");
	return uirv;
    }

    File::Path survfnamefp( dirnm, ".survey" );
    if ( File::exists(survfnamefp.fullPath()) )
    {
	const BufferString pardirnm( File::Path(dirnm).pathOnly() );
	uiRetVal newuirv = DBMan::isValidDataRoot( pardirnm );
	if ( newuirv.isOK() )
	    dirnm.set( pardirnm );
    }

    uirv = isValidFolder( dirnm );
    if ( !uirv.isOK() )
	return uirv;

    // OK, so the dir seems suitable, but it hasn't got a .omf.
    // why ask? let's just put one.
    const BufferString stdomf( mGetSetupFileName("omf") );
    const BufferString omffnm = File::Path(dirnm,".omf").fullPath();
    File::copy( stdomf, omffnm );

    return uirv;
}


uiRetVal uiDataRootSel::isValidFolder( const char* dirnm ) const
{
    const File::Path fp( dirnm );

    // Inside the OpendTect installation
    File::Path fpodinst( GetSoftwareDir(0) );
    const int nrodinstlvls = fpodinst.nrLevels();
    if ( fp.nrLevels() >= nrodinstlvls )
    {
	const BufferString dirnm_at_odinstlvl( fp.dirUpTo(nrodinstlvls-1) );
	if ( dirnm_at_odinstlvl == fpodinst.fullPath() )
	    return uiRetVal( tr("The directory you have chosen is"
		    "\n*INSIDE*\nthe software installation directory."
		    "\nThis leads to many problems, and we cannot support this."
		    "\n\nPlease choose another directory") );
    }

    return uiRetVal::OK();
}


BufferString uiDataRootSel::getDir()
{
    BufferString dirnm( getInput() );
    if ( getUsableDir(dirnm).isOK() )
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


mGlobal(Basic) void SetBaseDataDir(const char*);


bool uiDataRootSel::setRootDirOnly( const char* dirnm )
{
    uiRetVal uirv = DBMan::isValidDataRoot( dirnm );
    if ( !uirv.isOK() )
	{ gUiMsg().error( uirv ); return false; }

    SetBaseDataDir( dirnm );
    addDirNameToSettingsIfNew( dirnm, true );
    return true;
}


void uiDataRootSel::writeDefSurvFile( const char* dirnm )
{
    od_ostream strm( GetLastSurveyFileName() );
    if ( strm.isOK() )
	strm << dirnm;
}
