/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          Feb 2010
________________________________________________________________________

-*/


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
#include "uicombobox.h"
#include "uilistbox.h"
#include "uilabel.h"

#include "arrayndsmoother.h"
#include "flatposdata.h"
#include "sampledprobdenfunc.h"
#include "gaussianprobdenfunc.h"
#include "od_helpids.h"


uiEditProbDenFunc::uiEditProbDenFunc( uiParent* p, ProbDenFunc& pdf, bool ed )
    : uiGroup(p,"ProbDenFunc editor")
    , inpdf_(pdf)
    , editable_(ed)
    , pdf_(*pdf.clone())
    , nrdims_(pdf.nrDims())
    , chgd_(false)
{
}


uiEditProbDenFuncDlg::uiEditProbDenFuncDlg( uiParent* p, ProbDenFunc& pdf,
					    bool ed, bool isnew )
    : uiDialog(p,uiDialog::Setup(toUiString("%1 %2").arg(ed ? tr("Edit") :
	     tr("Browse")).arg(tr("Probability Density Function")),
	     toUiString("%1 '%2'").arg(ed ? "Edit" : "Browse").arg(pdf.name()
	     .isEmpty() ? tr("PDF") : mToUiStringTodo(pdf.name())),
	     mODHelpKey(mEditProbDenFuncHelpID) ))
	     , edfld_(0)
{
    if ( !ed )
	setCtrlStyle( uiDialog::CloseOnly );

    const FixedString typ( pdf.getTypeStr() );
    if ( typ.startsWith("Sampled") )
	edfld_ = new uiEditSampledProbDenFunc( this, pdf, ed );
    else if ( typ.startsWith("Gaussian") )
	edfld_ = new uiEditGaussianProbDenFunc( this, pdf, ed, isnew );
    else
	new uiLabel( this, tr("Unsupported PDF type: %1")
						   .arg(mToUiStringTodo(typ)) );
}


bool uiEditProbDenFuncDlg::acceptOK( CallBacker* )
{
    if ( !edfld_ )
	return true;

    return edfld_->commitChanges();
}


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


uiEditSampledProbDenFunc::uiEditSampledProbDenFunc( uiParent* p,
				ProbDenFunc& pdf, bool ed )
    : uiEditProbDenFunc(p,pdf,ed)
    , vwwin1d_(0)
    , vwwinnd_(0)
    , tbl_(0)
    , curdim2_(0)
{
    tabstack_ = new uiTabStack( this, "Tabs" );
    mDeclArrNDPDF;
    uiGroup* dimnmgrp = new uiGroup( tabstack_->tabGroup(), "Names group" );
    for ( int idim=0; idim<nrdims_; idim++ )
    {
	uiString txt;
	if ( nrdims_ > 1 )
	    txt = tr( "Variable %1" ).arg( idim + 1 );
	else
	    txt = tr("Variable name");
	uiGenInput* nmfld = new uiGenInput(dimnmgrp, txt, pdf_.dimName(idim) );
	if ( idim )
	    nmfld->attach( alignedBelow, nmflds_[idim-1] );
	nmflds_ += nmfld;
	if ( !editable_ )
	    nmfld->setReadOnly( true );
    }
    tabstack_->addTab( dimnmgrp, nrdims_ < 2 ? uiStrings::sName() :
					       uiStrings::sName(mPlural) );

    if ( !andpdf || nrdims_ > 3 )
	return;

    uiGroup* grp = new uiGroup( tabstack_->tabGroup(), "Values group" );
    mkTable( grp );
    tabstack_->addTab( grp, uiStrings::sValue(mPlural) );
    tabstack_->selChange().notify( mCB(this,uiEditSampledProbDenFunc,tabChg) );

    updateUI();
}


