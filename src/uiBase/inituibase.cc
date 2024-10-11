/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "filepath.h"
#include "moddepmgr.h"
#include "oddirs.h"
#include "odruncontext.h"
#include "plugins.h"
#include "settings.h"

#include "uibutton.h"
#include "uicursor.h"
#include "uifileseltool.h"
#include "uihelpview.h"
#include "uirgbarray.h"
#include "odgraphicsitem.h"
#include "uigraphicsviewbase.h"
#include "uimainwin.h"

void Init_uiLocalFileSelToolProvider_Class();

static const FilePath& GetuiODPrintSupportFp()
{
    static FilePath fp;
    if ( fp.isEmpty() )
    {
	BufferString libnm; libnm.setMinBufSize( 32 );
	SharedLibAccess::getLibName( "uiODPrintSupport",
			libnm.getCStr(), libnm.bufSize() );
	fp.set( GetLibPlfDir() ).add( libnm );
    }

    return fp;
}

const char* sODPrintSupport = "uiODPrintSupport";

using boolFromQWidgetWithGeomFnm = bool(*)(QWidget&,const char*,int,int,int);
using boolFromQGraphicsSceneFn = bool(*)(QGraphicsScene&);
using boolFromQPaintDeviceFn = bool(*)(QPaintDevice*);

static boolFromQWidgetWithGeomFnm saveaspdffn_ = nullptr;
static boolFromQGraphicsSceneFn doprintdlgfn_ = nullptr;
static boolFromQPaintDeviceFn isqprinterfn_ = nullptr;

mGlobal(uiBase) void setGlobal_QPrintSupport_Fns(boolFromQWidgetWithGeomFnm,
			boolFromQGraphicsSceneFn,boolFromQPaintDeviceFn);
void setGlobal_QPrintSupport_Fns( boolFromQWidgetWithGeomFnm saveaspdffn,
				  boolFromQGraphicsSceneFn doprintdlgfn,
				  boolFromQPaintDeviceFn isqprinterfn )
{
    saveaspdffn_ = saveaspdffn;
    doprintdlgfn_ = doprintdlgfn;
    isqprinterfn_ = isqprinterfn;
}


bool uiMainWin::saveAsPDF( QWidget& qwin, const char* fnm,
			   int width, int height, int res )
{
    return saveaspdffn_ ? (*saveaspdffn_)( qwin, fnm, width, height, res )
			: false;
}


bool uiGraphicsViewBase::doPrintDialog( QGraphicsScene& qscene )
{
    return doprintdlgfn_ ? (*doprintdlgfn_)( qscene ) : false;
}


bool ODGraphicsDynamicImageItem::isQPrinter( QPaintDevice* qpaintdevice )
{
    return isqprinterfn_ ? (*isqprinterfn_)( qpaintdevice ) : false;
}


bool uiMainWin::hasPrintSupport()
{
    const FilePath& libfp = GetuiODPrintSupportFp();
    const PluginManager::Data* pidata = PIM().findData( libfp.fileName() );
    return pidata && pidata->isloaded_;
}


mDefModInitFn(uiBase)
{
    mIfNotFirstTime( return );

    uiButton::setHaveCommonPBIcons(
	    !Settings::common().isFalse("Ui.Icons.PushButtons") );

    uiCursorManager::initClass();
    FlareHelpProvider::initODHelp();
    VideoProvider::init();

    WebsiteHelp::initClass();
    ReleaseNotesProvider::initClass();
    uiRGBImageLoader::initClass();

    Init_uiLocalFileSelToolProvider_Class();
    uiFileSelToolProvider::addPluginFileSelProviders();

    if ( OD::InNormalRunContext() || OD::InStandAloneRunContext() )
    {
	const FilePath& libfp = GetuiODPrintSupportFp();
	if ( libfp.exists() )
	    PIM().load( libfp.fullPath(), PluginManager::Data::AppDir,
			PI_AUTO_INIT_LATE );
    }

    System::CrashDumper::getInstance().setSendAppl( "od_uiReportIssue" );
}
