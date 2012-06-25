/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Satyaki Maitra
 Date:          Feb 2010
________________________________________________________________________

-*/

static const char* rcsID mUnusedVar = "$Id: uicreatedpspdf.cc,v 1.16 2012-06-25 19:01:05 cvsnanne Exp $";

#include "uicreatedpspdf.h"

#include "uibutton.h"
#include "uicombobox.h"
#include "uidatapointsetcrossplot.h"
#include "uigeninput.h"
#include "uiprobdenfunvarsel.h"
#include "uiioobjsel.h"
#include "uimsg.h"
#include "uitaskrunner.h"

#include "arrayndimpl.h"
#include "ctxtioobj.h"
#include "dpsdensitycalc.h"
#include "ioobj.h"
#include "probdenfunctr.h"
#include "sampledprobdenfunc.h"


static int cMaxNrPDFs = 3;

uiCreateDPSPDF::uiCreateDPSPDF( uiParent* p,
				uiDataPointSetCrossPlotter& plotter,
       				const BufferStringSet& colnames )
    : uiDialog(p,uiDialog::Setup("Create Probability Density Function",
				 "Specify parameters","111.0.3"))
    , plotter_(plotter)
    , createfrmfld_(0)
    , nrdisp_(1)
{
    setCtrlStyle( DoAndStay );

    uiLabeledComboBox* selcbx = 0;
    if ( plotter_.selAreaSize() )
    {
	BufferStringSet seltype;
        seltype.add( "Whole region" );
	seltype.add( "Selected region" );
	seltype.add( "Non Selected region" );
	selcbx = new uiLabeledComboBox( this, seltype, "Create PDF from" );
	createfrmfld_ = selcbx->box();
    }

    TypeSet<int> colids;
    const DataPointSet& dps = plotter_.dps();
    uiDataPointSet::DColID dcid=-dps.nrFixedCols()+1;
    for ( ; dcid<dps.nrCols(); dcid++ )
	colids += dcid;

    rmbuts_.allowNull(); addbuts_.allowNull();
    uiPrDenFunVarSel::DataColInfo colinfo( colnames, colids );
    const CallBack pushcb = mCB(this,uiCreateDPSPDF,butPush);

    for ( int idx=0; idx<cMaxNrPDFs; idx++ )
    {
	uiPrDenFunVarSel* fld = new uiPrDenFunVarSel( this, colinfo );
	fld->attrSelChanged.notify( mCB(this,uiCreateDPSPDF,setColRange) );
	fld->setColNr( plotter_.axisData(idx).colid_ + 3 );
	setColRange( fld );
	if ( idx == 0 )
	{
	    rmbuts_ += 0;
	    if ( selcbx )
		fld->attach( alignedBelow, selcbx );
	}
	else
	{
	    fld->attach( alignedBelow, probflds_[idx-1] );
	    uiButton* rmbut = new uiPushButton( this, "<- Less", pushcb, true );
	    rmbut->attach( rightAlignedBelow, fld );
	    rmbuts_ += rmbut;
	}

	if ( idx == cMaxNrPDFs-1 )
	    addbuts_ += 0;
	else
	{
	    uiButton* addbut = new uiPushButton( this, "More ->", pushcb, true);
	    addbut->attach( leftAlignedBelow, fld );
	    addbuts_ += addbut;
	}

	probflds_ += fld;
    }

    IOObjContext ioobjctxt = mIOObjContext(ProbDenFunc);
    ioobjctxt.forread = false;
    outputfld_ = new uiIOObjSel( this, ioobjctxt );
    outputfld_->setLabelText( "Output PDF" );
    outputfld_->attach( alignedBelow, probflds_[probflds_.size()-1] );

    butPush( addbuts_[1] );
    if ( plotter_.isY2Shown() )
	butPush( addbuts_[2] );
    handleDisp( 0 );
}


uiCreateDPSPDF::~uiCreateDPSPDF()
{}


void uiCreateDPSPDF::setColRange( CallBacker* cb )
{
    mDynamicCastGet(uiPrDenFunVarSel*,varselfld,cb)
    if ( !varselfld ) return;

    Interval<float> datarange( mUdf(float),-mUdf(float) );

    bool isaxisshown = false;
    for ( int idx=0; idx<3; idx++ )
    {
	if ( varselfld->selColID() ==  plotter_.axisData(idx).colid_ )
	{
	    isaxisshown = true;
	    datarange = plotter_.axisHandler(idx)->range();
	}
    }

    if ( !isaxisshown )
    {
	for ( int rid=0; rid<plotter_.dps().size(); rid++ )
	{
	    const float val = plotter_.getVal(varselfld->selColID(),rid);
	    if ( !mIsUdf(val) && val > datarange.stop )
		datarange.stop = val;
	    if ( !mIsUdf(val) && val < datarange.start )
		datarange.start = val;
	}
    }

    StepInterval<float> attrrange( datarange );
    attrrange.step =
	fabs( datarange.stop-datarange.start )/ varselfld->selNrBins();
    varselfld->setAttrRange( attrrange );
}


