/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Wayne Mogg
 Date:		December 2021
________________________________________________________________________

-*/

#include "uifontselgrp.h"

#include "draw.h"
#include "uibutton.h"
#include "uifont.h"
#include "uigeninput.h"
#include "uilabel.h"


uiFontSelGrp::uiFontSelGrp( uiParent* p, const uiString& lbl, bool withpos )
    : uiGroup(p,"Font selection")
    , withpos_(withpos)
    , changed(this)
{
    auto* lblfld = new uiLabel( this, lbl );

    fontbut_ = new uiPushButton( this, tr("Font"), false );
    fontbut_->setIcon( "font" );
    fontbut_->attach( rightTo, lblfld );
    mAttachCB( fontbut_->activated, uiFontSelGrp::fontChgCB );

    if ( withpos_ )
    {
	halignfld_ = new uiGenInput( this, uiStrings::phrJoinStrings(
			     uiStrings::sHorizontal(),uiStrings::sAlignment()),
				     StringListInpSpec(Alignment::HPosDef()) );
	halignfld_->attach( rightTo, fontbut_ );
	mAttachCB( halignfld_->valuechanged, uiFontSelGrp::propertyChgCB );

	valignfld_ = new uiGenInput( this, uiStrings::phrJoinStrings(
			     uiStrings::sVertical(),uiStrings::sAlignment()),
				     StringListInpSpec(Alignment::VPosDef()) );
	valignfld_->attach( alignedBelow, halignfld_ );
	mAttachCB( valignfld_->valuechanged, uiFontSelGrp::propertyChgCB );

	PositionInpSpec offspec( PositionInpSpec::Setup(true,false,false) );
	offsetfld_ = new uiGenInput( this, uiStrings::phrJoinStrings(
						tr("Text"),
						uiStrings::sOffset(),
						tr("(X/Y)")),
				     offspec.setName("X",0).setName("Y",0) );
	offsetfld_->attach( alignedBelow, valignfld_ );
	mAttachCB( offsetfld_->valuechanged, uiFontSelGrp::propertyChgCB );
    }
    setHAlignObj( fontbut_ );
}


uiFontSelGrp::~uiFontSelGrp()
{
    detachAllNotifiers();
}


void uiFontSelGrp::setFont( const FontData& oth )
{
    fontdata_ = oth;
}


FontData uiFontSelGrp::getFont() const
{
    return fontdata_;
}


void uiFontSelGrp::setAlignment( const Alignment& all )
{
    if ( withpos_ )
    {
	halignfld_->setValue( all.hPos() );
	valignfld_->setValue( all.vPos() );
    }
}


Alignment uiFontSelGrp::getAlignment() const
{
    const auto& hdef = Alignment::HPosDef();
    const auto& vdef = Alignment::VPosDef();
    return withpos_ ?
	    Alignment(hdef.getEnumForIndex(halignfld_->getIntValue()),
		      vdef.getEnumForIndex(valignfld_->getIntValue())) :
	    Alignment();
}


Coord uiFontSelGrp::getTextOffset() const
{
    return offsetfld_ ? offsetfld_->getCoord() : Coord(0.,0.);
}


void uiFontSelGrp::setTextOffset( Coord off )
{
    if ( offsetfld_ )
    {
	offsetfld_->setValue( off );
    }
}


void uiFontSelGrp::fontChgCB( CallBacker* )
{
    if ( !selectFont(fontdata_, this) )
	return;

    changed.trigger();
}


void uiFontSelGrp::propertyChgCB( CallBacker* )
{
    changed.trigger();
}


bool uiFontSelGrp::fillPar( IOPar& par ) const
{
    BufferString fontstr;
    fontdata_.putTo( fontstr );
    par.set(sKey::Font(), fontstr );

    par.set(sKey::Alignment(), getAlignment().uiValue() );
    BufferString offsetstr( getTextOffset().toString() );
    par.set(sKey::Offset(), offsetstr );

    return true;
}


bool uiFontSelGrp::usePar( const IOPar& par )
{
    const FixedString fontstr = par.find( sKey::Font() );
    if ( fontstr )
	fontdata_.getFrom( fontstr );

    if ( fontdata_.pointSize()==0 )
	fontdata_.setPointSize( FontData::defaultPointSize() );

    int align = Alignment().uiValue();
    par.get( sKey::Alignment(), align );
    Alignment all;
    all.setUiValue( align );
    setAlignment( all );

    const FixedString offsetstr = par.find( sKey::Offset() );
    Coord offset( 0., 0. );
    if ( offsetstr )
	offset.fromString( offsetstr );

    setTextOffset( offset );

    return true;
}