void uiEditSampledProbDenFunc::mkTable( uiGroup* grp )
{
    mDeclSzVars;

    uiTable::Setup su( nrrows, nrcols );
    su.coldesc( pdf_.dimName(0) )
      .rowdesc( nrdims_ > 1 ? pdf_.dimName(1) : "Values" )
      .fillrow(true).fillcol(true)
      .manualresize(true).sizesFixed(true);
    tbl_ = new uiTable( grp, su, "Values table" );

    if ( nrdims_ == 1 )
	tbl_->setColumnLabel( 0, uiStrings::sValue() );
    else
    {
	for ( int icol=0; icol<nrcols; icol++ )
	{
	    const float val = andpdf->sampling(0).atIndex(icol);
	    tbl_->setColumnLabel( icol, toUiString(val) );
	}
    }

    for ( int irow=0; irow<nrrows; irow++ )
    {
	mGetRowIdx(irow);
	const float rowval = andpdf->sampling(nrdims_<2?0:1).atIndex(rowidx);
	tbl_->setRowLabel( irow, toUiString(rowval) );
    }

    uiButtonGroup* bgrp = new uiButtonGroup( grp, "Buttons", OD::Vertical );
    new uiToolButton( bgrp, nrdims_ == 1 ? "distmap" : "viewprdf",
	    tr("View function"), mCB(this,uiEditSampledProbDenFunc,viewPDF) );
    if ( editable_ )
	new uiToolButton( bgrp, "smoothcurve", uiStrings::phrJoinStrings(
				tr("Smooth"), uiStrings::sValue(mPlural)),
				mCB(this,uiEditSampledProbDenFunc,smoothReq) );
    if ( nrdims_ > 2 )
    {
	const char* dim2nm = pdf_.dimName( 2 );
	new uiToolButton( bgrp, uiToolButton::RightArrow,
				toUiString("%1 %2").arg(uiStrings::sNext())
				.arg(dim2nm),
				mCB(this,uiEditSampledProbDenFunc,dimNext) );
	new uiToolButton( bgrp, uiToolButton::LeftArrow,
			    toUiString("%1 %2").arg(uiStrings::sPrevious())
			    .arg(dim2nm),
			    mCB(this,uiEditSampledProbDenFunc,dimPrev) );
    }
    bgrp->attach( rightOf, tbl_ );
}


uiEditSampledProbDenFunc::~uiEditSampledProbDenFunc()
{
    delete &pdf_;
}


#define mErrRet(s) { uiMSG().error(s); return false; }

bool uiEditSampledProbDenFunc::getNamesFromScreen()
{
    BufferStringSet nms;
    for ( int idim=0; idim<nrdims_; idim++ )
    {
	const BufferString newnm( nmflds_[idim]->text() );
	if ( newnm.isEmpty() )
	    mErrRet(tr("Please enter all dimension names"))
	    if (nms.isPresent(newnm))
	    mErrRet(tr("No duplicate dimension names allowed"))
	if ( newnm != pdf_.dimName(idim) )
	{
	    pdf_.setDimName( idim, nmflds_[idim]->text() );
	    chgd_ = true;
	}
    }

    return true;
}


void uiEditSampledProbDenFunc::putValsToScreen()
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


bool uiEditSampledProbDenFunc::getValsFromScreen( bool* chgd )
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
	    BufferString tbltxt = tbl_->text( RowCol(irow,icol) );
	    tbltxt.trimBlanks();
	    if ( tbltxt.isEmpty() )
		mErrRet(tr("Please fill all cells - or use 'Cancel'"))

	    idxs[0] = nrdims_ == 1 ? rowidx : icol;
	    const float tblval = tbltxt.toFloat();
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


class uiEditSampledProbDenFunc2DDataPack : public FlatDataPack
{
public:
uiEditSampledProbDenFunc2DDataPack( Array2D<float>* a2d,
      const ProbDenFunc& pdf )
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
{ mODTextTranslationClass(uiPDF1DViewWin)
public:

uiPDF1DViewWin( uiParent* p, const float* xvals, const float* yvals, int sz )
    : uiDialog(p,uiDialog::Setup(tr("1D PDF Viewer"), uiStrings::sEmptyString(),
				 mNoHelpKey).modal(false) )
{
    setCtrlStyle( uiDialog::CloseOnly );
    disp_ = new uiFunctionDisplay( this, uiFunctionDisplay::Setup() );
    disp_->setVals( xvals, yvals, sz );
}

    uiFunctionDisplay*	disp_;

};


void uiEditSampledProbDenFunc::viewPDF( CallBacker* )
{
    mDeclSzVars; mDeclIdxs;
    if ( !andpdf || !getValsFromScreen() ) return;

    const CallBack clsecb( mCB(this,uiEditSampledProbDenFunc,vwWinClose) );
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
	    vwwin1d_->disp_->xAxis()->setCaption( toUiString(pdf_.dimName(0)) );
	    vwwin1d_->disp_->yAxis(false)->setCaption( uiStrings::sValue() );
	    vwwin1d_->setDeleteOnClose( true );
	    vwwin1d_->windowClosed.notify( clsecb );
	}
	vwwin1d_->show();
    }
    else
    {
	if ( !vwwinnd_ )
	{
	    uiFlatViewMainWin::Setup su( tr("Probability Density Function: %1")
	    .arg(pdf_.name()));
	    su.nrstatusfields(0);
	    vwwinnd_ = new uiFlatViewMainWin( this, su );
	    vwwinnd_->setDarkBG( false );
	    vwwinnd_->setInitialSize( 300, 300 );
	    uiFlatViewer& vwr = vwwinnd_->viewer();
	    FlatView::Appearance& app = vwr.appearance();
	    app.ddpars_.show( false, true );
	    app.ddpars_.vd_.ctab_ = "Rainbow";
	    app.ddpars_.vd_.blocky_ = true;
	    app.ddpars_.vd_.mappersetup_.cliprate_ = Interval<float>(0,0);
	    FlatView::Annotation& ann = app.annot_;
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
	FlatDataPack* dp = new uiEditSampledProbDenFunc2DDataPack(arr2d, pdf_);

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

	vwwinnd_->viewer().clearAllPacks();
	vwwinnd_->viewer().setPack( false, dp->id() );
	vwwinnd_->viewer().setViewToBoundingBox();
	vwwinnd_->start();
    }
}


