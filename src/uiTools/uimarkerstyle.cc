/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        K. Tingdahl
 Date:          July 2010
________________________________________________________________________

-*/

#include "uimarkerstyle.h"

#include "uicolor.h"
#include "uibutton.h"
#include "uigeninput.h"
#include "uispinbox.h"
#include "uistrings.h"
#include "color.h"
#include "draw.h"


uiMarkerStyle::uiMarkerStyle( uiParent* p )
    : uiGroup(p)
    , typefld_(0)
    , sizefld_(0)
    , colorfld_(0)
    , change(this)
{
    mAttachCB( postFinalise(), uiMarkerStyle::initGrp );
}


uiMarkerStyle::~uiMarkerStyle()
{
    detachAllNotifiers();
}



void uiMarkerStyle::createFlds( const uiStringSet& typnms, const Setup& su )
{
    uiString lbltxt( su.lbltxt_ );
    if ( lbltxt.isEmpty() )
	lbltxt = tr("Marker style");
    else if ( lbltxt.getString() == "-" )
	lbltxt.setEmpty();

    uiObject* alignobj = nullptr;

    if ( su.wshape_ )
    {
	typefld_ = new uiGenInput( this, lbltxt,
					StringListInpSpec(typnms) );
	typefld_->setWithCheck();
	mAttachCB( typefld_->valuechanged, uiMarkerStyle::changeCB );
	mAttachCB( typefld_->checked, uiMarkerStyle::needmarkerCB );
	alignobj = typefld_->attachObj();
    }

    if ( su.wcolor_ )
    {
	uiColorInput::Setup csu( Color::White(), su.wtransparency_ ?
		uiColorInput::Setup::InSelector : uiColorInput::Setup::None );
	csu.lbltxt( typefld_ ? uiString::empty() : tr("Marker color") )
	   .withdesc(true);
	colorfld_ = new uiColorInput( this,  csu );
	mAttachCB( colorfld_->colorChanged, uiMarkerStyle::changeCB );
	if ( typefld_ )
	    colorfld_->attach( rightTo, typefld_ );
	else
	    alignobj = colorfld_->attachObj();

    }

    if ( su.wsz_ )
    {
	uiLabeledSpinBox* lsb = new uiLabeledSpinBox( this,
	    su.wcolor_||su.wshape_ ? uiStrings::sSize() : tr("Marker size") );
	sizefld_ = lsb->box();
	sizefld_->setMinValue( 1 );
	const OD::MarkerStyle3D marker3ddef;
	sizefld_->setValue( marker3ddef.size_ );
	mAttachCB( sizefld_->valueChanging, uiMarkerStyle::changeCB );
	if ( colorfld_ )
	    lsb->attach( rightTo, colorfld_ );
	else if ( typefld_ )
	    lsb->attach( rightTo, typefld_ );
	else
	    alignobj = lsb->attachObj();
    }

    setHAlignObj( alignobj );
}


void uiMarkerStyle::initGrp( CallBacker* )
{
    NotifyStopper nstype( typefld_->checked );
    typefld_->setChecked( true );
    needmarkerCB(nullptr);
}


void uiMarkerStyle::needmarkerCB( CallBacker* cb )
{
    const bool ischecked = typefld_->isChecked();
    if ( sizefld_ )
	sizefld_->setSensitive( ischecked );
    if ( colorfld_ )
	colorfld_->setSensitive( ischecked );
    changeCB( cb );
}


bool uiMarkerStyle::showMarker() const
{
    return typefld_ ? typefld_->isChecked() : true;
}


void uiMarkerStyle::setShowMarker( bool yn )
{
    if ( typefld_ )
    {
	typefld_->setChecked( yn );
	typefld_->setSensitive( yn );
    }
}


void uiMarkerStyle::changeCB( CallBacker* cb )
{
    change.trigger( cb );
}


void uiMarkerStyle::setColor( const Color& col )
{
    if ( colorfld_ )
	colorfld_->setColor( col );
}


