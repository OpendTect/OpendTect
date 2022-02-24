/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Wayne Mogg
 Date:		December 2021
________________________________________________________________________

-*/

#include "uifontselgrp.h"

#include "uicolor.h"
#include "uifont.h"
#include "uilabel.h"
#include "uitoolbutton.h"


uiFontSelGrp::uiFontSelGrp( uiParent* p, const uiString& lbl, bool withcol,
			    OD::Color defcol )
    : uiGroup(p,"Font selection")
    , changed(this)
{
    auto* lblfld = new uiLabel( this, lbl );

    fontbut_ = new uiToolButton( this, "font", tr("Font"),
				 mCB(this,uiFontSelGrp,fontChgCB) );
    fontbut_->attach( rightTo, lblfld );

    if ( withcol )
    {
	uiColorInput::Setup su( defcol );
	su.withdesc( false );
	colorfld_ = new uiColorInput( this, su );
	colorfld_->colorChanged.notify( mCB(this,uiFontSelGrp,colChgCB) );
	colorfld_->attach( rightTo, fontbut_ );
    }

    setHAlignObj( fontbut_ );
}


uiFontSelGrp::~uiFontSelGrp()
{
}


void uiFontSelGrp::setFont( const FontData& oth )
{
    fontdata_ = oth;
}


FontData uiFontSelGrp::getFont() const
{
    return fontdata_;
}


void uiFontSelGrp::setColor( const OD::Color& col )
{
    if ( colorfld_ )
	colorfld_->setColor( col );
}


OD::Color uiFontSelGrp::getColor() const
{
    return colorfld_ ? colorfld_->color() : OD::Color::Black();
}


void uiFontSelGrp::fontChgCB( CallBacker* )
{
    if ( !selectFont(fontdata_,this) )
	return;

    changed.trigger();
}


void uiFontSelGrp::colChgCB( CallBacker* )
{
    changed.trigger();
}


bool uiFontSelGrp::fillPar( IOPar& par ) const
{
    BufferString fontstr;
    fontdata_.putTo( fontstr );
    par.set( sKey::Font(), fontstr );
    if ( colorfld_ )
	par.set( sKey::Color(), colorfld_->color() );

    return true;
}


bool uiFontSelGrp::usePar( const IOPar& par )
{
    const FixedString fontstr = par.find( sKey::Font() );
    if ( fontstr )
	fontdata_.getFrom( fontstr );

    if ( fontdata_.pointSize()==0 )
	fontdata_.setPointSize( FontData::defaultPointSize() );

    if ( colorfld_ )
    {
	OD::Color col;
	if ( par.get(sKey::Color(),col) )
	    colorfld_->setColor( col );
    }

    return true;
}
