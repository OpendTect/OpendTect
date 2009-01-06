/*+
 * COPYRIGHT: (C) dGB Beheer B.V.
 * AUTHOR   : Bert
 * DATE     : Jan 2009
-*/

static const char* rcsID = "$Id: qtclss.cc,v 1.1 2009-01-06 12:02:19 cvsbert Exp $";

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
