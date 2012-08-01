/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          Feb 2010
________________________________________________________________________

-*/

static const char* rcsID mUnusedVar = "$Id: uieditpdf.cc,v 1.27 2012-08-01 14:44:26 cvshelene Exp $";

#include "uieditpdf.h"

#include "uiaxishandler.h"
#include "uitoolbutton.h"
#include "uibuttongroup.h"
#include "uimsg.h"
#include "uiflatviewmainwin.h"
#include "uiflatviewer.h"
#include "uifunctiondisplay.h"
#include "uigeninput.h"
#include "uitabstack.h"
#include "uitable.h"

#include "arrayndsmoother.h"
#include "flatposdata.h"
#include "sampledprobdenfunc.h"

#define mDeclArrNDPDF	mDynamicCastGet(ArrayNDProbDenFunc*,andpdf,&pdf_)
#define mDeclSzVars mDeclArrNDPDF; \
    const int nrtbls mUnusedVar = nrdims_ > 2 ? andpdf->size(2) : 1; \
    const int nrcols mUnusedVar = nrdims_ < 2 ? 1 : andpdf->size( 0 ); \
    const int nrrows mUnusedVar = andpdf->size( nrdims_ < 2 ? 0 : 1 )
#define mDeclIdxs	int idxs[3]; idxs[2] = curdim2_
#define mGetRowIdx(irow) \
    const int rowidx = nrdims_ == 1 ? irow : nrrows -irow - 1
#define mAddDim2Str(bs) \
	    bs.add( " at " ).add( pdf_.dimName(2) ).add( "=" ) \
		      .add( andpdf->sampling(2).atIndex(curdim2_) );


uiEditProbDenFunc::uiEditProbDenFunc( uiParent* p, ProbDenFunc& pdf, bool ed )
    : uiDialog(p,uiDialog::Setup(
	BufferString( ed ? "Edit" : "Browse"," Probability Density Function"),
	BufferString( ed ? "Edit '" : "Browse '",
		      pdf.name().isEmpty() ? "PDF" : pdf.name().buf(),
		      "'"),
	"112.1.1"))
    , inpdf_(pdf)
    , editable_(ed)
    , chgd_(false)
    , vwwin1d_(0)
    , vwwinnd_(0)
    , pdf_(*pdf.clone())
    , tbl_(0)
    , nrdims_(pdf.nrDims())
    , curdim2_(0)
{
    if ( !ed )
	setCtrlStyle( uiDialog::LeaveOnly );
    tabstack_ = new uiTabStack( this, "Tabs" );
    mDeclArrNDPDF;
    uiGroup* dimnmgrp = new uiGroup( tabstack_->tabGroup(), "Dimension names" );
    for ( int idim=0; idim<nrdims_; idim++ )
    {
	BufferString txt;
	if ( nrdims_ > 1 )
	    txt.add( "Name of dimension " ).add( idim + 1 );
	else
	    txt = "Variable name";
	uiGenInput* nmfld = new uiGenInput( dimnmgrp, txt, pdf_.dimName(idim) );
	if ( idim )
	    nmfld->attach( alignedBelow, nmflds_[idim-1] );
	nmflds_ += nmfld;
	if ( !editable_ )
	    nmfld->setReadOnly( true );
    }
    tabstack_->addTab( dimnmgrp, nrdims_ < 2 ? "Name" : "Names" );

    if ( !andpdf || nrdims_ > 3 )
	return;

    uiGroup* grp = new uiGroup( tabstack_->tabGroup(), "Values group" );
    mkTable( grp );
    tabstack_->addTab( grp, "Values" );
    tabstack_->selChange().notify( mCB(this,uiEditProbDenFunc,tabChg) );
    putValsToScreen();
}


