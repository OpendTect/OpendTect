/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uicrdevenv.h"

#include "uifileinput.h"
#include "uilabel.h"
#include "uimsg.h"

#include "dirlist.h"
#include "file.h"
#include "oddirs.h"
#include "odjson.h"
#include "odver.h"
#include "od_helpids.h"
#include "od_ostream.h"


namespace OD
{

static const char* TutPlugins[] =
{
    "Tut",
    "uiTut",
    nullptr
};


static bool getTutSources( const char* plugindir,
			   BufferStringSet& srcfiles,
			   const char* plugindirout=nullptr,
			   BufferStringSet* srcfilesout=nullptr )
{
    const BufferStringSet plugindirs( TutPlugins );
    static BufferString includemask( "*.h" );
    static BufferString sourcemask( "*.cc" );
    for ( const auto* dir : plugindirs )
    {
	const FilePath pluginfpin( plugindir, dir->buf() );
	const FilePath cmakelistfp( pluginfpin, "CMakeLists.txt" );
	if ( !cmakelistfp.exists() )
	    return false;

	srcfiles.add( cmakelistfp.fullPath() );
	FilePath pluginfpout;
	if ( srcfilesout )
	{
	    pluginfpout.set( plugindirout ).add( dir->buf() );
	    if ( !pluginfpout.exists() )
		File::createDir( pluginfpout.fullPath() );

	    srcfilesout->add(FilePath(pluginfpout,
					    "CMakeLists.txt").fullPath());
	}

	const BufferString plugindirnm = pluginfpin.fullPath();
	DirList incldl( plugindirnm.buf(), File::DirListType::FilesInDir,
			"*.h" );
	if ( __iswin__ )
	{
	    BufferString deffnm( dir->buf(), "deps.h" );
	    deffnm.toLower();
	    incldl.remove( deffnm.str() );
	}

	const DirList srcdl( plugindirnm.buf(),
			     File::DirListType::FilesInDir, "*.cc" );
	if ( incldl.isEmpty() || srcdl.isEmpty() )
	    return false;

	BufferStringSet files( incldl );
	files.append( srcdl );
	for ( const auto* filenm : files )
	{
	    const FilePath fp( plugindirnm.buf(), filenm->str() );
	    srcfiles.add( fp.fullPath() );
	    if ( !srcfilesout || pluginfpout.isEmpty() )
		continue;

	    const FilePath fpout( pluginfpout, filenm->str() );
	    srcfilesout->add( fpout.fullPath() );
	}
    }

    if ( srcfiles.isEmpty() )
	return false;

    return srcfilesout ? srcfilesout->size() == srcfiles.size() : true;
}


static BufferString getRootSoftwareDir()
{
    FilePath swfp( GetSoftwareDir(false) );
    if ( __ismac__ )
	swfp.add( "Resources" );

    return swfp.fullPath();
}


static uiRetVal isSoftwareDirOK( const char* swdir, bool quick )
{
    if ( !File::isDirectory(swdir) )
	return od_static_tr( "uiCrDevEnv",
		    "Cannot find the OpendTect SDK at '%1'").arg( swdir );

    const bool isdevbuild = isDeveloperBuild();
    if ( !isdevbuild )
    {
	BufferString develverstr;
	GetSpecificODVersion( "devel", develverstr );
	if ( develverstr.isEmpty() )
	    return od_static_tr( "uiCrDevEnv", "Please download\n"
				 "and install development package first" );
    }

    if ( quick )
	return uiRetVal::OK();

    const FilePath cmakelistfp( swdir, "CMakeLists.txt" );
    const FilePath includefp( swdir, "include");
    const FilePath srcfp( swdir, "src" );
    const FilePath progfp( swdir, "doc", "Programmer", "pluginexample" );
    const FilePath progcmakelistfp( progfp, cmakelistfp.fileName() );
    const FilePath progversionfp( progfp, "version.h.in" );
    FilePath progpluginsfp( progfp );
    if ( isdevbuild )
	progpluginsfp.set( swdir );
    else
	progpluginsfp = progfp;

    progpluginsfp.add( "plugins" );
    if ( !cmakelistfp.exists() || !includefp.exists() || !srcfp.exists() ||
	 !progcmakelistfp.exists() || !progversionfp.exists() ||
	 !progpluginsfp.exists() )
	return od_static_tr( "uiCrDevEnv",
			     "The developments package seems "
			     "incomplete, try re-installing the package" );

    BufferStringSet srcfiles;
    if ( !OD::getTutSources(progpluginsfp.fullPath(),srcfiles) )
	return od_static_tr( "uiCrDevEnv",
			     "Cannot find the Tutorial source files" );

    return uiRetVal::OK();
}

} // namespace OD


