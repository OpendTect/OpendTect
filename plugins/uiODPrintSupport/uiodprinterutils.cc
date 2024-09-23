/*+
________________________________________________________________________

 Copyright:	(C) 1995-2024 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uiodprinterutils.h"

#include <QGraphicsScene>
#include <QPaintDevice>
#include <QPainter>
#include <QPrinter>
#include <QPrintDialog>


bool OD::SaveAsPDF( QWidget& qwin, const char* filename, int w, int h, int res )
{
    QString fileName( filename );
    QPrinter pdfprinter;
    pdfprinter.setOutputFormat( QPrinter::PdfFormat );
    const QPageSize pgsz( QSizeF(w,h), QPageSize::Point );
    pdfprinter.setPageSize( pgsz );
    pdfprinter.setFullPage( false );
    pdfprinter.setOutputFileName( filename );
    pdfprinter.setResolution( res );

    QPainter painter( &pdfprinter );
    const QRect qrec =
	pdfprinter.pageLayout().paintRectPixels( pdfprinter.resolution() );
    qwin.render( &painter, qrec.topLeft(), qwin.rect() );

    return true;
}


bool OD::DoPrintDialog( QGraphicsScene& qscene )
{
    QPrinter printer;
    QPrintDialog printdlg( &printer );
    if ( printdlg.exec() == QDialog::Rejected )
	return false;

    QPainter painter( &printer );
    painter.setRenderHint( QPainter::Antialiasing );
    qscene.render( &painter );

    return true;
}


bool OD::IsQPrinter( QPaintDevice* qpaintdevice )
{
    mDynamicCastGet(QPrinter*,qprinter,qpaintdevice)
    return qprinter;
}
