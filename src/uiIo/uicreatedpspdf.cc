/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Satyaki Maitra
 Date:          Feb 2010
________________________________________________________________________

-*/

static const char* rcsID = "$Id: uicreatedpspdf.cc,v 1.1 2010-02-16 06:13:26 cvssatyaki Exp $";

#include "uicreatedpspdf.h"

#include "uicombobox.h"
#include "uidatapointsetcrossplot.h"
#include "uigeninput.h"
#include "uiioobjsel.h"
#include "uimsg.h"

#include "arrayndimpl.h"
#include "ctxtioobj.h"
#include "ioobj.h"
#include "probdenfunctr.h"
#include "sampledprobdenfunc.h"


uiCreateDPSPDF::uiCreateDPSPDF( uiParent* p,
				uiDataPointSetCrossPlotter& plotter )
    : uiDialog(p,uiDialog::Setup("Create Probability Density Function",
				 "Specify parameters",mTODOHelpID))
    , plotter_(plotter)
    , createfrmfld_(0)
    , createoffld_(0)
{
    setCtrlStyle( DoAndStay );

    uiLabeledComboBox* selcbx = 0;
    uiLabeledComboBox* sel1cbx = 0;
    if ( plotter_.selectionAreas().size() )
    {
	BufferStringSet seltype;
        seltype.add( "Whole region" );
	seltype.add( "Selected region" );
	seltype.add( "Non Selected region" );
	selcbx = new uiLabeledComboBox( this, seltype, "Create PDF from" );
	createfrmfld_ = selcbx->box();
    }

    if ( plotter_.isY2Shown() )
    {
	BufferStringSet seltype;
       	seltype.add( "Y1 axis & X axis" );
	seltype.add( "Y2 axis & X axis" );
	sel1cbx = new uiLabeledComboBox( this, seltype, "Create PDF of" );
	createoffld_ = sel1cbx->box();
	if ( createfrmfld_ )
	    sel1cbx->attach( alignedBelow, selcbx );
    }

    nrbinfld_ =  new uiGenInput( this, "Nr of Bins", IntInpIntervalSpec() );
    if ( plotter_.isY2Shown() )
	nrbinfld_->attach( alignedBelow, sel1cbx );
    else if ( plotter_.selectionAreas().size() )
	nrbinfld_->attach( alignedBelow, selcbx );

    IOObjContext ioobjctxt = mIOObjContext(ProbDenFunc);
    ioobjctxt.forread = false;
    outputfld_ = new uiIOObjSel( this, ioobjctxt );
    outputfld_->setLabelText( "Output PDF" );
    outputfld_->attach( alignedBelow, nrbinfld_ );
}


uiCreateDPSPDF::~uiCreateDPSPDF()
{}


void uiCreateDPSPDF::fillPDF( SampledProbDenFunc2D& pdf )
{
    const uiDataPointSetCrossPlotter::AxisData& xaxis = plotter_.axisData(0);
    const uiDataPointSetCrossPlotter::AxisData& yaxis =
	plotter_.axisData( createoffld_&& createoffld_->currentItem() ? 2 : 1 );
    pdf.dim0nm_ = xaxis.axis_->name();
    pdf.dim1nm_ = yaxis.axis_->name();
    
    Interval<int> nrbinrg = nrbinfld_->getIInterval();
    
    const Interval<float> xaxintv = xaxis.axis_->range();
    pdf.sd0_.start = xaxintv.start;
    pdf.sd0_.step = (xaxintv.start - xaxintv.stop)/(float)nrbinrg.start;
    
    const Interval<float> yaxintv = yaxis.axis_->range();
    pdf.sd1_.start = yaxintv.start;
    pdf.sd1_.step = (yaxintv.start - yaxintv.stop)/(float)nrbinrg.stop;

    plotter_.beforeDraw();
    Array2D<float>* data =
	new Array2DImpl<float>( plotter_.arrArea().width()+1,
				plotter_.arrArea().height()+1 );
    data->setAll( (float)0 );
    bool fory2 = false;
    int areatype = 0;
    if ( createfrmfld_ ) areatype = createfrmfld_->currentItem();
    if ( createoffld_ ) fory2 = createoffld_->currentItem();
    plotter_.calcDensity( data, true, false, fory2, areatype, &nrbinrg,
	    		  &pdf.bins_ );
}


bool uiCreateDPSPDF::acceptOK( CallBacker* )
{
    const IOObj* pdfioobj = outputfld_->ioobj();
    if ( !pdfioobj ) return false;

    Array2DImpl<float> pdfarr( -1, -1 );
    SampledProbDenFunc2D pdf( pdfarr ); 
    fillPDF( pdf );

    BufferString errmsg;
    if ( !ProbDenFuncTranslator::write(pdf,*pdfioobj,&errmsg) )
	{ uiMSG().error(errmsg); return false; }

    uiMSG().message( "Probability density function successfully created" );
    return false;
}
