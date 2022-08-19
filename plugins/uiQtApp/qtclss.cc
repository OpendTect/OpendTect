/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

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