uiCrDevEnv::uiCrDevEnv( uiParent* p, const FilePath& workdirfp )
    : uiDialog(p,uiDialog::Setup(tr("Create Development Environment"),
				 mNoDlgTitle,mODHelpKey(mCreateDevEnvHelpID)))
{
    auto* lbl = new uiLabel( this,
		    tr("Specify OpendTect plugin development location.\n") );
    lbl->attach( leftBorder );

    workdirfld_ = new uiGenInput( this, uiStrings::sName(),
				  workdirfp.fileName() );
    workdirfld_->attach( ensureBelow, lbl );

    basedirfld_ = new uiFileInput( this, tr("Create in"),
			uiFileInput::Setup(workdirfp.pathOnly())
					    .directories(true) );
    basedirfld_->attach( alignedBelow, workdirfld_ );
}


uiCrDevEnv::~uiCrDevEnv()
{
}


FilePath uiCrDevEnv::getWorkDir() const
{
    return FilePath( basedirfld_->text(), workdirfld_->text() );
}


uiRetVal uiCrDevEnv::canCreateDevEnv()
{
    const BufferString swdir( OD::getRootSoftwareDir() );
    return OD::isSoftwareDirOK( swdir.buf(), true );
}


#undef mErrRet
#define mErrRet(s) { uiMSG().error(s); return; }

void uiCrDevEnv::crDevEnv( uiParent* appl )
{
    const BufferString swdir = OD::getRootSoftwareDir();
    const uiRetVal swdircheck = OD::isSoftwareDirOK( swdir.buf(), false );
    if ( !swdircheck.isOK() )
    {
	uiMSG().error( swdircheck );
	return;
    }

    //TODO: store/read the previous valid value from the user settings?
    static FilePath workdirfp;
    if ( workdirfp.isEmpty() )
	workdirfp.set( GetPersonalDir() ).add( "ODWork" );

    uiCrDevEnv dlg( appl, workdirfp );
    if ( dlg.go() != uiDialog::Accepted )
	return;

    workdirfp = dlg.getWorkDir();
    const BufferString workdir( workdirfp.fullPath() );
    if ( workdirfp.exists() )
    {
	uiString msg;
	const bool isdir = File::isDirectory( workdir.str() );
	if ( isdir )
	{
	    msg = od_static_tr( "uiCrDevEnv",
			"The folder you selected (%1)\nalready exists.\n\n")
		.arg( workdir.str() );
	}
	else
	{
	    msg = tr("You selected a file.\n\n");
	}

	msg.append("Do you want to completely remove the existing %1\n"
		    "and create a new development location there?")
	    .arg(isdir ? tr("folder") : tr("file"));

	if ( !uiMSG().askDelete(msg) )
	    return;

	if ( isdir )
	    File::removeDir( workdir.str() );
	else
	    File::remove( workdir.str() );
    }

    const uiRetVal uirv = copyEnv( swdir, workdir.str() );
    if ( !uirv.isOK() )
    {
	uiMSG().error( uirv );
	return;
    }

    uiMSG().message( od_static_tr( "uiCrDevEnv",
			"Creation of the environment has succeeded.") );
    const uiString docmsg = od_static_tr( "uiCrDevEnv",
	    "Do you want to take a look at the developers documentation?" );
    if ( uiMSG().askGoOn(docmsg) )
	HelpProvider::provideHelp( HelpKey("dev",0) );
}