void uiEditProbDenFunc::mkTable( uiGroup* grp )
{
    mDeclSzVars;

    uiTable::Setup su( nrrows, nrcols );
    su.coldesc( pdf_.dimName(0) )
      .rowdesc( nrdims_ > 1 ? pdf_.dimName(1) : "Values" )
      .fillrow(true).fillcol(true)
      .manualresize(true).sizesFixed(true);
    tbl_ = new uiTable( grp, su, "Values table" );

    if ( nrdims_ == 1 )
	tbl_->setColumnLabel( 0, "Value" );
    else
    {
	for ( int icol=0; icol<nrcols; icol++ )
	{
	    const float val = andpdf->sampling(0).atIndex(icol);
	    tbl_->setColumnLabel( icol, toString(val) );
	}
    }

    for ( int irow=0; irow<nrrows; irow++ )
    {
	mGetRowIdx(irow);
	const float rowval = andpdf->sampling(nrdims_<2?0:1).atIndex(rowidx);
	tbl_->setRowLabel( irow, toString(rowval) );
    }

    uiButtonGroup* bgrp = new uiButtonGroup( grp );
    new uiToolButton( bgrp, nrdims_ == 1 ? "distmap" : "viewprdf",
	    "View function", mCB(this,uiEditProbDenFunc,viewPDF) );
    if ( editable_ )
	new uiToolButton( bgrp, "smoothcurve", "Smooth values",
				mCB(this,uiEditProbDenFunc,smoothReq) );
    if ( nrdims_ > 2 )
    {
	const char* dim2nm = pdf_.dimName( 2 );
	new uiToolButton( bgrp, uiToolButton::RightArrow,
			    BufferString("Next ",dim2nm),
			    mCB(this,uiEditProbDenFunc,dimNext) );
	new uiToolButton( bgrp, uiToolButton::LeftArrow,
			    BufferString("Previous ",dim2nm),
			    mCB(this,uiEditProbDenFunc,dimPrev) );
    }
    bgrp->attach( rightOf, tbl_ );
}


uiEditProbDenFunc::~uiEditProbDenFunc()
{
    delete &pdf_;
}


#define mErrRet(s) { uiMSG().error(s); return false; }

bool uiEditProbDenFunc::getNamesFromScreen()
{
    BufferStringSet nms;
    for ( int idim=0; idim<pdf_.nrDims(); idim++ )
    {
	const BufferString newnm( nmflds_[idim]->text() );
	if ( newnm.isEmpty() )
	    mErrRet("Please enter all dimension names")
	if ( nms.indexOf(newnm) >= 0 )
	    mErrRet("No duplicate dimension names allowed")
	if ( newnm != pdf_.dimName(idim) )
	{
	    pdf_.setDimName( idim, nmflds_[idim]->text() );
	    chgd_ = true;
	}
    }

    return true;
}


void uiEditProbDenFunc::putValsToScreen()
{
    mDeclSzVars; mDeclIdxs;

    ArrayND<float>& data = andpdf->getData();
    for ( int irow=0; irow<nrrows; irow++ )
    {
	mGetRowIdx(irow);
	idxs[1] = nrdims_ == 1 ? 0 : rowidx;
	for ( int icol=0; icol<nrcols; icol++ )
	{
	    idxs[0] = nrdims_ == 1 ? rowidx : icol;
	    const float arrval = data.getND( idxs );
	    tbl_->setValue( RowCol(irow,icol), arrval );
	}
    }
}


