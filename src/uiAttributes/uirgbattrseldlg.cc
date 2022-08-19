/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uirgbattrseldlg.h"

#include "uiattrsel.h"
#include "attribsel.h"


uiRGBAttrSelDlg::uiRGBAttrSelDlg( uiParent* p, const Attrib::DescSet& ds,
				  Pos::GeomID geomid )
    : uiDialog(p,Setup(tr("Select RGB Attributes"),mNoDlgTitle,mTODOHelpKey))
{
    rfld_ = new uiAttrSel( this, ds );
    rfld_->showSteeringData( true );
    rfld_->setGeomID( geomid );
    rfld_->setLabelText( tr("Red Attribute") );

    gfld_ = new uiAttrSel( this, ds );
    gfld_->showSteeringData( true );
    gfld_->setGeomID( geomid );
    gfld_->setLabelText( tr("Green Attribute") );
    gfld_->attach( alignedBelow, rfld_ );

    bfld_ = new uiAttrSel( this, ds );
    bfld_->showSteeringData( true );
    bfld_->setGeomID( geomid );
    bfld_->setLabelText( tr("Blue Attribute") );
    bfld_->attach( alignedBelow, gfld_ );

    tfld_ = new uiAttrSel( this, ds );
    tfld_->showSteeringData( true );
    tfld_->setGeomID( geomid );
    tfld_->setLabelText( tr("Alpha Attribute") );
    tfld_->attach( alignedBelow, bfld_ );
}


uiRGBAttrSelDlg::~uiRGBAttrSelDlg()
{
}


void uiRGBAttrSelDlg::setSelSpec( const TypeSet<Attrib::SelSpec>& as )
{
    if ( as.size() != 4 )
	return;

    rfld_->setSelSpec( &as[0] );
    gfld_->setSelSpec( &as[1] );
    bfld_->setSelSpec( &as[2] );
    tfld_->setSelSpec( &as[3] );
}


void uiRGBAttrSelDlg::fillSelSpec( TypeSet<Attrib::SelSpec>& as ) const
{
    if ( as.size() != 4 )
	as.setSize( 4 );

    rfld_->fillSelSpec( as[0] );
    gfld_->fillSelSpec( as[1] );
    bfld_->fillSelSpec( as[2] );
    tfld_->fillSelSpec( as[3] );
}


bool uiRGBAttrSelDlg::acceptOK( CallBacker* )
{
    return true;
}
