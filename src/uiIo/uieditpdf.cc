/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          Feb 2010
________________________________________________________________________

-*/

static const char* rcsID = "$Id: uieditpdf.cc,v 1.10 2010-03-05 11:32:00 cvssatyaki Exp $";

#include "uieditpdf.h"

#include "uiaxishandler.h"
#include "uibutton.h"
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
#include "pixmap.h"
#include "sampledprobdenfunc.h"

#define mDeclArrNDPDF mDynamicCastGet(ArrayNDProbDenFunc*,andpdf,workpdf_)
#define mDeclNrDims \
    const int nrdims = pdf_.nrDims()
#define mDeclSzVars \
    const int nrtbltabs = nrdims > 2 ? andpdf->size(2) : 1; \
    const int nrcols = nrdims < 2 ? 1 : andpdf->size( 0 ); \
    const int nrrows = andpdf->size( nrdims < 2 ? 0 : 1 )
#define mDeclIdxs int idxs[3]; idxs[2] = curdim2_
#define mGetRowIdx(irow) \
    const int rowidx = nrdims == 1 ? irow : nrrows -irow - 1


uiEditProbDenFunc::uiEditProbDenFunc( uiParent* p, ProbDenFunc& pdf, bool ed )
    : uiDialog(p,uiDialog::Setup(
	BufferString( ed ? "Edit" : "Browse"," Probability Density Function"),
	BufferString( ed ? "Edit '" : "Browse '",
		      pdf.name().isEmpty() ? "PDF" : pdf.name().buf(),
		      "'"),
	mTODOHelpID))
    , pdf_(pdf)
    , editable_(ed)
    , chgd_(false)
    , flatvwwin_(0)
    , workpdf_(0)
    , tbl_(0)
    , curdim2_(0)
{
    mDeclNrDims;
    tabstack_ = new uiTabStack( this, "Tabs" );

    uiGroup* dimnmgrp = new uiGroup( tabstack_->tabGroup(), "Dimension names" );
    for ( int idim=0; idim<nrdims; idim++ )
    {
	BufferString txt;
	if ( nrdims > 1 )
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
    tabstack_->addTab( dimnmgrp, nrdims < 2 ? "Name" : "Names" );

    mDynamicCastGet(const ArrayNDProbDenFunc*,andpdf,&pdf_)
    if ( !andpdf || nrdims > 3 )
	return;
    workpdf_ = pdf_.clone();

    uiGroup* grp = new uiGroup( tabstack_->tabGroup(), "Values group" );
    mkTable( grp );
    tabstack_->addTab( grp, "Values" );
    putValsToScreen();
}


void uiEditProbDenFunc::mkTable( uiGroup* grp )
{
    mDeclNrDims; mDeclArrNDPDF; mDeclSzVars;

    uiTable::Setup su( nrrows, nrcols );
    su.coldesc( pdf_.dimName(0) )
      .rowdesc( nrdims > 1 ? pdf_.dimName(1) : "Values" )
      .fillrow(true).fillcol(true)
      .manualresize(true).sizesFixed(true);
    tbl_ = new uiTable( grp, su, "Values table" );

    if ( nrdims == 1 )
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
	const float rowval = andpdf->sampling(nrdims<2 ? 0: 1).atIndex(rowidx);
	tbl_->setRowLabel( irow, toString(rowval) );
    }

    uiButtonGroup* bgrp = new uiButtonGroup( grp );
    if ( nrdims > 0 )
    {
	uiToolButton* but = new uiToolButton( bgrp, "View",
				ioPixmap("viewprdf.png"),
				mCB(this,uiEditProbDenFunc,viewPDF) );
	but->setToolTip( "View function" );
    }
    if ( editable_ )
    {
	uiToolButton* but = new uiToolButton( bgrp, "Smooth",
				ioPixmap("smoothcurve.png"),
				mCB(this,uiEditProbDenFunc,smoothReq) );
	but->setToolTip( "Smooth values" );
    }
    if ( nrdims > 2 )
    {
	uiToolButton* but = new uiToolButton( bgrp, "Next",
				mCB(this,uiEditProbDenFunc,dimNext) );
	but->setArrowType( uiToolButton::RightArrow );
	but = new uiToolButton( bgrp, "Prev",
				mCB(this,uiEditProbDenFunc,dimPrev) );
	but->setArrowType( uiToolButton::LeftArrow );
    }
    bgrp->attach( rightOf, tbl_ );
}


uiEditProbDenFunc::~uiEditProbDenFunc()
{
    delete workpdf_;
}


void uiEditProbDenFunc::putValsToScreen()
{
    if ( !workpdf_ ) return;
    mDeclNrDims; mDeclArrNDPDF; mDeclSzVars; mDeclIdxs;

    ArrayND<float>& data = andpdf->getData();
    for ( int irow=0; irow<nrrows; irow++ )
    {
	mGetRowIdx(irow);
	idxs[1] = nrdims == 1 ? 0 : rowidx;
	for ( int icol=0; icol<nrcols; icol++ )
	{
	    idxs[0] = nrdims == 1 ? rowidx : icol;
	    const float arrval = data.getND( idxs );
	    tbl_->setValue( RowCol(irow,icol), arrval );
	}
    }
}


