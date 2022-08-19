/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uibuttongroup.h"
#include "uibutton.h"

#include <QButtonGroup>
#include <QAbstractButton>

mUseQtnamespace

uiButtonGroup::uiButtonGroup( uiParent* p, const char* nm,
			      OD::Orientation orient )
    : uiGroup( p ,nm )
    , orientation_(orient)
    , newrowcol_(false)
{
    qbuttongrp_ = new QButtonGroup();
}


uiButtonGroup::~uiButtonGroup()
{
    delete qbuttongrp_;
}


void uiButtonGroup::nextButtonOnNewRowCol()
{ newrowcol_ = true; }


int uiButtonGroup::addButton( uiButton* button )
{
    const bool vertical = orientation_ == OD::Vertical;
    const int id = qbuttongrp_->buttons().size();
    qbuttongrp_->addButton( button->qButton(), id );
    const bool firstbut = uibuts_.isEmpty();
    if ( firstbut )
    {
	uibuts_ += button;
	return id;
    }

    if ( newrowcol_ )
    {
	button->attach( vertical ? rightTo : leftAlignedBelow, uibuts_.first());
	newrowcol_ = false;
    }
    else
	button->attach( vertical ? leftAlignedBelow : rightTo,
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
