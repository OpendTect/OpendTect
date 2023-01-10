/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uisizesel.h"

#include "uibutton.h"
#include "uilabel.h"
#include "uispinbox.h"

#include "hiddenparam.h"

class HP_uiSizeSel
{
mOD_DisableCopy(HP_uiSizeSel)
public:
HP_uiSizeSel()   {}
~HP_uiSizeSel()  {}

    uiCheckBox*		symmfld_;
};

static HiddenParam<uiSizeSel,HP_uiSizeSel*> hp( nullptr );

uiSizeSel::uiSizeSel( uiParent* p, const uiString& lbl, int maxnrdim )
    : uiGroup(p,"Image Size Group")
    , valueChanging(this)
{
    if ( maxnrdim < 1 )
	maxnrdim = 1;

    auto* uilbl = new uiLabel( this, lbl );
    for ( int idx=0; idx<maxnrdim; idx++ )
    {
	auto* fld = new uiSpinBox( this, 0, BufferString("Dim",idx) );
	mAttachCB( fld->valueChanging, uiSizeSel::valueChangingCB );
	if ( idx==0 )
	    fld->attach( rightTo, uilbl );
	else
	    fld->attach( rightTo, sizeflds_[idx-1] );

	sizeflds_ += fld;
    }

    hp.setParam( this, new HP_uiSizeSel );
    auto* symmfld = new uiCheckBox( this, toUiString("Symmetric") );
    symmfld->setChecked( false );
    symmfld->attach( rightTo, sizeflds_.last() );
    hp.getParam(this)->symmfld_ = symmfld;

    setHAlignObj( sizeflds_[0] );
}


uiSizeSel::~uiSizeSel()
{
    detachAllNotifiers();
    hp.removeParam( this );
}


void uiSizeSel::valueChangingCB( CallBacker* cb )
{
    const bool dosymm = hp.getParam(this)->symmfld_->isChecked();
    if ( dosymm )
    {
	const int sz = sCast(uiSpinBox*,cb)->getIntValue();
	for ( auto* fld : sizeflds_ )
	{
	    if ( fld == cb )
		continue;

	    NotifyStopper ns( fld->valueChanged );
	    fld->setValue( sz );
	}
    }

    valueChanging.trigger();
}


int uiSizeSel::maxNrDim() const
{
    return sizeflds_.size();
}


void uiSizeSel::setNrDim( int nrdim )
{
    for ( int idx=0; idx<sizeflds_.size(); idx++ )
	sizeflds_[idx]->display( idx<nrdim );
}


int uiSizeSel::currentNrDim() const
{
    int idx = 0;
    for ( auto* fld : sizeflds_ )
	if ( fld->isDisplayed() )
	    idx++;

    return idx;
}


void uiSizeSel::setSymmetric( bool yn )
{
    hp.getParam(this)->symmfld_->setChecked( yn );
}


void uiSizeSel::setImageSize( int dim, int sz )
{
    if ( dim==-1 )
    {
	for ( auto* fld : sizeflds_ )
	    fld->setValue( sz );
	return;
    }

    if ( sizeflds_.validIdx(dim) )
	sizeflds_[dim]->setValue( sz );
}


int uiSizeSel::getImageSize( int dim ) const
{
    return sizeflds_.validIdx(dim) ? sizeflds_[dim]->getIntValue() : -1;
}


void uiSizeSel::setSizeRange( int dim, const StepInterval<int>& rg )
{
    if ( dim==-1 )
    {
	for ( auto* fld : sizeflds_ )
	    fld->setInterval( rg );
	return;
    }

    if ( sizeflds_.validIdx(dim) )
	sizeflds_[dim]->setInterval( rg );

}


void uiSizeSel::setPrefix( int dim, const uiString& str )
{
    if ( dim==-1 )
    {
	for ( auto* fld : sizeflds_ )
	    fld->setPrefix( str );
	return;
    }

    if ( sizeflds_.validIdx(dim) )
	sizeflds_[dim]->setPrefix( str );
}


void uiSizeSel::setDefaultPrefixes()
{
    const int nrdim = currentNrDim();
    if ( nrdim==1 )
	setPrefix( 0, toUiString("z:") );
    else if ( nrdim==2 )
    {
	setPrefix( 0, toUiString("nr:") );
	setPrefix( 1, toUiString("z:") );
    }
    else if ( nrdim==3 )
    {
	setPrefix( 0, toUiString("inl:") );
	setPrefix( 1, toUiString("crl:") );
	setPrefix( 2, toUiString("z:") );
    }
    else
	setPrefix( -1, uiString::emptyString() );
}


void uiSizeSel::setImageSize( std::array<int,2> sz )
{
    const int nrflds = std::min( sizeflds_.size(), int(sz.size()) );
    const bool issymmetric =  sz[0] == sz[1];
    setSymmetric( issymmetric );
    for ( int idx=0; idx<nrflds; idx++ )
	sizeflds_[idx]->setValue( sz[idx] );
}


void uiSizeSel::setImageSize( std::array<int,3> sz )
{
    const int nrflds = std::min( sizeflds_.size(), int(sz.size()) );
    const bool issymmetric =  sz[0] == sz[1] && sz[0] == sz[2];
    setSymmetric( issymmetric );
    for ( int idx=0; idx<nrflds; idx++ )
	sizeflds_[idx]->setValue( sz[idx] );
}


std::array<int,2> uiSizeSel::getImageSize2D() const
{
    std::array<int,2> sz;
    sz.fill( -1 );
    const int nrflds = std::min( sizeflds_.size(), 2 );
    for ( int idx=0; idx<nrflds; idx++ )
	sz[idx] = sizeflds_[idx]->getIntValue();

    return sz;
}


std::array<int,3> uiSizeSel::getImageSize3D() const
{
    std::array<int,3> sz;
    sz.fill( -1 );
    const int nrflds = std::min( sizeflds_.size(), 3 );
    for ( int idx=0; idx<nrflds; idx++ )
	sz[idx] = sizeflds_[idx]->getIntValue();

    return sz;
}
