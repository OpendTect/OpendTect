/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uicreatedpspdf.h"

#include "uibutton.h"
#include "uicombobox.h"
#include "uidatapointsetcrossplot.h"
#include "uieditpdf.h"
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
				const uiDataPointSetCrossPlotter* plotter,
				bool requireunits )
    : uiDialog(p,uiDialog::Setup(uiStrings::sCreateProbDesFunc(),
				 mNoDlgTitle,mODHelpKey(mCreateDPSPDFHelpID)))
    , restrictedmode_(false)
    , requireunits_(requireunits)
    , plotter_(plotter)
    , dps_(plotter_->dps())
{
    enableSaveButton( tr("View/Edit after creation") );
    createDefaultUI();
}


uiCreateDPSPDF::uiCreateDPSPDF( uiParent* p, const DataPointSet& dps,
				bool requireunits, bool restricted )
    : uiDialog(p,uiDialog::Setup(uiStrings::sCreateProbDesFunc(),
				 mNoDlgTitle,mODHelpKey(mCreateDPSPDFHelpID)))
    , restrictedmode_(restricted)
    , requireunits_(requireunits)
    , dps_(&dps)
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

    BufferStringSet colnames;
    TypeSet<int> colids;
    MnemonicSelection mns;
    mns.setNullAllowed();
    ObjectSet<const UnitOfMeasure> uoms;
    uoms.setNullAllowed();
    uiDataPointSet::DColID dcid = -dps_->nrFixedCols()+1;
    for ( ; dcid<dps_->nrCols(); dcid++ )
    {
	if ( restrictedmode_ && (dcid<-1 || dcid==0) )
	    continue;

	BufferString colname;
	if ( dcid<0 )
	    colname.set( dcid==-1 ? "Z" : dcid==-3 ? "X-Coord" : "Y-Coord" );
	else
	    colname.set( dps_->colName(dcid) );

	colnames.add( colname.buf() );
	colids += dcid;
	if ( requireunits_ )
	{
	    const UnitOfMeasure* uom = dps_->unit( dcid==-1 ? 0 : dcid );
	    if ( dcid==-1 )
	    {
		mns.add( &Mnemonic::distance() );
		uoms.add( uom );
	    }
	    else
	    {
		StepInterval<float> dimrg = getRange( dcid );
		dimrg.step = fabs( dimrg.width() ) / 25;
		Array1DImpl<float> pdfarr( dimrg.nrSteps()+1 );
		pdfarr.setAll( 1.f );
		Sampled1DProbDenFunc pdf( pdfarr );
		pdf.setDimName( 0, colname.buf() );
		pdf.setRange( 0, dimrg );
		if ( uom )
		    pdf.setUOMSymbol( 0, uom->getLabel() );

		const Mnemonic* guessedmn =
				  uiEditProbDenFunc::guessMnemonic( pdf, 0 );
		const UnitOfMeasure* guesseduom = uom ? uom
				: uiEditProbDenFunc::guessUnit( pdf, 0 );
		mns.add( guessedmn );
		uoms.add( guesseduom );
	    }
	}
    }

    rmbuts_.setNullAllowed(); addbuts_.setNullAllowed();
    uiPrDenFunVarSel::DataColInfo colinfo( colnames, colids, mns, uoms );
    const CallBack pushcb = mCB(this,uiCreateDPSPDF,butPush);

    for ( int idx=0; idx<cMaxNrPDFs; idx++ )
    {
	uiString lbl = tr("%1 %2").arg(uiStrings::sDimension()).arg( idx+1 );
	auto* fld = new uiPrDenFunVarSel( this, colinfo, lbl, requireunits_ );
	mAttachCB( fld->attrSelChanged, uiCreateDPSPDF::setColRange );
	fld->setColNr( plotter_ ? plotter_->axisData(idx).colid_ + 3 : idx );
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

    mAttachCB( postFinalize(), uiCreateDPSPDF::initDlg );
}


uiCreateDPSPDF::~uiCreateDPSPDF()
{
    detachAllNotifiers();
    delete pdf_;
}


void uiCreateDPSPDF::initDlg( CallBacker* )
{
    int nrattribs = dps_->nrCols();
    if ( StringView(dps_->colName(0)) == sKey::MD() )
	nrattribs--;

    for ( int idx=1; idx<prefnrdims_; idx++ )
    {
	if ( nrattribs > idx )
	    butPush( addbuts_[idx] );
    }

    handleDisp( nullptr );
}