bool uiEditProbDenFunc::getValsFromScreen( bool* chgd )
{
    mDeclSzVars; mDeclIdxs;

    ArrayND<float>& data = andpdf->getData();
    mDynamicCastGet(const ArrayNDProbDenFunc*,organdpdf,&inpdf_)
    const ArrayND<float>& orgdata = organdpdf->getData();
    for ( int irow=0; irow<nrrows; irow++ )
    {
	mGetRowIdx(irow);
	idxs[1] = nrdims_ == 1 ? 0 : rowidx;
	for ( int icol=0; icol<nrcols; icol++ )
	{
	    BufferString bstbltxt = tbl_->text( RowCol(irow,icol) );
	    char* tbltxt = bstbltxt.buf();
	    mTrimBlanks(tbltxt);
	    if ( !*tbltxt )
		mErrRet("Please fill all cells - or use 'Cancel'")

	    idxs[0] = nrdims_ == 1 ? rowidx : icol;
	    const float tblval = toFloat( tbltxt );
	    data.setND( idxs, tblval );
	    if ( chgd )
	    {
		const float orgarrval = orgdata.getND( idxs );
		if ( !mIsEqual(tblval,orgarrval,mDefEps) )
		    *chgd = true;
	    }
	}
    }

    return true;
}


class uiEditProbDenFunc2DDataPack : public FlatDataPack
{
public:
uiEditProbDenFunc2DDataPack( Array2D<float>* a2d, const ProbDenFunc& pdf )
    : FlatDataPack("Probability Density Function",a2d)
    , pdf_(pdf)
{
    setName( "Probability" );
}
const char* dimName( bool dim0 ) const
{
    return pdf_.dimName( dim0 ? 0 : 1 );
}

    const ProbDenFunc&	pdf_;
};

class uiPDF1DViewWin : public uiDialog
{
public:

uiPDF1DViewWin( uiParent* p, const float* xvals, const float* yvals, int sz )
    : uiDialog(p,uiDialog::Setup("1D PDF Viewer","","").modal(false) )
{
    setCtrlStyle( uiDialog::LeaveOnly );
    disp_ = new uiFunctionDisplay( this, uiFunctionDisplay::Setup() );
    disp_->setVals( xvals, yvals, sz );
}

    uiFunctionDisplay* 	disp_;

};


void uiEditProbDenFunc::viewPDF( CallBacker* )
{
    mDeclSzVars; mDeclIdxs;
    if ( !andpdf || !getValsFromScreen() ) return;

    const CallBack clsecb( mCB(this,uiEditProbDenFunc,vwWinClose) );
    const ArrayND<float>& data = andpdf->getData();
    if ( nrdims_ == 1 )
    {
	TypeSet<float> xvals;
	const int sz = data.info().getSize(0);
	for ( int idx=0; idx<sz; idx++ )
	    xvals += andpdf->sampling(0).atIndex(idx);

	if ( vwwin1d_ )
	    vwwin1d_->disp_->setVals( xvals.arr(), data.getData(), sz );
	else
	{
	    vwwin1d_ =
		new uiPDF1DViewWin( this, xvals.arr(), data.getData(), sz );
	    vwwin1d_->disp_->xAxis()->setName( pdf_.dimName(0) );
	    vwwin1d_->disp_->yAxis(false)->setName( "Value" );
	    vwwin1d_->setDeleteOnClose( true );
	    vwwin1d_->windowClosed.notify( clsecb );
	}
	vwwin1d_->show();
    }
    else
    {
	if ( !vwwinnd_ )
	{
	    uiFlatViewMainWin::Setup su( "Probability Density Function" );
	    su.nrstatusfields(0);
	    vwwinnd_ = new uiFlatViewMainWin( this, su );
	    vwwinnd_->setDarkBG( false );
	    vwwinnd_->setInitialSize( 300, 300 );
	    uiFlatViewer& vwr = vwwinnd_->viewer();
	    FlatView::Appearance& app = vwr.appearance();
	    app.ddpars_.show( false, true );
	    app.ddpars_.vd_.blocky_ = true;
	    app.ddpars_.vd_.mappersetup_.cliprate_ = Interval<float>(0,0);
	    FlatView::Annotation& ann = app.annot_;
	    ann.title_ = pdf_.name();
	    ann.setAxesAnnot( true );
	    ann.x1_.name_ = pdf_.dimName(0);
	    ann.x2_.name_ = pdf_.dimName(1);
	    vwwinnd_->windowClosed.notify( clsecb );
	}

	if ( nrdims_ > 2 )
	{
	    FlatView::Annotation& ann = vwwinnd_->viewer().appearance().annot_;
	    ann.title_ = pdf_.name();
	    mAddDim2Str( ann.title_ );
	}

	Array2D<float>* arr2d = new Array2DImpl<float>( nrcols, nrrows );
	for ( int irow=0; irow<nrrows; irow++ )
	{
	    mGetRowIdx(irow);
	    idxs[1] = nrdims_ == 1 ? 0 : rowidx;
	    for ( int icol=0; icol<nrcols; icol++ )
	    {
		idxs[0] = nrdims_ == 1 ? rowidx : icol;
		arr2d->set( idxs[0], idxs[1], data.getND(idxs) );
	    }
	}
	FlatDataPack* dp = new uiEditProbDenFunc2DDataPack( arr2d, pdf_ );

	SamplingData<float> sd( andpdf->sampling(0) );
	StepInterval<double> rg( sd.start,
				 sd.start + (andpdf->size(0)-1) * sd.step,
				 sd.step );
	dp->posData().setRange( true, rg );
	sd = SamplingData<float>( andpdf->sampling(1) );
	rg = StepInterval<double>( sd.start,
				   sd.start + (andpdf->size(1)-1) * sd.step,
				   sd.step );
	dp->posData().setRange( false, rg );
	DPM( DataPackMgr::FlatID() ).add( dp );

	vwwinnd_->viewer().setPack( false, dp->id(), false );
	vwwinnd_->start();
    }
}


