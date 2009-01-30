/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        A.H. Lammertink
 Date:          18/08/2001
________________________________________________________________________

-*/
static const char* rcsID = "$Id: uibuttongroup.cc,v 1.18 2009-01-30 05:08:31 cvssatyaki Exp $";

#include "uibuttongroup.h"
#include "uibutton.h"

#include <QButtonGroup>
#include <QAbstractButton>


uiButtonGroup::uiButtonGroup( uiParent* p, const char* nm, bool vertical )
    : uiGroup( p ,nm )
    , prevbutton_(0)
    , vertical_(vertical)
{
    qbuttongrp_ = new QButtonGroup();
}


uiButtonGroup::~uiButtonGroup()
{
    delete qbuttongrp_;
}


void uiButtonGroup::addButton( uiButton* button )
{
    qbuttongrp_->addButton( button->qButton() );
    button->setStretch( grpobj_->width(), grpobj_->height() );
    if ( prevbutton_ )
	button->attach( vertical_ ? leftAlignedBelow : rightTo, prevbutton_ );
    prevbutton_ = button;
}


void uiButtonGroup::selectButton( int id )
{
    if ( qbuttongrp_->button( id ) )
	qbuttongrp_->button( id )->setDown( true );
}


int uiButtonGroup::selectedId() const
{ return qbuttongrp_->checkedId(); }


int uiButtonGroup::nrButtons() const
{ return qbuttongrp_->buttons().size(); }


void uiButtonGroup::setSensitive( int id, bool yn )
{
    QAbstractButton* but = qbuttongrp_->button( id );
    if ( but ) but->setEnabled( yn );
}


// TODO: implement displayFrame / isFrameDisplayed
void uiButtonGroup::displayFrame( bool yn )
{}


bool uiButtonGroup::isFrameDisplayed() const
{ return false; }


void uiButtonGroup::setExclusive( bool yn )
{ qbuttongrp_->setExclusive( yn ); }


bool uiButtonGroup::isExclusive() const
{ return qbuttongrp_->exclusive(); }