bool uiEditProbDenFunc::getValsFromScreen( bool* chgd )
{
    if ( !workpdf_ ) return true;
    mDeclNrDims; mDeclArrNDPDF; mDeclSzVars; mDeclIdxs;

    ArrayND<float>& data = andpdf->getData();
    for ( int irow=0; irow<nrrows; irow++ )
    {
	mGetRowIdx(irow);
	idxs[1] = nrdims == 1 ? 0 : rowidx;
	for ( int icol=0; icol<nrcols; icol++ )
	{
	    BufferString bstbltxt = tbl_->text( RowCol(irow,icol) );
	    char* tbltxt = bstbltxt.buf();
	    mTrimBlanks(tbltxt);
	    if ( !*tbltxt )
	    {
		uiMSG().error("Please fill all cells - or use 'Cancel'");
		return false;
	    }

	    idxs[0] = nrdims == 1 ? rowidx : icol;
	    const float tblval = atof( tbltxt );
	    const float arrval = data.getND( idxs );
	    if ( !mIsEqual(tblval,arrval,mDefEps) )
	    {
		data.setND( idxs, tblval );
		if ( chgd ) *chgd = true;
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

uiFunctionDisplay* 	funcDisp()	{ return disp_; }

uiFunctionDisplay* 	disp_;

};


void uiEditProbDenFunc::viewPDF( CallBacker* )
{
    mDeclNrDims; mDeclArrNDPDF;
    if ( !andpdf ) return;

    const ArrayND<float>& data = andpdf->getData();
    if ( nrdims == 1 )
    {
	TypeSet<float> xvals;
	const int sz = data.info().getSize(0);
	for ( int idx=0; idx<sz; idx++ )
	    xvals += andpdf->sampling(0).atIndex(idx);

	uiPDF1DViewWin* pdfvwr =
	    new uiPDF1DViewWin( this, xvals.arr(), data.getData(), sz );
	pdfvwr->funcDisp()->xAxis()->setName( workpdf_->dimName(0) );
	pdfvwr->funcDisp()->yAxis(false)->setName( "Frequency" );
	pdfvwr->setDeleteOnClose( false );
	pdfvwr->show();
	return;
    }

    if ( !getValsFromScreen() ) return;

    mDeclSzVars; mDeclIdxs;

    if ( !flatvwwin_ )
    {
	uiFlatViewMainWin::Setup su( "Probability Density Function" );
	flatvwwin_ = new uiFlatViewMainWin( this, su );
	flatvwwin_->setDarkBG( false );
	uiFlatViewer& vwr = flatvwwin_->viewer();
	vwr.setInitialSize( uiSize(300,200) );
	FlatView::Appearance& app = vwr.appearance();
	app.ddpars_.show( false, true );
	FlatView::Annotation& ann = app.annot_;
	ann.title_ = pdf_.name();
	ann.setAxesAnnot( true );
	ann.x1_.name_ = workpdf_->dimName(0);
	ann.x2_.name_ = workpdf_->dimName(1);
	flatvwwin_->windowClosed.notify(mCB(this,uiEditProbDenFunc,vwWinClose));
    }

    Array2D<float>* arr2d = new Array2DImpl<float>( nrcols, nrrows );
    for ( int irow=0; irow<nrrows; irow++ )
    {
	mGetRowIdx(irow);
	idxs[1] = nrdims == 1 ? 0 : rowidx;
	for ( int icol=0; icol<nrcols; icol++ )
	{
	    idxs[0] = nrdims == 1 ? rowidx : icol;
	    arr2d->set( idxs[0], idxs[1], data.getND(idxs) );
	}
    }
    FlatDataPack* dp = new uiEditProbDenFunc2DDataPack( arr2d, *workpdf_ );

    SamplingData<float> sd( andpdf->sampling(0) );
    StepInterval<double> rg( sd.start, sd.start + andpdf->size(0) * sd.step,
	    		     sd.step );
    dp->posData().setRange( true, rg );
    sd = SamplingData<float>( andpdf->sampling(1) );
    rg = StepInterval<double>( sd.start, sd.start + andpdf->size(1) * sd.step,
	    		       sd.step );
    dp->posData().setRange( false, rg );
    DPM( DataPackMgr::FlatID() ).add( dp );

    flatvwwin_->viewer().setPack( false, dp->id(), false );
    flatvwwin_->start();
}


void uiEditProbDenFunc::vwWinClose( CallBacker* )
{
    flatvwwin_ = 0;
}


void uiEditProbDenFunc::dimNext( CallBacker* )
{
}


void uiEditProbDenFunc::dimPrev( CallBacker* )
{
}


void uiEditProbDenFunc::smoothReq( CallBacker* )
{
    mDeclArrNDPDF; if ( !andpdf || !getValsFromScreen() ) return;

    ArrayND<float>* arrclone = andpdf->getArrClone();
    ArrayNDGentleSmoother<float> gs( *arrclone, andpdf->getData() );
    gs.execute();
    delete arrclone;

    putValsToScreen();
    if ( flatvwwin_ )
	viewPDF( 0 );
}


bool uiEditProbDenFunc::acceptOK( CallBacker* )
{
    if ( !editable_ || !workpdf_ ) return true;

    ProbDenFunc* pdf = const_cast<ProbDenFunc*>( &pdf_ );
    for ( int idim=0; idim<pdf_.nrDims(); idim++ )
    {
	const BufferString newnm( nmflds_[idim]->text() );
	if ( newnm != pdf_.dimName(idim) )
	{
	    pdf->setDimName( idim, nmflds_[idim]->text() );
	    chgd_ = true;
	}
    }
    if ( !tbl_ ) return true;

    if ( !getValsFromScreen(&chgd_) )
	return false;

    if ( chgd_ )
	pdf->copyFrom( *workpdf_ );
    return true;
}
