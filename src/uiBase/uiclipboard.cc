/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : K. Tingdahl
 * DATE     : Nov 2015
 * FUNCTION : Clipboard functionality
-*/


#include "uiclipboard.h"

#include "uistring.h"
#include "uirgbarray.h"

#include <QApplication>
#include <QClipboard>


void uiClipboard::getText( BufferString& str )
{
     QClipboard* clipboard = QApplication::clipboard();
     str = BufferString( clipboard->text() );
}


void uiClipboard::getText( uiString& str )
{
     QClipboard* clipboard = QApplication::clipboard();
     str.setFrom( clipboard->text() );
}


void uiClipboard::setText( const uiString& str )
{
     QClipboard* clipboard = QApplication::clipboard();
     clipboard->clear();
     QString qstr;
     str.fillQString( qstr );
     clipboard->setText( qstr );
}


void uiClipboard::setImage( const QImage& img )
{
     QClipboard* clipboard = QApplication::clipboard();
     clipboard->clear();
     clipboard->setImage( img );
}


void uiClipboard::setImage( const OD::RGBImage& img )
{
    mDynamicCastGet(const uiRGBArray*,rgba, &img );
    if ( rgba )
    {
	setImage( rgba->qImage() );
    }
    else
    {
	uiRGBArray arr( img );
	setImage( arr.qImage() );
    }
}


QString uiClipboard::getText()
{
     return QApplication::clipboard()->text();
}
