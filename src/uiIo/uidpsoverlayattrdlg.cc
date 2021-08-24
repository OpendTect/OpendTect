/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Satyaki Maitra
 Date:          Feb 2010
________________________________________________________________________

-*/

#include "uidpsoverlayattrdlg.h"
#include "uidatapointsetcrossplot.h"

#include "uibutton.h"
#include "uicombobox.h"
#include "uicolortable.h"
#include "od_helpids.h"

uiDPSOverlayPropDlg::uiDPSOverlayPropDlg( uiParent* p,
					  uiDataPointSetCrossPlotter& pltr )
    : uiDialog(p,uiDialog::Setup(tr("Overlay Properties"),
				 tr("Display Properties within points"),
				 mODHelpKey(mDPSOverlayPropDlgHelpID) ))
    , plotter_(pltr)
{
    const DataPointSet& dps = plotter_.dps();
    uiDataPointSet::DColID dcid = -dps.nrFixedCols()+1;
    colids_ += mUdf(int);
    BufferStringSet colnames;
    colnames.add( "None" );
    for ( ; dcid<dps.nrCols(); dcid++ )
    {
	colids_ += dcid;
	colnames.add( userName(dcid) );
    }

    y3coltabfld_ =
	new uiColorTableGroup( this, plotter_.y3CtSeq(), OD::Horizontal, false);
    y3coltabfld_->enableManage( false );
    y3coltabfld_->setInterval( plotter_.y3Mapper().range() );
    uiLabeledComboBox* y3lblcbx =
	new uiLabeledComboBox( this, colnames, tr("Overlay Y1 Attribute"), "" );
    y3lblcbx->attach( alignedBelow, y3coltabfld_ );
    y3propselfld_ = y3lblcbx->box();
    if ( !mIsUdf(plotter_.y3Colid()) )
    {
	if ( colids_.indexOf(plotter_.y3Colid()) > 0 )
	    y3propselfld_->setCurrentItem( colids_.indexOf(plotter_.y3Colid()));
    }
    else
	y3propselfld_->setCurrentItem( 0 );
    y3coltabfld_->scaleChanged.notify(
	    mCB(this,uiDPSOverlayPropDlg,scaleChanged) );
    y3propselfld_->selectionChanged.notify(
	    mCB(this,uiDPSOverlayPropDlg,attribChanged) );


    uiLabeledComboBox* y4lblcbx = 0;
    if ( plotter_.isY2Shown() )
    {
	y4coltabfld_ = new uiColorTableGroup( this, plotter_.y4CtSeq(),
					      OD::Horizontal, false );
	y4coltabfld_->enableManage( false );
	y4coltabfld_->attach( alignedBelow, y3lblcbx );
	y4lblcbx =
	    new uiLabeledComboBox( this, colnames,
                                   tr("Overlay Y2 Attribute"), "");
	y4lblcbx->attach( alignedBelow, y4coltabfld_ );
	y4propselfld_ = y4lblcbx->box();
	y4coltabfld_->setInterval( plotter_.y4Mapper().range() );
	if ( !mIsUdf(plotter_.y4Colid()) )
	{
	    if ( colids_.indexOf(plotter_.y4Colid()) > 0 )
		y4propselfld_->setCurrentItem(
			colids_.indexOf(plotter_.y4Colid()) );
	}
	else
	    y4propselfld_->setCurrentItem( 0 );

	y4coltabfld_->scaleChanged.notify(
	    mCB(this,uiDPSOverlayPropDlg,scaleChanged) );
	y4propselfld_->selectionChanged.notify(
		mCB(this,uiDPSOverlayPropDlg,attribChanged) );
    }

    uiButton* applybut = uiButton::getStd( this, OD::Apply,
	    mCB(this,uiDPSOverlayPropDlg,doApply), true );
    applybut->attach( centeredBelow, plotter_.isY2Shown() ? y4lblcbx
							  : y3lblcbx );
}


const char* uiDPSOverlayPropDlg::userName( int did ) const
{
    if ( did >= 0 )
	return plotter_.dps().colName( did );
    else if ( did == -1 )
	return "Z";
    else
	return did == -3 ? "X-Coord" : "Y-Coord";
}


void uiDPSOverlayPropDlg::doApply( CallBacker* )
{ acceptOK(0); }


