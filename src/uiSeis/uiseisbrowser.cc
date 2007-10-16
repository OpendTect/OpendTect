/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Sulochana/Satyaki
 Date:          Oct 2007
 RCS:           $Id: uiseisbrowser.cc,v 1.1 2007-10-16 16:27:58 cvssatyaki Exp $
________________________________________________________________________

-*/

#include "uiseisbrowser.h"
#include "uitoolbar.h"
#include "uilabel.h"
#include "uitable.h"
#include "seisread.h"
#include "ioman.h"


uiSeisBrowser::uiSeisBrowser( uiParent* p, const uiSeisBrowser::Setup& setup )
    : uiDialog( p, setup )
    , tbl_(0)
    , rdr_(0)
    , tb_(0)
{
    if ( !openData(setup) )
    {
	setTitleText( "Error" );
	BufferString lbltxt( "Cannot open input data (" );
	lbltxt += Seis::nameOf(setup.geom_); lbltxt += ")\n";
	lbltxt += IOM().nameOf( setup.id_ );
	if ( !setup.linekey_.isEmpty() )
	    { lbltxt += " - "; lbltxt += setup.linekey_; }
	new uiLabel( this, lbltxt );
	setCtrlStyle( LeaveOnly );
	return;
    }

    createMenuAndToolBar();
    createTable();

    setPos( setup.startpos_, setup.startz_ );
}


bool uiSeisBrowser::setPos( const BinID& bid, float z )
{
    if ( !tbl_ ) return false;
    return true;
}


bool uiSeisBrowser::openData( const uiSeisBrowser::Setup& setup )
{
    return true;
}

#define mAddButton(fnm,func,tip,toggle) \
    tb_->addButton( fnm, mCB(this,uiSeisBrowser,func), tip, toggle )

void uiSeisBrowser::createMenuAndToolBar()
{
    tb_ = new uiToolBar( this, "Tool Bar" );
    mAddButton("go-to.png",goToSelected,"",false );	   
}


void uiSeisBrowser::createTable()
{
}


void uiSeisBrowser::goToSelected( CallBacker* )
{
}