void uiEditProbDenFunc::vwWinClose( CallBacker* )
{
    vwwinnd_ = 0; vwwin1d_ = 0;
}


void uiEditProbDenFunc::tabChg( CallBacker* )
{
    if ( tabstack_->currentPageId() != 1 )
	return;

    getNamesFromScreen();
    if ( tbl_ )
	setToolTips();
}


void uiEditProbDenFunc::setToolTips()
{
    mDeclSzVars;
#define mMkTT(dim) \
    BufferString tt( pdf_.dimName(dim) ); \
    if ( nrdims_ > 2 ) { mAddDim2Str(tt); } 

    if ( nrdims_ > 1 )
    {
	mMkTT(0)
	for ( int icol=0; icol<nrcols; icol++ )
	    tbl_->setColumnToolTip( icol, tt );
    }

    mMkTT(nrdims_ > 1 ? 1 : 0)
    for ( int irow=0; irow<nrrows; irow++ )
	tbl_->setRowToolTip( irow, tt );
}


void uiEditProbDenFunc::dimNext( CallBacker* )
{
    mDeclSzVars;
    if ( curdim2_ > nrtbls-2 || !getValsFromScreen() ) return;
    curdim2_++;
    updateUI();
    setToolTips();
}


void uiEditProbDenFunc::dimPrev( CallBacker* )
{
    if ( curdim2_ < 1 || !getValsFromScreen() ) return;
    curdim2_--;
    updateUI();
    setToolTips();
}


void uiEditProbDenFunc::smoothReq( CallBacker* )
{
    mDeclArrNDPDF; if ( !andpdf || !getValsFromScreen() ) return;

    ArrayND<float>* arrclone = andpdf->getArrClone();
    ArrayNDGentleSmoother<float> gs( *arrclone, andpdf->getData() );
    gs.execute();
    delete arrclone;

    updateUI();
}


void uiEditProbDenFunc::updateUI()
{
    putValsToScreen();
    if ( vwwinnd_ || vwwin1d_ )
	viewPDF( 0 );
}


bool uiEditProbDenFunc::acceptOK( CallBacker* )
{
    if ( !editable_ ) return true;

    if ( !getNamesFromScreen() )
	return false;
    if ( tbl_ && !getValsFromScreen(&chgd_) )
	return false;

    if ( chgd_ )
	const_cast<ProbDenFunc&>(inpdf_).copyFrom( pdf_ );

    return true;
}
