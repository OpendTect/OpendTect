/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : Bert
 * DATE     : Jan 2009
-*/

static const char* rcsID mUnusedVar = "$Id$";

#include "qtclss.h"
#include <QMainWindow>
#include <QPushButton>

QtClss::QtClss( QWidget* w )
{
    win_ = new QMainWindow( w );
    btn_ = new QPushButton( win_ );
    btn_->setText( "Hello World" );
}


QtClss::~QtClss()
{
    delete win_;
}


void QtClss::go()
{
    win_->show();
}
