/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        H. Huck
 Date:          July 2010
________________________________________________________________________

-*/
static const char* rcsID = "$Id: uitrcpositiondlg.cc,v 1.8 2011/06/15 04:28:21 cvsumesh Exp $";

#include "uitrcpositiondlg.h"

#include "bufstringset.h"
#include "ioobj.h"
#include "pickretriever.h"
#include "posinfo2d.h"
#include "position.h"
#include "seistrctr.h"
#include "survinfo.h"
#include "surv2dgeom.h"

#include "uicombobox.h"
#include "uimsg.h"
#include "uiseisioobjinfo.h"
#include "uispinbox.h"
#include "uitoolbutton.h"


uiTrcPositionDlg::uiTrcPositionDlg( uiParent* p, const CubeSampling& cs,
				    bool is2d, const MultiID& mid )
    : uiDialog( p, uiDialog::Setup("Attribute trace position",0,"101.1.7")
	   		     .modal(false) )
    , linesfld_( 0 )
    , trcnrfld_( 0 )
    , inlfld_( 0 )
    , crlfld_( 0 )
    , mid_( mid )
{
    if ( is2d )
    {
	BufferStringSet linenames;
	uiSeisIOObjInfo objinfo( mid );
	objinfo.ioObjInfo().getLineNames( linenames );
	BufferString str = "Compute attribute on line:";
	linesfld_ = new uiLabeledComboBox( this, str );
	for ( int idx=0; idx<linenames.size(); idx++ )
	    linesfld_->box()->addItem( linenames.get(idx) );

	linesfld_->box()->selectionChanged.notify(
					mCB(this,uiTrcPositionDlg,lineSel) );
	trcnrfld_ = new uiLabeledSpinBox( this, "at trace nr:" );
	trcnrfld_->attach( alignedBelow, linesfld_ );
	lineSel(0);
    }
    else
    {
	BufferString str = "Compute attribute at position:";
	inlfld_ = new uiLabeledSpinBox( this, str );
	crlfld_ = new uiSpinBox( this );
	crlfld_->attach( rightTo, inlfld_ );
	inlfld_->box()->setInterval( cs.hrg.inlRange() );
	inlfld_->box()->setValue( cs.hrg.inlRange().snappedCenter() );
	crlfld_->setInterval( cs.hrg.crlRange() );
	crlfld_->setValue( cs.hrg.crlRange().snappedCenter() );
    }

    getposbut_ = new uiToolButton( this, "pick.png", "Point in 3D scene",
				   mCB(this,uiTrcPositionDlg,getPosCB) );
    if ( trcnrfld_ )
	getposbut_->attach( rightOf, trcnrfld_ );
    else
	getposbut_->attach( rightOf, crlfld_ );

    pickretriever_ = PickRetriever::getInstance();
    pickretriever_->finished()->notify(
	    			mCB(this,uiTrcPositionDlg,pickRetrievedCB) );
    zrg_.setFrom( cs.zrg );
}


uiTrcPositionDlg::~uiTrcPositionDlg()
{
    pickretriever_->finished()->remove(
	    			mCB(this,uiTrcPositionDlg,pickRetrievedCB) );
}


void uiTrcPositionDlg::getPosCB( CallBacker* )
{
    pickretriever_->enable( 0 );
    getposbut_->setSensitive( false );
}


bool uiTrcPositionDlg::getSelLineGeom( PosInfo::Line2DData& l2ddata )
{
    uiSeisIOObjInfo objinfo( mid_ );
    const IOObj* obj = objinfo.ioObj();
    if ( !obj ) return false;

    S2DPOS().setCurLineSet( obj->name() );
    const char* sellnm = linesfld_->box()->text();
    l2ddata.setLineName( sellnm );
    return S2DPOS().getGeometry( l2ddata ) ? true : false;
}


void uiTrcPositionDlg::pickRetrievedCB( CallBacker* )
{
    getposbut_->setSensitive( true );
    const Coord3 crd = pickretriever_->getPos();
    if ( !pickretriever_->success() )
	return;

    if ( trcnrfld_ )
    {
	PosInfo::Line2DData line2d;
	if ( !getSelLineGeom( line2d ) )
	    return;

	PosInfo::Line2DPos l2dpos;
	const float dist = SI().crlDistance();
	if ( !line2d.getPos( crd,  l2dpos, dist*dist ) )
	{
	    BufferString msg( "Please pick trace on line:",
		    	      linesfld_->box()->text() );
	    uiMSG().message( msg );
	    return;
	}

	trcnrfld_->box()->setValue( l2dpos.nr_ );
    }
    else
    {
	const BinID bid = SI().transform( crd );
	inlfld_->box()->setValue( bid.inl );
	crlfld_->setValue( bid.crl );
    }
}


LineKey uiTrcPositionDlg::getLineKey() const
{
    LineKey lk;
    if ( !linesfld_ ) return lk;

    lk.setLineName( linesfld_->box()->text() );
    return lk;
}


CubeSampling uiTrcPositionDlg::getCubeSampling() const
{
    CubeSampling cs;
    if ( trcnrfld_ )
    {
	int trcnr = trcnrfld_->box()->getValue();
	cs.hrg.set( cs.hrg.inlRange(), StepInterval<int>( trcnr, trcnr, 1 ) );
    }
    else
    {
	int inlnr = inlfld_->box()->getValue();
	int crlnr = crlfld_->getValue();
	cs.hrg.set( StepInterval<int>( inlnr, inlnr, 1 ),
		    StepInterval<int>( crlnr, crlnr, 1 ) );
    }

    cs.zrg = zrg_;
    return cs;
}


void uiTrcPositionDlg::lineSel( CallBacker* cb )
{
    PosInfo::Line2DData line2d;
    if ( !getSelLineGeom( line2d ) )
	return;

    trcnrfld_->box()->setInterval( line2d.trcNrRange() );
    trcnrfld_->box()->setValue( line2d.trcNrRange().snappedCenter() );
}
