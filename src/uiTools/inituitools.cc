/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "moddepmgr.h"

#include "uiarray2dinterpol.h"
#include "uibatchjobdispatcherlauncher.h"
#include "uifontsel.h"
#include "uigridder2d.h"
#include "uiinterpollayermodel.h"
#include "uimainwin.h"
#include "uiraytrace1d.h"
#include "uirefltrace1d.h"

#include "file.h"
#include "filepath.h"
#include "genc.h"
#include "oddirs.h"
#include "odiconfile.h"
#include "plugins.h"
#include "procdescdata.h"
#include "settings.h"


namespace OD
{

static void addODIconFolders( CallBacker* )
{
    const StringView icsetnm( IconFile::getIconSubFolderName() );
    BufferStringSet icondirnms;
    if ( GetSetupShareDirNames(icsetnm.buf(),icondirnms,true) )
    {
	for ( const auto* dirnm : icondirnms )
	    addIconsFolder( dirnm->str() );
    }

    const StringView deficsetnm( IconFile::getDefaultIconSubFolderName() );
    if ( deficsetnm == icsetnm )
	return;

    icondirnms.setEmpty();
    if ( GetSetupShareDirNames(deficsetnm.buf(),icondirnms) )
    {
	for ( const auto* dirnm : icondirnms )
	    addIconsFolder( dirnm->str() );
    }
}

} // namespace OD


mDefModInitFn(uiTools)
{
    mIfNotFirstTime( return );

    uiInverseDistanceGridder2D::initClass();
    uiInverseDistanceArray2DInterpol::initClass();
    uiTriangulationArray2DInterpol::initClass();
    uiExtensionArray2DInterpol::initClass();
    uiVrmsRayTracer1D::initClass();
    uiAICalc1D::initClass();

    uiSingleBatchJobDispatcherLauncher::initClass();
    uiZSliceInterpolationModel::initClass();

    uiGeneralSettingsGroup::initClass();
    uiVisSettingsGroup::initClass();
    uiFontSettingsGroup::initClass();
    OD::addODIconFolders( nullptr );
    uiMainWin::iconSetChanged().notify( mSCB(OD::addODIconFolders) );

    if ( !NeedDataBase() )
	return;

    BufferString libnm; libnm.setMinBufSize( 32 );
    SharedLibAccess::getLibName( "uiCRS", libnm.getCStr(), libnm.bufSize() );
    const FilePath libfp( GetLibPlfDir(), libnm );
    if ( libfp.exists() )
	PIM().load( libfp.fullPath(), PluginManager::Data::AppDir,
		    PI_AUTO_INIT_LATE );

    gatherFireWallProcInf();
}
