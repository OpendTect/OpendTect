/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Satyaki Maitra
 Date:          Feb 2010
________________________________________________________________________

-*/


#include "uicreatedpspdf.h"

#include "uibutton.h"
#include "uicombobox.h"
#include "uidatapointsetcrossplot.h"
#include "uieditpdf.h"
#include "uigeninput.h"
#include "uiprobdenfunvarsel.h"
#include "uiioobjsel.h"
#include "uimsg.h"
#include "uistrings.h"
#include "uitaskrunner.h"

#include "arrayndimpl.h"
#include "ctxtioobj.h"
#include "datacoldef.h"
#include "dpsdensitycalc.h"
#include "ioobj.h"
#include "probdenfunctr.h"
#include "sampledprobdenfunc.h"
#include "survinfo.h"
#include "unitofmeasure.h"
#include "od_helpids.h"


static int cMaxNrPDFs = 6;

uiCreateDPSPDF::uiCreateDPSPDF( uiParent* p,
				const uiDataPointSetCrossPlotter* plotter )
    : uiDialog(p,uiDialog::Setup(uiStrings::sCreateProbDesFunc(),
				 mNoDlgTitle,mODHelpKey(mCreateDPSPDFHelpID)))
    , restrictedmode_(false)
    , plotter_(plotter)
    , dps_(plotter_->dps())
{
    enableSaveButton( tr("View/Edit after creation") );
    createDefaultUI();
}


uiCreateDPSPDF::uiCreateDPSPDF( uiParent* p, const DataPointSet& dps,
				bool restricted )
    : uiDialog(p,uiDialog::Setup(uiStrings::sCreateProbDesFunc(),
				 mNoDlgTitle,mODHelpKey(mCreateDPSPDFHelpID)))
    , restrictedmode_(restricted)
    , dps_(dps)
{
    enableSaveButton( tr("View/Edit after creation") );
    createDefaultUI();
}


void uiCreateDPSPDF::createDefaultUI()
{
    setOkText( uiStrings::sCreate() );

    uiLabeledComboBox* selcbx = nullptr;
    if ( plotter_ && plotter_->selAreaSize() )
    {
	BufferStringSet seltype( DPSDensityCalcND::CalcAreaTypeNames(), 3 );;
	selcbx = new uiLabeledComboBox( this, seltype, tr("Create PDF from") );
	createfrmfld_ = selcbx->box();
    }

    TypeSet<int> colids;
    BufferStringSet colnames;
    uiDataPointSet::DColID dcid=-dps_.nrFixedCols()+1;
    for ( ; dcid<dps_.nrCols(); dcid++ )
    {
	if ( restrictedmode_ && (dcid<-1 || dcid==0) )
	    continue;
	if ( dcid<0 )
	    colnames.add( dcid==-1 ? "Z" : dcid==-3 ? "X-Coord" : "Y-Coord" );
	else
	    colnames.add( dps_.colName(dcid) );
	colids += dcid;
    }

    rmbuts_.allowNull(); addbuts_.allowNull();
    uiPrDenFunVarSel::DataColInfo colinfo( colnames, colids );
    const CallBack pushcb = mCB(this,uiCreateDPSPDF,butPush);

    for ( int idx=0; idx<cMaxNrPDFs; idx++ )
    {
	uiString lbl = tr("%1 %2").arg(uiStrings::sDimension()).arg( idx+1 );
	auto* fld = new uiPrDenFunVarSel( this, colinfo, lbl );
	fld->attrSelChanged.notify( mCB(this,uiCreateDPSPDF,setColRange) );
	fld->setColNr( plotter_ ? plotter_->axisData(idx).colid_ + 3
				: idx+3 );
	setColRange( fld );
	if ( idx == 0 )
	{
	    rmbuts_ += nullptr;
	    if ( selcbx )
		fld->attach( alignedBelow, selcbx );
	}
	else
	{
	    fld->attach( alignedBelow, probflds_[idx-1] );
	    auto* rmbut = new uiPushButton( this, tr("<- Less"), pushcb, true );
	    rmbut->attach( rightAlignedBelow, fld );
	    rmbuts_ += rmbut;
	}

	if ( idx == cMaxNrPDFs-1 )
	    addbuts_ += nullptr;
	else
	{
	    auto* addbut = new uiPushButton( this, tr("More ->"), pushcb, true);
	    addbut->attach( leftAlignedBelow, fld );
	    addbuts_ += addbut;
	}

	probflds_ += fld;
    }

    IOObjContext ioobjctxt = mIOObjContext(ProbDenFunc);
    ioobjctxt.forread_ = false;
    outputfld_ = new uiIOObjSel( this, ioobjctxt );
    outputfld_->setLabelText(
	uiStrings::phrOutput(uiStrings::sProbDensFunc(true,1)) );
    outputfld_->attach( alignedBelow, probflds_[probflds_.size()-1] );

    int nrattribs = dps_.nrCols();
    if ( FixedString(dps_.colName(0)) == sKey::MD() )
	nrattribs--;

    for ( int idx=1; idx<=2; idx++ )
    {
	if ( nrattribs > idx )
	    butPush( addbuts_[idx] );
    }
    handleDisp( nullptr );
}


