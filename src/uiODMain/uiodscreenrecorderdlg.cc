/** file@  src/uiODMain/uiodscreenrecorderdlg.cc */

/*+
________________________________________________________________________

 Copyright:	(C) 1995-2026 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uiodscreenrecorderdlg.h"

#include "uilabel.h"
#include "uiodmain.h"

#include <QGuiApplication>
#include <QScreen>
#include <QLibraryInfo>



uiODScreenRecorderDlg::uiODScreenRecorderDlg( uiODMain& appl )
    : uiDialog( &appl, Setup(tr("Screen Recorder"),mNoHelpKey)
				.modal(false) )
{
    setCtrlStyle( CloseOnly );
    setDeleteOnClose( false );

    new uiLabel( this,
	tr("Screen recorder dialog wiring is operational.\n"
	   "Active Qt platform plugin: %1")
	.arg(toUiString(QGuiApplication::platformName())) );

    for (const char *name : {
             "XDG_SESSION_TYPE",
             "XDG_CURRENT_DESKTOP",
             "DISPLAY",
             "WAYLAND_DISPLAY",
             "QT_QPA_PLATFORM",
             "QT_PLUGIN_PATH",
             "QT_MEDIA_BACKEND"
         }) {
        qInfo().noquote() << name << "=" << environmentQt(name);
    }

    qInfo().noquote() << "Qt compile-time version:" << QT_VERSION_STR;
    qInfo().noquote() << "Qt runtime version:     " << qVersion();
    qInfo().noquote() << "Qt installation prefix: "
                      << QLibraryInfo::path(QLibraryInfo::PrefixPath);
    qInfo().noquote() << "Qt plugin directory:    "
                      << QLibraryInfo::path(QLibraryInfo::PluginsPath);
    qInfo().noquote() << "Qt platform plugin:     "
                      << QGuiApplication::platformName();
    
    
     for (QScreen *screen : QGuiApplication::screens()) 
     {
        qInfo().noquote()
            << "Screen:" << screen->name()
            << "geometry:" << screen->geometry()
            << "DPR:" << screen->devicePixelRatio()
            << "refresh rate:" << screen->refreshRate();
    }

}


uiODScreenRecorderDlg::~uiODScreenRecorderDlg()
{
}


QString uiODScreenRecorderDlg::environmentQt( const char* name ) const
{
    return qEnvironmentVariableIsSet(name)
        ? qEnvironmentVariable(name)
        : QStringLiteral("<unset>");

    /*const QByteArray val = qgetenv(name);
    return val.isEmpty() ? tr("<not set>") : toUiString(val.constData());*/
}


/*uiODScreenRecorderDlg::uiODScreenRecorderDlg( const uiODScreenRecorderDlg& )
    : uiDialog( nullptr, Setup(tr("Screen Recorder"),mNoHelpKey)
                .modal(false) )
{
    setCtrlStyle( CloseOnly );
    setDeleteOnClose( false )
}*/
