/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          Feb 2010
________________________________________________________________________

-*/

static const char* rcsID = "$Id: uieditpdf.cc,v 1.7 2010-03-02 16:13:50 cvsbert Exp $";

#include "uieditpdf.h"

#include "uigeninput.h"
#include "uitabstack.h"
#include "uitable.h"
#include "uibutton.h"
#include "uimsg.h"
#include "uiflatviewmainwin.h"
#include "uiflatviewer.h"
#include "pixmap.h"

#include "sampledprobdenfunc.h"
#include "arrayndsmoother.h"
#include "flatposdata.h"

#define mDeclArrNDPDF(var) mDynamicCastGet(ArrayNDProbDenFunc*,var,&pdf_)
#define mDeclNrDims \
    const int nrdims = pdf_.nrDims()
#define mDeclSzVars(var) \
    const int nrtbltabs = nrdims > 2 ? (var).info().getSize(2) : 1; \
    const int nrrows = (var).info().getSize( 0 ); \
    const int nrcols = nrdims < 2 ? 1 : (var).info().getSize( 1 )
#define mDeclIdxs(var) \
    TypeSet<int> var( nrdims < 3 ? 2 : nrdims, 0 )
#define mDeclArrVars(var) \
    ArrayND<float>& data = (var).getData(); \
    mDeclIdxs(idxs)


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

    mDeclArrNDPDF(andpdf);
    if ( !andpdf || nrdims > 3 )
	return;

    mDeclSzVars(andpdf->getData());
    uiTable::Setup su( nrrows, nrcols );
    su.rowdesc( pdf_.dimName(0) )
      .coldesc( nrdims > 1 ? pdf_.dimName(1) : "Values" )
      .fillrow(true).fillcol(true)
      .manualresize(true).sizesFixed(true);

    mDeclArrVars(*andpdf);
    for ( int itab=0; itab<nrtbltabs; itab++ )
    {
	BufferString tabnm( "Values" );
	if ( nrdims > 2 ) tabnm.add(" [").add(itab+1).add("]");
	uiGroup* grp = new uiGroup( tabstack_->tabGroup(),
				    BufferString(tabnm," group").buf() );

	uiTable* tbl = new uiTable( grp, su, BufferString("dim ",itab) );
	if ( nrcols == 1 )
	    tbl->setColumnLabel( 0, "Value" );
	else
	{
	    for ( int icol=0; icol<nrcols; icol++ )
	    {
		const float val = andpdf->sampling(1).atIndex(icol);
		tbl->setColumnLabel( icol, toString(val) );
	    }
	}

	for ( int irow=0; irow<nrrows; irow++ )
	{
	    const float rowval = andpdf->sampling(0).atIndex(nrrows - irow - 1);
	    tbl->setRowLabel( irow, toString(rowval) );
	}
	tbls_ += tbl;
	tabstack_->addTab( grp, tabnm );
    }

    uiToolButton* vwbut = 0;
    if ( nrdims > 1 )
    {
	vwbut = new uiToolButton( this, "View",
				ioPixmap("viewprdf.png"),
				mCB(this,uiEditProbDenFunc,viewPDF) );
	vwbut->setToolTip( "View function" );
	vwbut->attach( centeredRightOf, tabstack_ );
    }
    if ( editable_ )
    {
	uiToolButton* smbut = new uiToolButton( this, "Smooth",
				ioPixmap("smoothcurve.png"),
				mCB(this,uiEditProbDenFunc,smoothReq) );
	smbut->setToolTip( "Smooth values" );
	if ( vwbut )
	    smbut->attach( alignedBelow, vwbut );
	else
	    smbut->attach( centeredRightOf, tabstack_ );
    }

    putToScreen( andpdf->getData() );
}


uiEditProbDenFunc::~uiEditProbDenFunc()
{
}


void uiEditProbDenFunc::putToScreen( const ArrayND<float>& data )
{
    mDeclNrDims; mDeclSzVars(data); mDeclIdxs(idxs);

    for ( int itab=0; itab<nrtbltabs; itab++ )
    {
	uiTable& tbl = *tbls_[itab];

	if ( nrdims > 2 ) idxs[2] = itab;
	for ( int irow=0; irow<nrrows; irow++ )
	{
	    idxs[0] = nrrows - irow - 1;
	    for ( int icol=0; icol<nrcols; icol++ )
	    {
		idxs[1] = icol;
		const float arrval = data.getND( idxs.arr() );
		tbl.setValue( RowCol(irow,icol), arrval );
	    }
	}
    }
}