void uiEditSampledProbDenFunc::vwWinClose( CallBacker* )
{
    vwwinnd_ = 0; vwwin1d_ = 0;
}


void uiEditSampledProbDenFunc::tabChg( CallBacker* )
{
    if ( tabstack_->currentPageId() != 1 )
	return;

    getNamesFromScreen();
    if ( tbl_ )
	setToolTips();
}


void uiEditSampledProbDenFunc::setToolTips()
{
    mDeclSzVars;
#define mMkTT(dim) \
    BufferString tt( pdf_.dimName(dim) ); \
    if ( nrdims_ > 2 ) { mAddDim2Str(tt); }

    if ( nrdims_ > 1 )
    {
	mMkTT(0)
	for ( int icol=0; icol<nrcols; icol++ )
	    tbl_->setColumnToolTip( icol, mToUiStringTodo(tt) );
    }

    mMkTT(nrdims_ > 1 ? 1 : 0)
    for ( int irow=0; irow<nrrows; irow++ )
	tbl_->setRowToolTip( irow, toUiString(tt) );
}


void uiEditSampledProbDenFunc::dimNext( CallBacker* )
{
    mDeclSzVars;
    if ( curdim2_ > nrtbls-2 || !getValsFromScreen() ) return;
    curdim2_++;
    updateUI();
    setToolTips();
}


void uiEditSampledProbDenFunc::dimPrev( CallBacker* )
{
    if ( curdim2_ < 1 || !getValsFromScreen() ) return;
    curdim2_--;
    updateUI();
    setToolTips();
}


void uiEditSampledProbDenFunc::smoothReq( CallBacker* )
{
    mDeclArrNDPDF; if ( !andpdf || !getValsFromScreen() ) return;

    ArrayND<float>* arrclone = andpdf->getArrClone();
    ArrayNDGentleSmoother<float> gs( *arrclone, andpdf->getData() );
    gs.execute();
    delete arrclone;

    updateUI();
}


void uiEditSampledProbDenFunc::updateUI()
{
    putValsToScreen();
    if ( vwwinnd_ || vwwin1d_ )
	viewPDF( 0 );

    mDynamicCastGet(uiDialog*,dlg,parent())
    if ( !dlg ) return;

    mDeclArrNDPDF;
    if ( !andpdf ) return;

    uiString title = tr("%1 '%2'").arg(editable_ ? uiStrings::sEdit() :
		     tr("Browse ")).arg(pdf_.name());
    title = toUiString("%1 %2").arg(title).arg(tr("at %2 = %3")
	    .arg(pdf_.dimName(2)).arg(andpdf->sampling(2).atIndex(curdim2_)));
    dlg->setTitleText( title );
}


bool uiEditSampledProbDenFunc::commitChanges()
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