uiRetVal uiCrDevEnv::copyEnv( const char* swdir, const char* envdir )
{
    if ( File::isDirectory(envdir) )
    {
	if ( !File::isWritable(envdir) )
	    return uiStrings::phrCannotWrite( envdir );
    }
    else
    {
	if ( !File::createDir(envdir) )
	    return uiStrings::phrCannotCreateDirectory(toUiString(envdir));
    }

    const FilePath cmakemodfpout( envdir, "CMakeModules" );
    if ( !cmakemodfpout.exists() &&
	 !File::createDir(cmakemodfpout.fullPath()) )
	return uiStrings::phrCannotCreateDirectory(
				    toUiString( cmakemodfpout.fullPath()) );

    const FilePath pluginsfpout( envdir, "plugins" );
    if ( !pluginsfpout.exists() && !File::createDir(pluginsfpout.fullPath()) )
	return uiStrings::phrCannotCreateDirectory(
				    toUiString( pluginsfpout.fullPath()) );

    const FilePath progfp( swdir, "doc", "Programmer", "pluginexample" );
    const FilePath cmakelistsfpin( progfp, "CMakeLists.txt" );
    const FilePath cmakelistsfpout( envdir, "CMakeLists.txt" );
    if ( !File::copy(cmakelistsfpin.fullPath(),cmakelistsfpout.fullPath()) )
	return uiStrings::phrCannotCopy(
				    toUiString(cmakelistsfpin.fullPath()) );

    const FilePath versionsfpin( progfp, "version.h.in" );
    const FilePath versionsfpout( cmakemodfpout, "version.h.in" );
    if ( !File::copy(versionsfpin.fullPath(),versionsfpout.fullPath()) )
	return uiStrings::phrCannotCopy(toUiString(versionsfpin.fullPath()) );

    const FilePath cmakepresetsfpin( progfp, "CMakeUserPresets.json" );
    const FilePath cmakepresetsfpout( envdir, "CMakeUserPresets.json" );
    if ( !File::copy(cmakepresetsfpin.fullPath(),
	cmakepresetsfpout.fullPath()) )
	return uiStrings::phrCannotCopy(
				    toUiString(cmakepresetsfpin.fullPath()) );

    FilePath pluginsfpin;
    if ( isDeveloperBuild() )
	pluginsfpin.set( swdir );
    else
	pluginsfpin = progfp;

    pluginsfpin.add( "plugins" );
    const BufferString pluginsout = pluginsfpout.fullPath();
    BufferStringSet srcfilesin, srcfilesout;
    if ( !OD::getTutSources(pluginsfpin.fullPath(),srcfilesin,
			    pluginsout.str(),&srcfilesout) )
	return od_static_tr( "uiCrDevEnv", "Cannot copy all source files" );

    for ( int idx=0; idx<srcfilesin.size(); idx++ )
    {
	if ( !File::copy(srcfilesin.get(idx),srcfilesout.get(idx)) )
	    return od_static_tr( "uiCrDevEnv",
				 "Cannot copy all source files" );
    }

    const BufferString cmakepresetsfnm = cmakepresetsfpout.fullPath();

    uiRetVal uirv;
    PtrMan<OD::JSON::ValueSet> cmakepresets =
			    OD::JSON::ValueSet::read( cmakepresetsfnm, uirv );
    if ( !uirv.isOK() )
    {
	uiMSG().warning( tr("Please fill the CMake Presets manually") );
	return uiRetVal::OK();
    }

    const FilePath swdirfp( swdir );
    BufferString swdirfnm( swdirfp.fullPath() );
    if ( __iswin__ )
    {
	BufferString longswdirfnm = FilePath::getLongPath( swdirfnm.buf() );
	longswdirfnm.replace( FilePath::dirSep(FilePath::Windows),
			      FilePath::dirSep(FilePath::Unix) );
	swdirfnm = longswdirfnm;
    }

    OD::JSON::Object& cmakepresetsobj = cmakepresets->asObject();
    OD::JSON::Array* configpresets = cmakepresetsobj.getArray(
							"configurePresets" );
    if ( !configpresets )
	return uiRetVal::OK();

    OD::JSON::Object* commonpreset = nullptr;
    for ( int idx=0; idx<configpresets->size(); idx++ )
    {
	if ( configpresets->isObjectChild(idx) )
	{
	    OD::JSON::Object& preset = configpresets->object( idx );
	    BufferString namestr = preset.getStringValue( "name" );
	    if ( namestr == "common" )
	    {
		commonpreset = &preset;
		break;
	    }
	}
    }

    if ( !commonpreset )
	return uiRetVal::OK();

    OD::JSON::Object* cachevars = commonpreset->getObject( "cacheVariables") ;
    if ( cachevars )
    {
	OD::JSON::Object* oddir = cachevars->getObject( "OpendTect_DIR" );
	if ( oddir )
	    oddir->set( "value", swdirfnm.str() );

	OD::JSON::Object* odbindir = cachevars->getObject("OD_BINARY_BASEDIR");
	if ( odbindir )
	    odbindir->set( "value", swdirfnm.str() );
    }

    return cmakepresets->write( cmakepresetsfnm, true );
}


#undef mErrRet
#define mErrRet(msg) { uiMSG().error( msg ); return false; }

bool uiCrDevEnv::acceptOK( CallBacker* )
{
    const BufferString basedir = basedirfld_->text();
    if ( basedir.isEmpty() || !File::isDirectory(basedir) )
	mErrRet( tr("Please enter a valid (existing) location") )

    const BufferString workdirnm = workdirfld_->text();
    if ( workdirnm.isEmpty() )
	mErrRet( tr("Please enter a (sub-)folder name") )

    const FilePath workdirfp = getWorkDir();
    const BufferString workdir = workdirfp.fullPath();
    if ( !File::exists(workdir.str()) && __iswin__ &&
	 workdir.matches("Program Files",OD::CaseInsensitive) )
	mErrRet( tr("Please do not use 'Program Files'.\n"
		    "Instead, a folder like 'My Documents' would be OK.") )

    return true;
}