Color uiMarkerStyle::getColor() const
{
    return colorfld_ ? colorfld_->color() : Color::White();
}


void uiMarkerStyle::setSize( int sz )
{
    if ( sizefld_ )
	sizefld_->setValue( sz );
}


int uiMarkerStyle::getSize() const
{
    return sizefld_ ? sizefld_->getIntValue() : 1;
}



// uiMarkerStyle2D

uiMarkerStyle2D::uiMarkerStyle2D( uiParent* p, const Setup& su,
				  const TypeSet<OD::MarkerStyle2D::Type>* excl )
    : uiMarkerStyle(p)
{
    const EnumDefImpl<OD::MarkerStyle2D::Type>& def
				= OD::MarkerStyle2D::TypeDef();
    uiStringSet nms;
    for ( int idx=0; idx<def.size(); idx++ )
    {
	const OD::MarkerStyle2D::Type mtyp = def.getEnumForIndex( idx );
	if ( !excl || !excl->isPresent(mtyp) )
	{
	    types_.add( mtyp );
	    nms.add( def.toUiString( mtyp ) );
	}
    }

    createFlds( nms, su );
}


void uiMarkerStyle2D::getMarkerStyle( OD::MarkerStyle2D& st ) const
{
    if ( typefld_ )
	st.type_ = getType();
    if ( sizefld_ )
	st.size_ = getSize();
    if ( colorfld_ )
	st.color_ = getColor();
}


void uiMarkerStyle2D::setType( OD::MarkerStyle2D::Type tp )
{
    if ( !typefld_ )
	return;

    int idx = types_.indexOf( tp );
    if ( idx<0 )
        idx = 0;

    typefld_->setValue( idx );
}


OD::MarkerStyle2D::Type uiMarkerStyle2D::getType() const
{
    const int idx = typefld_ ? typefld_->getIntValue() : 0;
    return types_.validIdx(idx) ? types_[idx] : OD::MarkerStyle2D::Circle;
}


void uiMarkerStyle2D::setMarkerStyle( const OD::MarkerStyle2D& st )
{
    setType( st.type_ );
    setSize( st.size_ );
    setColor( st.color_ );
}




//uiMarkerStyle3D


uiMarkerStyle3D::uiMarkerStyle3D( uiParent* p, const uiMarkerStyle::Setup& su,
				  const TypeSet<OD::MarkerStyle3D::Type>* excl )
    : uiMarkerStyle(p)
{
    const EnumDefImpl<OD::MarkerStyle3D::Type>& def
				= OD::MarkerStyle3D::TypeDef();
    uiStringSet nms;
    for ( int idx=0; idx<def.size(); idx++ )
    {
	const OD::MarkerStyle3D::Type mtyp = def.getEnumForIndex( idx );
	if ( !excl || !excl->isPresent(mtyp) )
	{
	    types_.add( mtyp );
	    nms.add( def.toUiString( mtyp ) );
	}
    }
    createFlds( nms, su );
}


void uiMarkerStyle3D::getMarkerStyle( OD::MarkerStyle3D& st ) const
{
    if ( typefld_ )
	st.type_ = getType();
    if ( sizefld_ )
	st.size_ = getSize();
    if ( colorfld_ )
	st.color_ = getColor();
}


void uiMarkerStyle3D::setType( OD::MarkerStyle3D::Type tp )
{
    if ( !typefld_ )
	return;

    int idx = types_.indexOf( tp );
    if ( idx<0 )
	idx = 0;

    typefld_->setValue( idx );
}


OD::MarkerStyle3D::Type uiMarkerStyle3D::getType() const
{
    const int idx = typefld_ ? typefld_->getIntValue() : 0;
    return types_.validIdx(idx) ? types_[idx] : OD::MarkerStyle3D::Sphere;
}


void uiMarkerStyle3D::setMarkerStyle( const OD::MarkerStyle3D& st )
{
    NotifyStopper ns( change );
    setType( st.type_ );
    setSize( st.size_ );
    setColor( st.color_ );
}