uiEditGaussianProbDenFunc::uiEditGaussianProbDenFunc( uiParent* p,
					ProbDenFunc& pdf, bool ed, bool isnew )
    : uiEditProbDenFunc(p,pdf,ed)
    , tabstack_(0)
    , ccfld_(0)
    , pdf1d_(0)
    , pdf2d_(0)
    , pdfnd_(0)
    , var1fld_(0)
    , addsetbut_(0)
    , rmbut_(0)
{
    if ( nrdims_ == 1 )
	mDynamicCast(Gaussian1DProbDenFunc*,pdf1d_,&pdf_)
    else if ( nrdims_ == 2 )
	mDynamicCast(Gaussian2DProbDenFunc*,pdf2d_,&pdf_)
    else
	mDynamicCast(GaussianNDProbDenFunc*,pdfnd_,&pdf_)

    uiGroup* varsgrp = 0;
    if ( !pdfnd_ )
	varsgrp = new uiGroup( this, "Vars group" );
    else
    {
	tabstack_ = new uiTabStack( this, "Tabs" );
	varsgrp = new uiGroup( tabstack_->tabGroup(), "Vars group" );
    }

    for ( int idim=0; idim<nrdims_; idim++ )
    {
	uiGenInput* nmfld = new uiGenInput( varsgrp, uiString::emptyString(),
					StringInpSpec(pdf.dimName(idim)) );
	uiGenInput* expfld = new uiGenInput( varsgrp, uiString::emptyString(),
                                             FloatInpSpec() );
	uiGenInput* stdfld = new uiGenInput( varsgrp, uiString::emptyString(),
                                             FloatInpSpec() );
	if ( !isnew )
	{
	    nmfld->setText( pdf.dimName(idim) );
	    float exp, stdev;
	    if ( pdf1d_ )
		{ exp = pdf1d_->exp_; stdev = pdf1d_->std_; }
	    else if ( pdf2d_ && idim==0 )
		{ exp = pdf2d_->exp0_; stdev = pdf2d_->std0_; }
	    else if ( pdf2d_ && idim==1 )
		{ exp = pdf2d_->exp1_; stdev = pdf2d_->std1_; }
	    else
	    {
		exp = pdfnd_->vars_[idim].exp_;
		stdev = pdfnd_->vars_[idim].std_;
	    }
	    expfld->setValue( exp );
	    stdfld->setValue( stdev );
	}

	if ( idim > 0 )
	    nmfld->attach( alignedBelow, nmflds_[idim-1] );
	else
	{
	    varsgrp->setHAlignObj( expfld );
	    uiLabel* lbl = new uiLabel( varsgrp, tr("Variable name") );
	    lbl->attach( centeredAbove, nmfld );
	    lbl = new uiLabel( varsgrp, tr("Expectation") );
	    lbl->attach( centeredAbove, expfld );
	    lbl = new uiLabel( varsgrp, tr("Standard Deviation") );
	    lbl->attach( centeredAbove, stdfld );
	}
	expfld->attach( rightOf, nmfld );
	stdfld->attach( rightOf, expfld );
	nmflds_ += nmfld; expflds_ += expfld; stdflds_ += stdfld;
	if ( !editable_ )
	{
	    nmfld->setReadOnly( true );
	    expfld->setReadOnly( true );
	    stdfld->setReadOnly( true );
	}
    }

    if ( !pdf1d_ )
    {
	uiGroup* ccgrp = 0;
	if ( pdf2d_ )
	{
	    ccfld_ = new uiGenInput(this,uiStrings::sCorrelation(),
							       FloatInpSpec(0));
	    ccfld_->attach( alignedBelow, varsgrp );
	    if ( !isnew )
		ccfld_->setValue( pdf2d_->cc_ );
	    if ( !editable_ )
		ccfld_->setReadOnly( true );
	}
	else
	{
	    ccgrp = new uiGroup( tabstack_->tabGroup(), "CC group" );
	    mkCorrTabFlds( ccgrp );
	    tabstack_->addTab( varsgrp, tr("Distributions") );
	    tabstack_->addTab( ccgrp, uiStrings::sCorrelation(mPlural) );
	    tabstack_->selChange().notify(
				mCB(this,uiEditGaussianProbDenFunc,tabChg) );
	    postFinalise().notify(mCB(this,uiEditGaussianProbDenFunc,initGrp));
	}
    }
}


