/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          Jan 2018
________________________________________________________________________

-*/

#include "uithemesel.h"
#include "settings.h"
#include "dirlist.h"
#include "uimain.h"

#include "uicombobox.h"

namespace OD { mGlobal(Basic) bool setActiveStyleName(const char*); }


uiThemeSel::uiThemeSel( uiParent* p )
	: uiGroup(p,"Theme selector")
{
    OD::getStyleNames( themenames_ );
    const BufferString curstylenm = OD::getActiveStyleName();
    uiLabeledComboBox* lcb = new uiLabeledComboBox( this, tr("Theme"), "Theme");
    selfld_ = lcb->box();
    for ( int idx=0; idx<themenames_.size(); idx++ )
    {
	const BufferString& nm = themenames_.get( idx );
	uiString txt; BufferString icnm;
	if ( nm == "default" )
	{
	    txt = tr( "Default Theme" );
	    icnm = "od";
	}
	else
	{
	    BufferString capsfirststr( nm );
	    capsfirststr.toUpper( true );
	    const BufferString coolnm( "'", capsfirststr, "'" );
	    txt = toUiString( coolnm );
	    icnm = OD::getStyleFile( nm.str(), "png" );
	}
	selfld_->addItem( txt );
	selfld_->setIcon( idx, icnm );
    }

    int curstyle = themenames_.indexOf( OD::getActiveStyleName() );
    if ( curstyle < 0 )
	curstyle = themenames_.indexOf( "default" );
    selfld_->setCurrentItem( curstyle<0 ? 0 : curstyle );

    mAttachCB( selfld_->selectionChanged, uiThemeSel::themeSel );
    setHAlignObj( selfld_ );
}


void uiThemeSel::themeSel( CallBacker* )
{
    const int selidx = selfld_->currentItem();
    if ( selidx >= 0 )
	activateTheme( themenames_.get(selidx) );
}


bool uiThemeSel::putInSettings( bool write )
{
    const int selidx = selfld_->currentItem();
    if ( selidx < 0 )
	return false;

    return setODTheme( themenames_.get(selidx), write );
}


void uiThemeSel::activateTheme( const char* nm )
{
    const BufferString qssfnm = OD::getStyleFile( nm, "qss" );
    uiMain::theMain().setStyleSheet( qssfnm );
}


bool uiThemeSel::setODTheme( const char* nm, bool mkpermanent )
{
    bool rv = OD::setActiveStyleName( nm );
    if ( rv && mkpermanent )
    {
	activateTheme( nm );
	Settings::common().write();
    }
    return rv;
}
