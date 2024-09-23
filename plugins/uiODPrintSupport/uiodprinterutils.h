#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uiodprintsupportmod.h"

#include "commondefs.h"

class QGraphicsScene;
class QPaintDevice;
class QWidget;

namespace OD
{

    mGlobal(uiODPrintSupport) bool SaveAsPDF(QWidget&,const char* fnm,
					     int width,int height,int res);
    mGlobal(uiODPrintSupport) bool DoPrintDialog(QGraphicsScene&);

    mGlobal(uiODPrintSupport) bool IsQPrinter(QPaintDevice*);

} // namespace OD
