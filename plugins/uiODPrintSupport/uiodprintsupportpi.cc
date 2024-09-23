/*+
________________________________________________________________________

 Copyright:	(C) 1995-2024 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uiodprintsupportmod.h"

#include "odplugin.h"
#include "uiodprinterutils.h"

using boolFromQWidgetWithGeomFnm = bool(*)(QWidget&,const char*,int,int,int);
using boolFromQGraphicsSceneFn = bool(*)(QGraphicsScene&);
using boolFromQPaintDeviceFn = bool(*)(QPaintDevice*);

mGlobal(uiBase) void setGlobal_QPrintSupport_Fns(boolFromQWidgetWithGeomFnm,
			boolFromQGraphicsSceneFn,boolFromQPaintDeviceFn);

mDefODPluginInfo(uiODPrintSupport)
{
    static PluginInfo retpi(
	    "Print support (GUI)",
	    "Implementations of Qt::PrintSupport for OpendTect" );

    return &retpi;
}


mDefODInitPlugin(uiODPrintSupport)
{
    setGlobal_QPrintSupport_Fns( OD::SaveAsPDF, OD::DoPrintDialog,
				 OD::IsQPrinter );
    return nullptr;
}