void uiEditGaussianProbDenFunc::mkCorrTabFlds( uiGroup* ccgrp )
{
    if ( editable_ )
    {
	uiGroup* topgrp = new uiGroup( ccgrp, "CC top group" );
	topgrp->setFrame( true );
	var1fld_ = new uiComboBox( topgrp, "Var 1" );
	var2fld_ = new uiComboBox( topgrp, "Var 2" );
	ccfld_ = new uiGenInput( topgrp, uiStrings::sEmptyString(),
							      FloatInpSpec(0));
	var2fld_->attach( rightOf, var1fld_ );
	ccfld_->attach( rightOf, var2fld_ );
	const CallBack varselcb( mCB(this,uiEditGaussianProbDenFunc,varSel) );
	var1fld_->selectionChanged.notify( varselcb );
	var2fld_->selectionChanged.notify( varselcb );
	uiLabel* lbl = new uiLabel( topgrp, tr("Correlate") );
	lbl->attach( centeredAbove, var1fld_ );
	lbl = new uiLabel( topgrp, tr("With") );
	lbl->attach( centeredAbove, var2fld_ );
	lbl = new uiLabel( topgrp, uiStrings::sCoefficient() );
	lbl->attach( centeredAbove, ccfld_ );

	const CallBack cb( mCB(this,uiEditGaussianProbDenFunc,addSetPush) );
	addsetbut_ = new uiPushButton( ccgrp, uiStrings::sAdd(), cb, true );
	addsetbut_->attach( centeredBelow, topgrp );
	ccfld_->updateRequested.notify( cb );
    }

    defcorrsfld_ = new uiListBox( ccgrp, "Defined Correlations" );
    if ( editable_ )
	defcorrsfld_->attach( centeredBelow, addsetbut_ );
    defcorrsfld_->setStretch( 2, 2 );
    defcorrsfld_->selectionChanged.notify(
				mCB(this,uiEditGaussianProbDenFunc,corrSel) );

    if ( editable_ )
    {
	rmbut_ = new uiToolButton( ccgrp, "remove",
				uiStrings::phrRemove(uiStrings::phrJoinStrings(
				tr("Selected"),uiStrings::sCorrelation())),
				mCB(this,uiEditGaussianProbDenFunc,rmPush) );
	rmbut_->attach( rightOf, defcorrsfld_ );
    }
}


int uiEditGaussianProbDenFunc::findCorr() const
{
    if ( !var1fld_ ) return -1;

    const GaussianNDProbDenFunc::Corr corr( var1fld_->currentItem(),
					    var2fld_->currentItem() );
    return pdfnd_->corrs_.indexOf( corr );
}


void uiEditGaussianProbDenFunc::updateCorrList( int cursel )
{
    if ( !editable_ ) return;

    NotifyStopper stopper( defcorrsfld_->selectionChanged );
    defcorrsfld_->setEmpty();
    if ( pdfnd_->corrs_.isEmpty() )
	return;

    const int nrcorrs = pdfnd_->corrs_.size();

    for ( int icorr=0; icorr<nrcorrs; icorr++ )
    {
	GaussianNDProbDenFunc::Corr corr = pdfnd_->corrs_[icorr];
	uiString itmtxt = toUiString("%1 <-> %2 (%3)")
			  .arg(pdfnd_->dimName(corr.idx0_))
			  .arg(pdfnd_->dimName(corr.idx1_))
			  .arg(corr.cc_);
	defcorrsfld_->addItem( itmtxt );
    }

    if ( cursel >= nrcorrs )
	cursel = nrcorrs - 1;
    else if ( cursel < 0 )
	cursel = 0;

    stopper.enableNotification();
    defcorrsfld_->setCurrentItem( cursel );
}

void uiEditGaussianProbDenFunc::initGrp( CallBacker* )
{
    updateCorrList( 0 );
}

void uiEditGaussianProbDenFunc::tabChg( CallBacker* )
{
    if ( !tabstack_ ) return;

    BufferStringSet varnms;
    for ( int idx=0; idx<nmflds_.size(); idx++ )
    {
	GaussianNDProbDenFunc::VarDef& vd = pdfnd_->vars_[idx];
	vd.name_ = nmflds_[idx]->text();
	vd.exp_ = expflds_[idx]->getFValue();
	vd.std_ = stdflds_[idx]->getFValue();
	varnms.add( vd.name_ );
    }

    if ( var1fld_ )
    {
	NotifyStopper stopper1( var1fld_->selectionChanged );
	NotifyStopper stopper2( var2fld_->selectionChanged );
	var1fld_->setEmpty(); var2fld_->setEmpty();
	var1fld_->addItems( varnms ); var2fld_->addItems( varnms );
    }

    corrSel( 0 );
}


