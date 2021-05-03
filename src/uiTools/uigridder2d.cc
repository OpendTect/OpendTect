/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        K. Tingdahl
 Date:          March 2008
________________________________________________________________________

-*/

#include "uigridder2d.h"

#include "gridder2d.h"
#include "uigeninput.h"
#include "survinfo.h"

mImplFactory2Param( uiDlgGroup, uiParent*, Gridder2D*, uiGridder2DFact );


uiGridder2DSel::uiGridder2DSel( uiParent* p, const Gridder2D* g )
    : uiDlgGroup( p, tr("Gridding") )
    , original_( g )
{
    griddingparams_.allowNull();
    BufferStringSet griddernames = Gridder2D::factory().getNames();
    uiStringSet gridderusernames = Gridder2D::factory().getUserNames();

    for ( int idx=0; idx<griddernames.size(); idx++ )
    {
	Gridder2D* gridder = g && (*griddernames[idx])==g->factoryKeyword()
	    ? g->clone()
	    : Gridder2D::factory().create(griddernames[idx]->buf());

	if ( !gridder )
	{
	    griddernames.removeSingle( idx-- );
	    gridderusernames.removeSingle( idx-- );
	    continue;
	}

	gridders_ += gridder;
    }

    griddingsel_ = new uiGenInput( this, tr("Algorithm"),
	    			   StringListInpSpec( gridderusernames ) );
    griddingsel_->valuechanged.notify( mCB(this,uiGridder2DSel,selChangeCB) );

    for ( int idx=0; idx<griddernames.size(); idx++ )
    {
	uiDlgGroup* uigridder =
	    uiGridder2DFact().create( 0, this, gridders_[idx], false );

	griddingparams_ += uigridder;

	if ( uigridder )
	{
	    uigridder->attach( alignedBelow, griddingsel_ );
	    uigridder->display( false );
	}
    }

    int selidx = original_
	? griddernames.indexOf( original_->factoryKeyword() )
	: -1;

    if ( selidx<0 ) selidx=0;

    griddingsel_->setValue( selidx );
    setHAlignObj( griddingsel_ );
    selChangeCB( 0 );
}


uiGridder2DSel::~uiGridder2DSel()
{
    griddingsel_->valuechanged.remove( mCB(this,uiGridder2DSel,selChangeCB) );
    deepErase( gridders_ );
}


const Gridder2D* uiGridder2DSel::getSel()
{
    const int selidx = griddingsel_->getIntValue();
    if ( griddingparams_[selidx] && !griddingparams_[selidx]->acceptOK() )
	return 0;

    return gridders_[selidx];
}


const char* uiGridder2DSel::errMsg() const
{
    const int selidx = griddingsel_->getIntValue();
    if ( griddingparams_[selidx] )
	return griddingparams_[selidx]->errMsg();

    return 0;
}


void uiGridder2DSel::selChangeCB( CallBacker* )
{
    for ( int idx=griddingparams_.size()-1; idx>=0; idx-- )
	if ( griddingparams_[idx] ) griddingparams_[idx]->display( false );

    const int selidx = griddingsel_->getIntValue();

    if ( griddingparams_[selidx] )
	griddingparams_[selidx]->display( true );
}



void uiInverseDistanceGridder2D::initClass()
{
    uiGridder2DFact().addCreator( uiInverseDistanceGridder2D::create );
}


uiDlgGroup* uiInverseDistanceGridder2D::create( uiParent* p, Gridder2D* g )
{
    mDynamicCastGet( InverseDistanceGridder2D*, idg, g );
    if ( !idg ) return 0;

    return new uiInverseDistanceGridder2D( p, *idg );
}


uiInverseDistanceGridder2D::uiInverseDistanceGridder2D ( uiParent* p,
						InverseDistanceGridder2D& idg )
    : uiDlgGroup( p, InverseDistanceGridder2D::sFactoryDisplayName() )
    , idg_( idg )
    , initialsearchradius_( idg.getSearchRadius() )
{
    uiString radius = tr("Search radius %1").arg(SI().getUiXYUnitString());
    searchradiusfld_ = new uiGenInput( this, radius, FloatInpSpec() );
    searchradiusfld_->setWithCheck();
    searchradiusfld_->setChecked( !mIsUdf(initialsearchradius_) );
    searchradiusfld_->setValue( initialsearchradius_ );
    setHAlignObj( searchradiusfld_ );
}


const char* uiInverseDistanceGridder2D::errMsg() const
{
    return InverseDistanceGridder2D::searchRadiusErrMsg().getOriginalString();
}
//the only thing that can go wrong


bool uiInverseDistanceGridder2D::acceptOK()
{
    const float searchradius = searchradiusfld_->isChecked()
		       ? searchradiusfld_->getFValue() : mUdf(float);
    if ( searchradius<=0 )
	return false;

    idg_.setSearchRadius( searchradius );
    return true;
}


bool uiInverseDistanceGridder2D::rejectOK()
{
    return revertChanges();
}


bool uiInverseDistanceGridder2D::revertChanges()
{
    idg_.setSearchRadius( initialsearchradius_ );
    return true;
}
