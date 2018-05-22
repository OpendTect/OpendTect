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


uiThemeSel::uiThemeSel( uiParent* p, bool withlabel )
    : uiGroup(p,"Theme selector")
{
    if ( withlabel )
    {
	uiLabeledComboBox* lcb = new uiLabeledComboBox( this,
						uiStrings::sTheme() );
	selfld_ = lcb->box();
	setHAlignObj( lcb );
    }
    else
    {
	selfld_ = new uiComboBox( this, "Theme" );
	setHAlignObj( selfld_ );
    }

    OD::getStyleNames( themenames_ );

    for ( int idx=0; idx<themenames_.size(); idx++ )
    {
	const BufferString& nm = themenames_.get( idx );
	uiString txt; BufferString icnm;
	if ( nm == "default" )
	{
	    txt = tr("Default Theme");
	    icnm = "od";
	}
	else if ( nm == "pro" )
	{
	    txt = tr("OpendTect Pro Theme");
	    icnm = "dgbpro";
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

    themenameatentry_ = OD::getActiveStyleName();
    int curstyle = themenames_.indexOf( themenameatentry_ );
    if ( curstyle < 0 )
    {
	themenameatentry_.set( "default" );
	curstyle = themenames_.indexOf( themenameatentry_ );
    }
    selfld_->setCurrentItem( curstyle<0 ? 0 : curstyle );

    mAttachCB( selfld_->selectionChanged, uiThemeSel::themeSel );
    selfld_->setToolTip( tr("Select a theme") );
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


void uiThemeSel::revert()
{
    activateTheme( themenameatentry_ );
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