void uiCreateDPSPDF::setPrefNrDimensions( int nrdims )
{
    if ( nrdims>=0 && nrdims<7 )
	prefnrdims_ = nrdims;
}


float uiCreateDPSPDF::getVal( int dcid, int drid ) const
{
    if ( dcid >= 0 )
	return dps_->value( dcid, drid );
    else if ( dcid == -1 )
    {
	const float val = dps_->z( drid );
	return val*SI().zDomain().userFactor();
    }

    return float( dcid == -3 ? dps_->coord(drid).x : dps_->coord(drid).y );
}


Interval<float> uiCreateDPSPDF::getRange( DataPointSet::ColID dpcid ) const
{
    Interval<float> ret( mUdf(float),-mUdf(float) );
    bool isaxisshown = false;
    if ( plotter_ )
    {
	for ( int idx=0; idx<3; idx++ )
	{
	    if ( plotter_->axisData(idx).colid_ == dpcid )
	    {
		isaxisshown = true;
		ret = plotter_->axisHandler(idx)->range();
	    }
	}
    }

    if ( !isaxisshown )
    {
	for ( int rid=0; rid<dps_->size(); rid++ )
	{
	    const float val = getVal(dpcid,rid);
	    if ( !mIsUdf(val) && val > ret.stop )
		ret.stop = val;
	    if ( !mIsUdf(val) && val < ret.start )
		ret.start = val;
	}
    }

    return ret;
}


void uiCreateDPSPDF::setColRange( CallBacker* cb )
{
    mDynamicCastGet(uiPrDenFunVarSel*,varselfld,cb)
    if ( !varselfld )
	return;

    StepInterval<float> attrrange = getRange( varselfld->selColID() );
    attrrange.step = fabs( attrrange.width() ) / varselfld->selNrBins();
    varselfld->setAttrRange( attrrange );
}


void uiCreateDPSPDF::butPush( CallBacker* cb )
{
    mDynamicCastGet(uiButton*,but,cb)
    if ( !but )
	return;

    bool isadd = false;
    int idx = addbuts_.indexOf( but );
    if ( idx < 0 )      idx = rmbuts_.indexOf( but );
    else                isadd = true;
    if ( idx < 0 ) return;

    nrdisp_ += isadd ? 1 : -1;
    handleDisp( nullptr );
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
	const UnitOfMeasure* uom = probflds_[dimnr]->getUnit();
	if ( uom )
	    prdf->setUOMSymbol( dimnr, uom->getLabel() );

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

    DPSDensityCalcND denscalc( *dps_, axisparams, pdf.getData(), areatype );
    if ( plotter_ )
	denscalc.setGroup( plotter_->curGroup() );

    uiTaskRunner taskrunner( this );
    TaskRunner::execute( &taskrunner, denscalc );
}


void uiCreateDPSPDF::setPrefDefNames( const BufferStringSet& prefdefnms )
{
    TypeSet<int> handledpropflds;
    for ( int idx=0; idx<prefdefnms.size(); idx++ )
    {
	const char* prefdefnm = prefdefnms.get( idx ).str();
	for ( int iprop=0; iprop<probflds_.size(); iprop++ )
	{
	    if ( handledpropflds.isPresent(iprop) )
		continue;

	    if ( !probflds_.get(iprop)->hasAttrib(prefdefnm) )
		continue;

	    probflds_.get(iprop)->setPrefCol( prefdefnm );
	    handledpropflds += iprop;
	    break;
	}
    }
}


bool uiCreateDPSPDF::acceptOK( CallBacker* )
{
    if ( requireunits_ )
    {
	for ( int idx=0; idx<nrdisp_; idx++ )
	{
	    if ( !probflds_.get(idx)->getUnit() )
	    {
		uiMSG().error( tr("No valid units selected for "
				  "the PDF creation.") );
		return false;
	    }
	}
    }

    if ( !createPDF() )
	return false;

    if ( hasSaveButton() && saveButtonChecked() )
    {
	if ( !viewPDF() )
	    return false;
    }

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
	Array1DImpl<float> pdfarr( 0 );
	Sampled1DProbDenFunc pdf( pdfarr );
	fillPDF( pdf );

	pdf_ = pdf.clone();
	mSavePDF( return false );
    }
    else if ( nrdisp_ == 2 )
    {
	Array2DImpl<float> pdfarr( 0, 0 );
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


bool uiCreateDPSPDF::viewPDF()
{
    uiEditProbDenFuncDlg editdlg( this, *pdf_, true );
    if ( editdlg.go() != uiDialog::Accepted )
	return false;

    mSavePDF( return false );
    return true;
}