bool uiDPSOverlayPropDlg::acceptOK( CallBacker* )
{
    if ( y3propselfld_->currentItem() )
    {
	y3coltabfld_->commitInput();
	plotter_.setOverlayY1Cols( colids_[y3propselfld_->currentItem()] );
	plotter_.setOverlayY1AttSeq( y3coltabfld_->colTabSeq() );
	plotter_.setOverlayY1AttMapr( y3coltabfld_->colTabMapperSetup() );
	plotter_.updateOverlayMapper( true );
	plotter_.setShowY3( true );
    }
    else
    {
	plotter_.setOverlayY1Cols( mUdf(int) );
	plotter_.setShowY3( false );
    }

    if ( plotter_.isY2Shown() && y4propselfld_->currentItem() )
    {
	y4coltabfld_->commitInput();
	plotter_.setOverlayY2Cols( colids_[y4propselfld_->currentItem()] );
	plotter_.setOverlayY2AttSeq( y4coltabfld_->colTabSeq() );
	plotter_.setOverlayY2AttMapr( y4coltabfld_->colTabMapperSetup() );
	plotter_.updateOverlayMapper( false );
	plotter_.setShowY4( true );
    }
    else
    {
	plotter_.setOverlayY2Cols( mUdf(int) );
	if ( plotter_.isY2Shown() && y4propselfld_ )
	    y4coltabfld_->setInterval( Interval<float>(0,1) );

	plotter_.setShowY4( false );
    }

    plotter_.drawContent();
    plotter_.reDrawSelections();
    return true;
}


void uiDPSOverlayPropDlg::attribChanged( CallBacker* )
{
    if ( y3propselfld_->currentItem() )
    {
	ColTab::MapperSetup prevmsu = y3coltabfld_->colTabMapperSetup();
	prevmsu.type_ = ColTab::MapperSetup::Auto;
	prevmsu.cliprate_ = Interval<float>(0.f,0.f);
	y3coltabfld_->setMapperSetup( &prevmsu, false );
	plotter_.setOverlayY1Cols( colids_[y3propselfld_->currentItem()] );
	plotter_.setOverlayY1AttMapr( y3coltabfld_->colTabMapperSetup() );
	plotter_.updateOverlayMapper( true );
	y3coltabfld_->setInterval( plotter_.y3Mapper().range() );
    }
    else
    {
	plotter_.setOverlayY1Cols( mUdf(int) );
	y3coltabfld_->setInterval( Interval<float>(0,1) );
	plotter_.setShowY3( false );
    }

    if ( plotter_.isY2Shown() && y4propselfld_->currentItem() )
    {
	ColTab::MapperSetup prevmsu = y4coltabfld_->colTabMapperSetup();
	prevmsu.type_ = ColTab::MapperSetup::Auto;
	prevmsu.cliprate_ = Interval<float>(0.f,0.f);
	y4coltabfld_->setMapperSetup( &prevmsu, false );
	plotter_.setOverlayY2Cols( colids_[y4propselfld_->currentItem()] );
	plotter_.setOverlayY2AttMapr( y4coltabfld_->colTabMapperSetup() );
	plotter_.updateOverlayMapper( false );
	y4coltabfld_->setInterval( plotter_.y4Mapper().range() );
    }
    else
    {
	plotter_.setOverlayY2Cols( mUdf(int) );
	if ( plotter_.isY2Shown() && y4propselfld_ )
	{
	    y4coltabfld_->setInterval( Interval<float>(0,1) );
	    plotter_.setShowY4( false );
	}
    }
}


void uiDPSOverlayPropDlg::scaleChanged( CallBacker* )
{
    ColTab::MapperSetup mappersetup;
    if ( y3propselfld_->currentItem() )
	plotter_.setOverlayY1AttMapr( y3coltabfld_->colTabMapperSetup() );
    else
    {
	plotter_.setOverlayY1Cols( mUdf(int) );
	y3coltabfld_->setInterval( Interval<float>(0,1) );
	plotter_.setShowY3( false );
    }

    if ( plotter_.isY2Shown() && y4propselfld_->currentItem() )
	plotter_.setOverlayY2AttMapr( y4coltabfld_->colTabMapperSetup() );
    else
    {
	plotter_.setOverlayY2Cols( mUdf(int) );
	if ( plotter_.isY2Shown() && y4propselfld_ )
	{
	    y4coltabfld_->setInterval( Interval<float>(0,1) );
	    plotter_.setShowY4( false );
	}
    }
}
