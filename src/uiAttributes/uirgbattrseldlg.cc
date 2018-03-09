/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Nanne Hemstra
 Date:          January 2016
________________________________________________________________________

-*/

#include "uirgbattrseldlg.h"

#include "uiattrsel.h"
#include "attribsel.h"


uiRGBAttrSelDlg::uiRGBAttrSelDlg( uiParent* p, const Attrib::DescSet& ds )
    : uiDialog(p,Setup(tr("Select RGB Attributes"),mNoDlgTitle,mTODOHelpKey))
{
    rfld_ = new uiAttrSel( this, ds, uiStrings::sRed() );
    rfld_->showSteeringData( true );

    gfld_ = new uiAttrSel( this, ds, uiStrings::sGreen() );
    gfld_->showSteeringData( true );
    gfld_->attach( alignedBelow, rfld_ );

    bfld_ = new uiAttrSel( this, ds, uiStrings::sBlue() );
    bfld_->showSteeringData( true );
    bfld_->attach( alignedBelow, gfld_ );

    tfld_ = new uiAttrSel( this, ds, tr("Alpha/Opacity") );
    tfld_->showSteeringData( true );
    tfld_->attach( alignedBelow, bfld_ );
}


uiRGBAttrSelDlg::~uiRGBAttrSelDlg()
{
}


void uiRGBAttrSelDlg::setSelSpec( const Attrib::SelSpecList& as )
{
    if ( as.size() != 4 )
	return;

    rfld_->setSelSpec( &as[0] );
    gfld_->setSelSpec( &as[1] );
    bfld_->setSelSpec( &as[2] );
    tfld_->setSelSpec( &as[3] );
}


void uiRGBAttrSelDlg::fillSelSpec( Attrib::SelSpecList& as ) const
{
    if ( as.size() != 4 )
	as.setSize( 4 );

    rfld_->fillSelSpec( as[0] );
    gfld_->fillSelSpec( as[1] );
    bfld_->fillSelSpec( as[2] );
    tfld_->fillSelSpec( as[3] );
}


bool uiRGBAttrSelDlg::acceptOK()
{
    return true;
}