void uiEditGaussianProbDenFunc::corrSel( CallBacker* )
{
    if ( !editable_ ) return;

    const int selidx = defcorrsfld_->currentItem();
    rmbut_->setSensitive( selidx >= 0 );
    if ( selidx < 0 )
	return;

    GaussianNDProbDenFunc::Corr& corr = pdfnd_->corrs_[selidx];
    var1fld_->setCurrentItem( corr.idx0_ );
    var2fld_->setCurrentItem( corr.idx1_ );
    ccfld_->setValue( corr.cc_ );

    varSel( 0 );
}


void uiEditGaussianProbDenFunc::varSel( CallBacker* cb )
{
    if ( !editable_ ) return;

    const int icorr = findCorr();
    addsetbut_->setText( var1fld_->currentItem() == var2fld_->currentItem()
    ? toUiString("-") : (icorr < 0 ? uiStrings::sAdd() : uiStrings::sSet() ) );
}


float uiEditGaussianProbDenFunc::getCC() const
{
    const float cc = ccfld_->getFValue();
    if ( mIsUdf(cc) )
	return 0;
    if ( cc < -cMaxGaussianCC() || cc > cMaxGaussianCC() )
    {
	uiMSG().error( mToUiStringTodo(sGaussianCCRangeErrMsg()) );
	return mUdf(float);
    }
    return cc;
}


void uiEditGaussianProbDenFunc::addSetPush( CallBacker* )
{
    const int idx0 = var1fld_->currentItem();
    const int idx1 = var2fld_->currentItem();
    if ( idx0 == idx1 )
	return;
    const float cc = getCC();
    if ( mIsUdf(cc) )
	return;
    else if ( cc == 0 )
    { uiMSG().error(tr("A zero correlation is not a correlation")); return; }

    int icorr = findCorr();
    if ( icorr >= 0 )
	pdfnd_->corrs_[icorr].cc_ = cc;
    else
    {
	pdfnd_->corrs_ += GaussianNDProbDenFunc::Corr( idx0, idx1, cc );
	icorr = pdfnd_->corrs_.size() - 1;
    }

    updateCorrList( icorr );
}


void uiEditGaussianProbDenFunc::rmPush( CallBacker* )
{
    int selidx = defcorrsfld_->currentItem();
    if ( selidx >= 0 )
    {
	pdfnd_->corrs_.removeSingle( selidx );
	updateCorrList( selidx );
    }
}


bool uiEditGaussianProbDenFunc::commitChanges()
{
    if ( !editable_ ) return true;

    BufferStringSet varnms;
    if ( pdfnd_ )
	pdfnd_->vars_.setEmpty();

    for ( int idim=0; idim<nmflds_.size(); idim++ )
    {
	const FixedString nm = nmflds_[idim]->text();
	const float exp = expflds_[idim]->getFValue();
	const float stdev = stdflds_[idim]->getFValue();
	if ( nm.isEmpty() )
	    mErrRet(tr("Please enter a name for all dimensions"))
	else if (varnms.isPresent(nm))
	    mErrRet(tr("Please enter different names for all dimensions"))
	if (mIsUdf(exp) || mIsUdf(stdev))
	    mErrRet(tr("Please enter all distribution values"))
	    if (stdev == 0)
	    mErrRet(tr("Standard deviations cannot be zero"))
	varnms.add( nm );

	if ( pdfnd_ )
	    pdfnd_->vars_ += GaussianNDProbDenFunc::VarDef( nm, exp, stdev );
	else
	{
	    pdf_.setDimName( idim, nm );
	    if ( pdf1d_ )
		{ pdf1d_->exp_ = exp; pdf1d_->std_ = stdev; }
	    else if ( idim == 0 )
		{ pdf2d_->exp0_ = exp; pdf2d_->std0_ = stdev; }
	    else if ( idim == 1 )
		{ pdf2d_->exp1_ = exp; pdf2d_->std1_ = stdev; }
	}
    }

    if ( pdfnd_ )
    {
	tabChg( 0 );
	const char* uncorr = pdfnd_->firstUncorrelated();
	if ( uncorr && *uncorr && !uiMSG().askGoOn(
	    tr("Variable '%1' is not correlated."
	       "\nDo you wish to leave uncorrelated variables?")
	  .arg(uncorr)))
	    return false;
    }
    else if ( pdf2d_ )
    {
	pdf2d_->cc_ = getCC();
	if ( mIsUdf(pdf2d_->cc_) )
	    return false;
    }

    chgd_ = !inpdf_.isEqual( pdf_ );
    const_cast<ProbDenFunc&>(inpdf_).copyFrom( pdf_ );
    return true;
}