void uiCreateDPSPDF::butPush( CallBacker* cb )
{
    mDynamicCastGet(uiButton*,but,cb)
    if ( !but ) return;

    bool isadd = false;
    int idx = addbuts_.indexOf( but );
    if ( idx < 0 )      idx = rmbuts_.indexOf( but );
    else                isadd = true;
    if ( idx < 0 ) return;

    nrdisp_ += isadd ? 1 : -1;
    handleDisp( 0 );
}


void uiCreateDPSPDF::handleDisp( CallBacker* )
{
    for ( int idx=0; idx<probflds_.size(); idx++ )
    {
	const bool dodisp = idx < nrdisp_;
	probflds_[idx]->display( idx < nrdisp_ );
	if ( addbuts_[idx] ) addbuts_[idx]->display( idx == nrdisp_-1 );
	if ( rmbuts_[idx] ) rmbuts_[idx]->display( idx == nrdisp_-1 );
    }
}


void uiCreateDPSPDF::fillPDF( ArrayNDProbDenFunc& pdf )
{
    ObjectSet<DPSDensityCalcND::AxisParam> axisparams;
    mDynamicCastGet(ProbDenFunc*,prdf,&pdf)
    if ( !prdf ) return;

    TypeSet<int> nrbins;
    for ( int dimnr=0; dimnr<prdf->nrDims(); dimnr++ )
    {
	prdf->setDimName( dimnr, probflds_[dimnr]->selColName() );
	StepInterval<float> dimrg = probflds_[dimnr]->selColRange();
	SamplingData<float>& sd = pdf.sampling( dimnr );
	sd.start = dimrg.start + dimrg.step/2;
	sd.step = dimrg.step;
	
	DPSDensityCalcND::AxisParam* axparam =
	    new DPSDensityCalcND::AxisParam();
	axparam->colid_ = probflds_[dimnr]->selColID();
	axparam->valrange_ = dimrg;
	nrbins += probflds_[dimnr]->selNrBins();
	axisparams += axparam;
    }

    if ( prdf->nrDims() == 1 )
    {
	mDynamicCastGet(Sampled1DProbDenFunc*,sprdf,&pdf)
	if ( !sprdf ) return;
	sprdf->bins_.setSize( nrbins[0] );
    }
    else if ( prdf->nrDims() == 2 )
    {
	mDynamicCastGet(Sampled2DProbDenFunc*,sprdf,&pdf)
	if ( !sprdf ) return;
	sprdf->bins_.setSize( nrbins[0], nrbins[1] );
    }
    else
    {
	mDynamicCastGet(SampledNDProbDenFunc*,sprdf,&pdf)
	if ( !sprdf ) return;
	sprdf->bins_.setSize( nrbins.arr() );
    }

    DPSDensityCalcND denscalc( plotter_.uidps(), axisparams,pdf.getData());
    uiTaskRunner tr( this );
    tr.execute( denscalc );
}


bool uiCreateDPSPDF::acceptOK( CallBacker* )
{
    const IOObj* pdfioobj = outputfld_->ioobj();
    if ( !pdfioobj ) return false;

    if ( nrdisp_ == 1 )
    {
	Array1DImpl<float> pdfarr = Array1DImpl<float>( 0 );
	Sampled1DProbDenFunc pdf( pdfarr ); 
	fillPDF( pdf );
	
	BufferString errmsg;
	if ( !ProbDenFuncTranslator::write(pdf,*pdfioobj,&errmsg) )
	    { uiMSG().error(errmsg); return false; }
    }
    else if ( nrdisp_ == 2 )
    {
	Array2DImpl<float> pdfarr = Array2DImpl<float>( Array2DInfoImpl() );
	Sampled2DProbDenFunc pdf( pdfarr ); 
	fillPDF( pdf );
	
	BufferString errmsg;
	if ( !ProbDenFuncTranslator::write(pdf,*pdfioobj,&errmsg) )
	    { uiMSG().error(errmsg); return false; }
    }
    else
    {
	ArrayNDImpl<float> pdfarr =
	    ArrayNDImpl<float>( ArrayNDInfoImpl(nrdisp_) );
	SampledNDProbDenFunc pdf( pdfarr ); 
	fillPDF( pdf );
	
	BufferString errmsg;
	if ( !ProbDenFuncTranslator::write(pdf,*pdfioobj,&errmsg) )
	    { uiMSG().error(errmsg); return false; }
    }


    uiMSG().message( "Probability density function successfully created" );
    return false;
}
