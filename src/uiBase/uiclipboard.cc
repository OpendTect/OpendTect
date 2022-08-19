/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uiclipboard.h"

#include "uistring.h"

#include <QApplication>
#include <QClipboard>

void uiClipboard::setText( const uiString& str )
{
    QClipboard* clipboard = QApplication::clipboard();
    clipboard->clear();
    QString qstr;
    str.fillQString( qstr );
    clipboard->setText( qstr );
}


void uiClipboard::setText( const char* txt )
{
    uiClipboard::setText( toUiString(txt) );
}


void uiClipboard::setImage( const QImage& img )
{
    QClipboard* clipboard = QApplication::clipboard();
    clipboard->clear();
    clipboard->setImage( img );
}
