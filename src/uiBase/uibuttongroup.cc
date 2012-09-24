/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        A.H. Lammertink
 Date:          18/08/2001
________________________________________________________________________

-*/
static const char* rcsID mUnusedVar = "$Id$";

#include "uibuttongroup.h"
#include "uibutton.h"

#include <QButtonGroup>
#include <QAbstractButton>

mUseQtnamespace

uiButtonGroup::uiButtonGroup( uiParent* p, const char* nm, bool vertical )
    : uiGroup( p ,nm )
    , vertical_(vertical)
{
    qbuttongrp_ = new QButtonGroup();
}


uiButtonGroup::~uiButtonGroup()
{
    delete qbuttongrp_;
}


int uiButtonGroup::addButton( uiButton* button )
{
    int id = qbuttongrp_->buttons().size();
    qbuttongrp_->addButton( button->qButton(), id );
    button->setStretch( grpobj_->width(), grpobj_->height() );
    if ( !uibuts_.isEmpty() )
	button->attach( vertical_ ? leftAlignedBelow : rightTo,
			uibuts_[uibuts_.size()-1] );
    uibuts_ += button;
    return id;
}


void uiButtonGroup::selectButton( int id )
{
    if ( qbuttongrp_->button( id ) )
	qbuttongrp_->button( id )->setChecked( true );
}


int uiButtonGroup::selectedId() const
{ return qbuttongrp_->checkedId(); }


int uiButtonGroup::nrButtons() const
{ return uibuts_.size(); }


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