bool uiEditProbDenFunc::getFromScreen( ArrayND<float>& data, bool* chgd )
{
    mDeclNrDims; mDeclSzVars(data); mDeclIdxs(idxs);

    for ( int itab=0; itab<nrtbltabs; itab++ )
    {
	const uiTable& tbl = *tbls_[itab];

	if ( nrdims > 2 ) idxs[2] = itab;
	for ( int irow=0; irow<nrrows; irow++ )
	{
	    idxs[0] = nrrows - irow - 1;
	    for ( int icol=0; icol<nrcols; icol++ )
	    {
		BufferString bstbltxt = tbl.text( RowCol(irow,icol) );
		char* tbltxt = bstbltxt.buf();
		mTrimBlanks(tbltxt);
		if ( !*tbltxt )
		{
		    uiMSG().error("Please fill all cells - or use 'Cancel'");
		    return false;
		}

		idxs[1] = icol;
		const float tblval = atof( tbltxt );
		const float arrval = data.getND( idxs.arr() );
		if ( !mIsEqual(tblval,arrval,mDefEps) )
		{
		    data.setND( idxs.arr(), tblval );
		    if ( chgd ) *chgd = true;
		}
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


void uiEditProbDenFunc::viewPDF( CallBacker* )
{
    mDeclNrDims; mDeclArrNDPDF(andpdf)
    if ( nrdims < 2 || !andpdf ) return;

    ArrayND<float>* arrnd = andpdf->getArrClone();
    getFromScreen( *arrnd );
    mDeclSzVars(*arrnd);
    Array2D<float>* arr2d = new Array2DImpl<float>( nrrows, nrcols );
    int idxs[3];
    idxs[2] = tabstack_->currentPageId()-1;
    if ( idxs[2] < 0 ) idxs[2] = 0;
    for ( idxs[0]=0; idxs[0]<nrcols; idxs[0]++ )
	for ( idxs[1]=0; idxs[1]<nrrows; idxs[1]++ )
	    arr2d->set( idxs[1], idxs[0], arrnd->getND(idxs) );
    delete arrnd;

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
	ann.x1_.name_ = pdf_.dimName(0); ann.x2_.name_ = pdf_.dimName(1);
	flatvwwin_->windowClosed.notify(mCB(this,uiEditProbDenFunc,vwWinClose));
    }

    FlatDataPack* dp = new uiEditProbDenFunc2DDataPack( arr2d, pdf_ );
    SamplingData<float> sd( andpdf->sampling(1) );
    StepInterval<double> rg( sd.start, sd.start + andpdf->size(0) * sd.step,
	    		     sd.step );
    dp->posData().setRange( true, rg );
    sd = SamplingData<float>( andpdf->sampling(0) );
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


void uiEditProbDenFunc::smoothReq( CallBacker* )
{
    mDeclArrNDPDF(andpdf) if ( !andpdf ) return;
    ArrayND<float>* inclone = andpdf->getArrClone();
    getFromScreen( *inclone );
    ArrayND<float>* outclone = andpdf->getArrClone();
    ArrayNDGentleSmoother<float> gs( *inclone, *outclone );
    gs.execute();
    delete inclone;
    putToScreen( *outclone );
    delete outclone;

    if ( flatvwwin_ )
	viewPDF( 0 );
}


bool uiEditProbDenFunc::acceptOK( CallBacker* )
{
    if ( !editable_ ) return true;

    for ( int idim=0; idim<pdf_.nrDims(); idim++ )
    {
	const BufferString newnm( nmflds_[idim]->text() );
	if ( newnm != pdf_.dimName(idim) )
	{
	    pdf_.setDimName( idim, nmflds_[idim]->text() );
	    chgd_ = true;
	}
    }
    if ( tbls_.isEmpty() ) return true;

    mDeclArrNDPDF(andpdf);
    return getFromScreen(andpdf->getData(),&chgd_);
}