uiCreateDPSPDF::~uiCreateDPSPDF()
{
    delete pdf_;
}


float uiCreateDPSPDF::getVal( int dcid, int drid ) const
{
    if ( dcid >= 0 )
	return dps_.value( dcid, drid );
    else if ( dcid == -1 )
    {
	const float val = dps_.z( drid );
	return val*SI().zDomain().userFactor();
    }

    return float( dcid == -3 ? dps_.coord(drid).x : dps_.coord(drid).y );
}


void uiCreateDPSPDF::setColRange( CallBacker* cb )
{
    mDynamicCastGet(uiPrDenFunVarSel*,varselfld,cb)
    if ( !varselfld ) return;

    Interval<float> datarange( mUdf(float),-mUdf(float) );

    bool isaxisshown = false;
    if ( plotter_ )
    {
	for ( int idx=0; idx<3; idx++ )
	{
	    if ( varselfld->selColID() ==  plotter_->axisData(idx).colid_ )
	    {
		isaxisshown = true;
		datarange = plotter_->axisHandler(idx)->range();
	    }
	}
    }

    if ( !isaxisshown )
    {
	for ( int rid=0; rid<dps_.size(); rid++ )
	{
	    DataPointSet::ColID dcolid = varselfld->selColID();
	    const float val = getVal(dcolid,rid);
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
	const BufferString colnm = probflds_[dimnr]->selColName();
	prdf->setDimName( dimnr, colnm );

	const DataPointSet::ColID colid = dps_.indexOf( colnm.buf() );
	const DataColDef& coldef = dps_.colDef( colid );
	if ( coldef.unit_ )
	    prdf->setUOMSymbol( dimnr, coldef.unit_->getLabel() );

	const StepInterval<float> dimrg = probflds_[dimnr]->selColRange();
	pdf.setRange( dimnr, dimrg );

	auto* axparam = new DPSDensityCalcND::AxisParam();
	axparam->colid_ = probflds_[dimnr]->selColID();
	axparam->valrange_ = dimrg;
	nrbins += probflds_[dimnr]->selNrBins();
	axisparams += axparam;
    }

    if ( !pdf.setSize(nrbins) )
	return;

    DPSDensityCalcND::CalcAreaType areatype = DPSDensityCalcND::All;
    if ( createfrmfld_ )
	DPSDensityCalcND::parseEnum( createfrmfld_->text(), areatype );

    DPSDensityCalcND denscalc( dps_, axisparams, pdf.getData(), areatype );
    if ( plotter_ )
	denscalc.setGroup( plotter_->curGroup() );

    uiTaskRunner taskrunner( this );
    TaskRunner::execute( &taskrunner, denscalc );
}


void uiCreateDPSPDF::setPrefDefNames( const BufferStringSet& prefdefnms )
{
    for ( int idx=0; idx<probflds_.size(); idx++ )
    {
	if ( !prefdefnms.validIdx(idx) )
	    continue;
	probflds_[idx]->setPrefCol( prefdefnms.get(idx).buf() );
    }
}


bool uiCreateDPSPDF::acceptOK( CallBacker* )
{
    if ( !createPDF() )
	return false;
    if ( hasSaveButton() && saveButtonChecked() )
	viewPDF();
    if ( !pdf_ )
    {
	uiMSG().error( tr("No valid PDF created") );
	return false;
    }

    return true;
}


#define mSavePDF( retstatement ) \
uiString errmsg; \
const IOObj* ioobj = outputfld_->ioobj(); \
if ( !ProbDenFuncTranslator::write(*pdf_,*ioobj,&errmsg) ) \
{ uiMSG().error(errmsg); retstatement; } \
pdf_->setName( ioobj->name() );


bool uiCreateDPSPDF::createPDF()
{
    const IOObj* pdfioobj = outputfld_->ioobj();
    if ( !pdfioobj ) return false;

    if ( nrdisp_ == 1 )
    {
	Array1DImpl<float> pdfarr = Array1DImpl<float>( 0 );
	Sampled1DProbDenFunc pdf( pdfarr );
	fillPDF( pdf );

	pdf_ = pdf.clone();
	mSavePDF( return false );
    }
    else if ( nrdisp_ == 2 )
    {
	Array2DImpl<float> pdfarr = Array2DImpl<float>( Array2DInfoImpl() );
	Sampled2DProbDenFunc pdf( pdfarr );
	fillPDF( pdf );

	pdf_ = pdf.clone();
	mSavePDF( return false );
    }
    else
    {
	ArrayNDImpl<float> pdfarr =
	    ArrayNDImpl<float>( ArrayNDInfoImpl(nrdisp_) );
	SampledNDProbDenFunc pdf( pdfarr );
	fillPDF( pdf );

	pdf_ = pdf.clone();
	mSavePDF( return false );
    }


    return true;
}


void uiCreateDPSPDF::viewPDF()
{
    uiEditProbDenFuncDlg editdlg( this, *pdf_, true );
    editdlg.go();
    mSavePDF( return );
}
