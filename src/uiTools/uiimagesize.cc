/*+
________________________________________________________________________

 Copyright:	dGB Beheer B.V.
 License:	https://dgbes.com/index.php/licensing
 Author:	Nanne Hemstra
 Date:		September 2021
________________________________________________________________________

-*/

#include "uiimagesize.h"

#include "uilabel.h"
#include "uispinbox.h"
#include "uistrings.h"


uiImageSize::uiImageSize( uiParent* p, const uiString& lbl, int maxnrdim )
    : uiGroup(p,"Image Size Group")
    , valueChanging(this)
{
    if ( maxnrdim < 1 )
	maxnrdim = 1;

    auto* uilbl = new uiLabel( this, lbl );
    for ( int idx=0; idx<maxnrdim; idx++ )
    {
	auto* fld = new uiSpinBox( this, 0, BufferString("Dim",idx) );
	mAttachCB( fld->valueChanging, uiImageSize::valueChangingCB );
	if ( idx==0 )
	    fld->attach( rightTo, uilbl );
	else
	    fld->attach( rightTo, sizeflds_[idx-1] );

	sizeflds_ += fld;
    }

    setHAlignObj( sizeflds_[0] );
}


uiImageSize::~uiImageSize()
{
    detachAllNotifiers();
}


void uiImageSize::valueChangingCB(CallBacker*)
{
    valueChanging.trigger();
}


int uiImageSize::maxNrDim() const
{
    return sizeflds_.size();
}


void uiImageSize::setNrDim( int nrdim )
{
    for ( int idx=0; idx<sizeflds_.size(); idx++ )
	sizeflds_[idx]->display( idx<nrdim );
}


int uiImageSize::currentNrDim() const
{
    int idx = 0;
    for ( auto* fld : sizeflds_ )
	if ( fld->isDisplayed() )
	    idx++;

    return idx;
}


void uiImageSize::setImageSize( int dim, int sz )
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


int uiImageSize::getImageSize( int dim ) const
{
    return sizeflds_.validIdx(dim) ? sizeflds_[dim]->getIntValue() : -1;
}


void uiImageSize::setSizeRange( int dim, const StepInterval<int>& rg )
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


void uiImageSize::setPrefix( int dim, const uiString& str )
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


void uiImageSize::setDefaultPrefixes()
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


void uiImageSize::setImageSize( std::array<int,2> sz )
{
    const int nrflds = std::min( sizeflds_.size(), int(sz.size()) );
    for ( int idx=0; idx<nrflds; idx++ )
	sizeflds_[idx]->setValue( sz[idx] );
}


void uiImageSize::setImageSize( std::array<int,3> sz )
{
    const int nrflds = std::min( sizeflds_.size(), int(sz.size()) );
    for ( int idx=0; idx<nrflds; idx++ )
	sizeflds_[idx]->setValue( sz[idx] );
}


std::array<int,2> uiImageSize::getImageSize2D() const
{
    std::array<int,2> sz;
    sz.fill( -1 );
    const int nrflds = std::min( sizeflds_.size(), 2 );
    for ( int idx=0; idx<nrflds; idx++ )
	sz[idx] = sizeflds_[idx]->getIntValue();

    return sz;
}


std::array<int,3> uiImageSize::getImageSize3D() const
{
    std::array<int,3> sz;
    sz.fill( -1 );
    const int nrflds = std::min( sizeflds_.size(), 3 );
    for ( int idx=0; idx<nrflds; idx++ )
	sz[idx] = sizeflds_[idx]->getIntValue();

    return sz;
}
