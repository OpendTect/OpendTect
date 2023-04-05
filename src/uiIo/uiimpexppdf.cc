/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uiimpexppdf.h"

#include "uieditpdf.h"
#include "uifileinput.h"
#include "uiioobjsel.h"
#include "uimsg.h"
#include "uitoolbutton.h"
#include "uiunitsel.h"

#include "ascstream.h"
#include "ctxtioobj.h"
#include "filepath.h"
#include "ioobj.h"
#include "odver.h"
#include "od_helpids.h"
#include "od_ostream.h"
#include "probdenfunctr.h"
#include "sampledprobdenfunc.h"
#include "timefun.h"
#include "unitofmeasure.h"

static const char* sKeyXLogType = "X Log Type";
static const char* sKeyYLogType = "Y Log Type";
static const char* sKeyXBinMin = "X Mid Bin Minimum";
static const char* sKeyYBinMin = "Y Mid Bin Minimum";
static const char* sKeyXBinWidth = "X Bin Width";
static const char* sKeyYBinWidth = "Y Bin Width";
static const char* sKeyXNoBins = "X No of Bins";
static const char* sKeyYNoBins = "Y No of Bins";
static const char* sKeyFirstXBin = "X Bin 0";
static const char* sKeyFirstYBin = "Y Bin 0";


uiImpRokDocPDF::uiImpRokDocPDF( uiParent* p )
    : uiDialog(p,uiDialog::Setup(tr("Import Probability Density Function"),
				 mNoDlgTitle, mODHelpKey(mImpRokDocPDFHelpID))
			   .modal(false))
{
    setOkCancelText( uiStrings::sImport(), uiStrings::sClose() );

    inpfld_ = new uiASCIIFileInput( this, true );
    inpfld_->setSelectMode( uiFileDialog::ExistingFiles );
    inpfld_->valueChanged.notify( mCB(this,uiImpRokDocPDF,selChg) );

    varnmsfld_ = new uiGenInput( this, tr("Output variable names"),
				 StringInpSpec(), StringInpSpec() );
    varnmsfld_->attach( alignedBelow, inpfld_ );
    extendbut_ = new uiToolButton( this, "extendpdf",
	    tr("Extend one row/col outward"), mCB(this,uiImpRokDocPDF,extPDF) );
    extendbut_->attach( rightOf, varnmsfld_ );

    uiGroup* grp = new uiGroup( this, "Output PDF sampling" );
    xrgfld_ = new uiGenInput( grp, tr("Output var1 range"),
			      FloatInpIntervalSpec());
    uiUnitSel::Setup uusu( Mnemonic::Other );
    uusu.mode( uiUnitSel::Setup::SymbolsOnly ).withnone( true );
    xunitfld_ = new uiUnitSel( grp, uusu );
    xunitfld_->attach( rightOf, xrgfld_ );
    xnrbinfld_ = new uiGenInput( grp, tr("Nr of Bins"), IntInpSpec(10,1) );
    xnrbinfld_->setElemSzPol( uiObject::Small );
    xnrbinfld_->attach( rightOf, xunitfld_ );

    yrgfld_ = new uiGenInput( grp, tr("Output var2 range"),
			      FloatInpIntervalSpec());
    yunitfld_ = new uiUnitSel( grp, uusu );
    yunitfld_->attach( rightOf, yrgfld_ );
    ynrbinfld_ = new uiGenInput( grp, tr("Nr of Bins"), IntInpSpec(10,1) );
    ynrbinfld_->setElemSzPol( uiObject::Small );
    yrgfld_->attach( alignedBelow, xrgfld_ );
    ynrbinfld_->attach( rightOf, yunitfld_ );

    grp->setHAlignObj( xrgfld_ );
    grp->attach( alignedBelow, varnmsfld_ );

    IOObjContext ioobjctxt = mIOObjContext(ProbDenFunc);
    ioobjctxt.forread_ = false;
    outputfld_ = new uiIOObjSel( this, ioobjctxt );
    outputfld_->setLabelText(
		    uiStrings::phrOutput( uiStrings::sProbDensFunc(true,1)) );
    outputfld_->attach( alignedBelow, grp );

    setDisplayedFields( false, false );
}


uiImpRokDocPDF::~uiImpRokDocPDF()
{}


void uiImpRokDocPDF::setDisplayedFields( bool dim1, bool dim2 )
{
    const bool doshow = dim1 || dim2;
    varnmsfld_->display( doshow );
    if ( doshow )
    {
	varnmsfld_->displayField( dim1, -1, 0 );
	varnmsfld_->displayField( dim2, -1, 1 );
    }

    extendbut_->display( doshow );
    xrgfld_->display( dim1 );
    xnrbinfld_->display( dim1 );
    xunitfld_->display( dim1 );
    yrgfld_->display( dim2 );
    ynrbinfld_->display( dim2 );
    yunitfld_->display( dim2 );
}


class RokDocImporter
{ mODTextTranslationClass(RokDocImporter)
public:

RokDocImporter( const char* fnm )
    : strm_(fnm)
{
}


int getNrDims()
{
    if ( !strm_.isOK() ) \
    {
	errmsg_ = tr("Cannot open input file to determine nr of dimensions");
	strm_.addErrMsgTo( errmsg_ );
	return 0;
    }

    int nrdims = 0;
    ascistream astrm( strm_, false );
    while ( !astrm.next().atEOS() )
    {
	if ( astrm.hasKeyword(sKeyXLogType) || astrm.hasKeyword(sKeyYLogType) )
	    nrdims++;
    }

    if ( nrdims != 1 && nrdims != 2 )
    {
	errmsg_ = tr("Can only handle 1D and 2D PDFs. Dimensions found: %1")
		     .arg( nrdims );
	return 0;
    }

    return nrdims;
}


const UnitOfMeasure* guessUnit( const ProbDenFunc& pdf, int dim )
{
    return uiEditProbDenFunc::guessUnit( pdf, dim );
}

// reOpen needed after problem with setPostion( 0 ) in RELEASE mode
#define mRewindStream(msg) \
{ \
    strm_.reOpen(); \
    if ( !strm_.isOK() ) \
    { errmsg_ = msg; strm_.addErrMsgTo( errmsg_ ); return nullptr; } \
}


Sampled1DProbDenFunc* get1DPDF()
{
    mRewindStream(tr("Cannot open input file to import 1D PDF data"))

    BufferString varnm("X");
    SamplingData<float> sd(0,1);
    int nr = -1;
    bool cols_are_0 = true;
    ascistream astrm( strm_, false );
    while ( !astrm.next().atEOS() )
    {
	if ( astrm.hasKeyword(sKeyXLogType) )
	    varnm = astrm.value();
	else if ( astrm.hasKeyword(sKeyXBinMin) )
	    sd.start = astrm.getFValue();
	else if ( astrm.hasKeyword(sKeyXBinWidth) )
	    sd.step = astrm.getFValue();
	else if ( astrm.hasKeyword(sKeyXNoBins) )
	    nr = astrm.getIValue();
	else if ( StringView(astrm.keyWord()).startsWith(sKeyFirstXBin) )
	    { cols_are_0 = true; break; }
    }

    if ( nr < 0 )
    {
	errmsg_ = tr("Could not find size for X variable");
	return nullptr;
    }

    Array1DImpl<float> a1d( nr );

    const int nrcols = cols_are_0 ? nr : 1;
    const int nrrows = cols_are_0 ? 1 : nr;

    float val = 0;
    for ( int irow=0; irow<nrrows; irow++ )
    {
	for ( int icol=0; icol<nrcols; icol++ )
	{
	    strm_ >> val;
	    a1d.set( cols_are_0 ? icol : irow, val );
	}
    }
    strm_.close();

    auto* pdf = new Sampled1DProbDenFunc( a1d );
    pdf->setDimName( 0, varnm );
    pdf->sampling() = sd;
    const UnitOfMeasure* uom = uiEditProbDenFunc::guessUnit( *pdf, 0 );
    if ( uom )
	pdf->setUOMSymbol( 0, uom->getLabel() );

    return pdf;
}


Sampled2DProbDenFunc* get2DPDF()
{
    mRewindStream(tr("Cannot open input file to import 2D PDF data"))

    BufferString dim0nm("X"), dim1nm("Y");
    SamplingData<float> sd0(0,1), sd1(0,1);
    int nr0=-1, nr1=-1;
    bool cols_are_0 = true;
    ascistream astrm( strm_, false );
    while ( !astrm.next().atEOS() )
    {
	if ( astrm.hasKeyword(sKeyXLogType) )
	    dim0nm = astrm.value();
	else if ( astrm.hasKeyword(sKeyXBinMin) )
	    sd0.start = astrm.getFValue();
	else if ( astrm.hasKeyword(sKeyXBinWidth) )
	    sd0.step = astrm.getFValue();
	else if ( astrm.hasKeyword(sKeyXNoBins) )
	    nr0 = astrm.getIValue();
	else if ( astrm.hasKeyword(sKeyYLogType) )
	    dim1nm = astrm.value();
	else if ( astrm.hasKeyword(sKeyYBinMin) )
	    sd1.start = astrm.getFValue();
	else if ( astrm.hasKeyword(sKeyYBinWidth) )
	    sd1.step = astrm.getFValue();
	else if ( astrm.hasKeyword(sKeyYNoBins) )
	    nr1 = astrm.getIValue();
	else if ( StringView(astrm.keyWord()).startsWith(sKeyFirstXBin) )
	    { cols_are_0 = true; break; }
	else if ( StringView(astrm.keyWord()).startsWith(sKeyFirstYBin) )
	    { cols_are_0 = false; break; }
    }
    if ( nr0 < 0 || nr1 < 0 )
    {
	errmsg_ = tr("Could not find size for %1")
		.arg(nr0 < 0 ? tr("X variable") : tr("Y variable"));
	return nullptr;
    }

    Array2DImpl<float> a2d( nr0, nr1 );

    const int nrcols = cols_are_0 ? nr0 : nr1;
    const int nrrows = cols_are_0 ? nr1 : nr0;

    float val = 0;
    for ( int irow=0; irow<nrrows; irow++ )
    {
	const int rowsdidx = nrrows - irow - 1;
	for ( int icol=0; icol<nrcols; icol++ )
	{
	    strm_ >> val;
	    a2d.set( cols_are_0 ? icol : rowsdidx,
		     cols_are_0 ? rowsdidx : icol, val );
	}
    }
    strm_.close();

    auto* pdf = new Sampled2DProbDenFunc( a2d );
    pdf->setDimName( 0, dim0nm );
    pdf->setDimName( 1, dim1nm );
    pdf->sampling(0) = sd0;
    pdf->sampling(1) = sd1;
    const UnitOfMeasure* uom = uiEditProbDenFunc::guessUnit( *pdf, 0 );
    if ( uom )
	pdf->setUOMSymbol( 0, uom->getLabel() );

    uom = uiEditProbDenFunc::guessUnit( *pdf, 1 );
    if ( uom )
	pdf->setUOMSymbol( 1, uom->getLabel() );

    return pdf;
}

    uiString		errmsg_;
    od_istream		strm_;

};

#define mInitPDFRead(act) \
{ \
    const int nrdims = imp.getNrDims(); \
    if ( nrdims == 0 ) \
    { uiMSG().error( imp.errmsg_ ); act; } \
\
    if ( nrdims == 1 ) \
	pdf = imp.get1DPDF(); \
    else if ( nrdims == 2 ) \
	pdf = imp.get2DPDF(); \
\
    if ( !pdf ) \
    { uiMSG().error( imp.errmsg_ ); act; } \
}

#define mCastPDF(pdfa,pdfb,pdf,act) \
{ \
    mDynamicCast(Sampled1DProbDenFunc*,pdfa,pdf) \
    mDynamicCast(Sampled2DProbDenFunc*,pdfb,pdf) \
    if ( !pdfa && !pdfb ) \
    { \
	act; \
    } \
}

void uiImpRokDocPDF::selChg( CallBacker* )
{
    RokDocImporter imp( inpfld_->fileName() );
    ArrayNDProbDenFunc* pdf = nullptr;
    mInitPDFRead(delete pdf;return)

    Sampled1DProbDenFunc* pdf1d = nullptr;
    Sampled2DProbDenFunc* pdf2d = nullptr;
    mCastPDF(pdf1d,pdf2d,pdf,delete pdf;return)

    varnmsfld_->setText( pdf1d ? pdf1d->dimName( 0 ) : pdf2d->dimName( 0 ) );
    const Interval<float> xrg = pdf->getRange( 0 );
    xrgfld_->setValue( xrg.start, 0 );
    xrgfld_->setValue( xrg.stop, 1 );
    xnrbinfld_->setValue( pdf->size(0) );
    const UnitOfMeasure* uom = UoMR().get( pdf1d ? pdf1d->getUOMSymbol(0)
						 : pdf2d->getUOMSymbol(0) );
    if ( uom )
	xunitfld_->setUnit( uom );

    if ( pdf2d )
    {
	varnmsfld_->setText( pdf2d->dimName( 1 ), 1 );
	const Interval<float> yrg = pdf->getRange( 1 );
	yrgfld_->setValue( yrg.start, 0 );
	yrgfld_->setValue( yrg.stop, 1 );
	ynrbinfld_->setValue( pdf->size(1) );
	uom = UoMR().get( pdf2d->getUOMSymbol(1) );
	if ( uom )
	    yunitfld_->setUnit( uom );
    }

    setDisplayedFields( pdf1d || pdf2d, pdf2d );

    const FilePath fnmfp( inpfld_->fileName() );
    outputfld_->setInputText( fnmfp.baseName() );
    delete pdf;
}


static bool getPDFFldRes( uiGenInput* rgfld, uiGenInput* szfld,
			  StepInterval<float>& rg, bool forgui )
{
    rg.setInterval( rgfld->getFInterval() );
    const int sz = szfld->getIntValue();
    if ( mIsUdf(sz) )
	return false;

    rg.step = sz < 1 ? mUdf(float) : rg.width() / sz;
    if ( !forgui )
    {
	rg.start += rg.step / 2.f;
	rg.stop -= rg.step / 2.f;
    }

    return !rg.isUdf() || sz == 1;
}


static void extPDFFlds( uiGenInput* rgfld, uiGenInput* szfld )
{
    StepInterval<float> rg;
    if ( !getPDFFldRes(rgfld,szfld,rg,true) )
	return;

    if ( mIsUdf(rg.step) ) return;

    rg.start -= rg.step;
    rg.stop += rg.step;

    rgfld->setValue( rg );
    szfld->setValue( rg.nrSteps() );
}


void uiImpRokDocPDF::extPDF( CallBacker* )
{
    extPDFFlds( xrgfld_, xnrbinfld_ );
    extPDFFlds( yrgfld_, ynrbinfld_ );
}


ArrayNDProbDenFunc* uiImpRokDocPDF::getAdjustedPDF(
						ArrayNDProbDenFunc* in ) const
{
    Sampled1DProbDenFunc* inpdf1d = nullptr;
    Sampled2DProbDenFunc* inpdf2d = nullptr;
    mCastPDF(inpdf1d,inpdf2d,in,return nullptr)

    StepInterval<float> xrg, yrg;
    if ( !getPDFFldRes(xrgfld_,xnrbinfld_,xrg,false) ||
	 ( inpdf2d && !getPDFFldRes(yrgfld_,ynrbinfld_,yrg,false) ) )
    { uiMSG().error(uiStrings::phrInvalid(uiStrings::phrOutput(
						tr("Range/Size")))); return 0; }

    const int xsz = xrg.nrSteps() + 1;
    const int ysz = inpdf2d ? yrg.nrSteps() + 1 : 1;
    Array1DImpl<float> a1d( xsz );
    Array2DImpl<float> a2d( xsz, ysz );
    const bool pdfis1d = inpdf1d || ysz == 1;
    for ( int idx=0; idx<xsz; idx++ )
    {
	const float xval = xrg.atIndex( idx );
	if ( pdfis1d )
	{
	    const float val = inpdf1d ? inpdf1d->value( xval )
				      : inpdf2d->value( xval, 0 );
	    a1d.set( idx, val );
	    continue;
	}

	for ( int idy=0; idy<ysz; idy++ )
	{
	    const float yval = yrg.atIndex( idy );
	    a2d.set( idx, idy, inpdf2d->value( xval, yval ) );
	}
    }

    deleteAndNullPtr( in );
    if ( pdfis1d )
    {
	auto* newpdf = new Sampled1DProbDenFunc( a1d );
	newpdf->setDimName( 0, varnmsfld_->text(0) );
	newpdf->sampling() = SamplingData<float>( xrg );
	const UnitOfMeasure* uom = xunitfld_->getUnit();
	if ( uom )
	    newpdf->setUOMSymbol( 0, uom->getLabel() );
	return newpdf;
    }

    auto* newpdf = new Sampled2DProbDenFunc( a2d );
    newpdf->setDimName( 0, varnmsfld_->text(0) );
    newpdf->setDimName( 1, varnmsfld_->text(1) );
    newpdf->sampling(0) = SamplingData<float>( xrg );
    newpdf->sampling(1) = SamplingData<float>( yrg );
    const UnitOfMeasure* uom = xunitfld_->getUnit();
    if ( uom )
	newpdf->setUOMSymbol( 0, uom->getLabel() );
    uom = yunitfld_->getUnit();
    if ( uom )
	newpdf->setUOMSymbol( 1, uom->getLabel() );

    return newpdf;
}


bool uiImpRokDocPDF::acceptOK( CallBacker* )
{
    const IOObj* pdfioobj = outputfld_->ioobj();
    if ( !pdfioobj ) return false;

    if ( !varnmsfld_->text(0) || !varnmsfld_->text(1) )
    { uiMSG().error( tr("Please enter a variable name") ); return false; }

    RokDocImporter imp( inpfld_->fileName() );
    ArrayNDProbDenFunc* pdf = nullptr;
    mInitPDFRead(delete pdf; return false)

    pdf = getAdjustedPDF( pdf );
    if ( !pdf )
	return false;

    Sampled1DProbDenFunc* pdf1d = nullptr;
    Sampled2DProbDenFunc* pdf2d = nullptr;
    mCastPDF(pdf1d,pdf2d,pdf,return false)

    uiString errmsg;
    if ( ( pdf1d && !ProbDenFuncTranslator::write(*pdf1d,*pdfioobj,&errmsg) ) ||
	 ( pdf2d && !ProbDenFuncTranslator::write(*pdf2d,*pdfioobj,&errmsg) ) )
    { uiMSG().error(errmsg); delete pdf; return false; }

    uiString msg( tr("Imported %1x%2 PDF.\n\nDo you want to import more?" ) );
    msg.arg( pdf->size(0) ).arg( pdf2d ? pdf2d->size(1) : 1 );
    delete pdf;
    bool ret = uiMSG().askGoOn( msg, uiStrings::sYes(),
				tr("No, close window") );
    return !ret;
}



uiExpRokDocPDF::uiExpRokDocPDF( uiParent* p )
    : uiDialog(p,uiDialog::Setup(uiStrings::phrExport(
	   uiStrings::sProbDensFunc(false,1) ),
	   mNoDlgTitle, mODHelpKey(mExpRokDocPDFHelpID) )
				 .savetext(tr("With units"))
				 .savebutton(true).savechecked(true)
				 .modal(false))
{
    setOkCancelText( uiStrings::sExport(), uiStrings::sClose() );

    IOObjContext ioobjctxt = mIOObjContext(ProbDenFunc);
    ioobjctxt.forread_ = true;
    inpfld_ = new uiIOObjSel( this, ioobjctxt );
    inpfld_->setLabelText( uiStrings::phrInput(tr("PDF")) );

    outfld_ = new uiASCIIFileInput( this, false );
    outfld_->setSelectMode( uiFileDialog::AnyFile );
    outfld_->attach( alignedBelow, inpfld_ );
}


uiExpRokDocPDF::~uiExpRokDocPDF()
{}


class RokDocExporter
{ mODTextTranslationClass(RokDocExporter)
public:

RokDocExporter( const char* fnm )
    : strm_(fnm)
{
}


bool put1DPDF( const Sampled1DProbDenFunc& pdf, bool withunits )
{
    if ( !strm_.isOK() )
	{ errmsg_ = uiStrings::sCantOpenOutpFile(); return false; }

    strm_ << "Probability Density Function 1D \"" << pdf.name()
	 << "\" output by OpendTect " << GetFullODVersion()
	 << " on " << Time::getDateTimeString() << "\n\n";

    const int nrx = pdf.size( 0 );
    BufferString varnm( pdf.dimName(0) );
    if ( withunits )
	varnm.add( " (" ).add( pdf.getUOMSymbol(0) ).add( ")" );

    strm_ << "X Log Type	: " << varnm << '\n';
    strm_ << "X Mid Bin Minimum : " << pdf.sampling().start << '\n';
    strm_ << "X Bin Width	: " << pdf.sampling().step << '\n';
    strm_ << "X No of Bins      : " << nrx << "\n\n";

    static const char* twentyspaces = "                    ";

    for ( int idx=0; idx<nrx; idx++ )
	strm_ << BufferString("X Bin ",idx,twentyspaces);

    strm_ << '\n';

    for ( int idx=0; idx<nrx; idx++ )
    {
	BufferString txt( pdf.get(idx) );
	const int len = txt.size();
	txt += len < 20 ? twentyspaces + len : " ";
	strm_ << txt;
    }

    if ( !strm_.isOK() )
	{ errmsg_ = uiStrings::sCannotWrite(); strm_.addErrMsgTo( errmsg_ ); }

    return errmsg_.isEmpty();
}


bool put2DPDF( const Sampled2DProbDenFunc& pdf, bool withunits )
{
    if ( !strm_.isOK() )
	{ errmsg_ = uiStrings::sCantOpenOutpFile(); return false; }

    strm_ << "Probability Density Function 2D \"" << pdf.name()
	 << "\" output by OpendTect " << GetFullODVersion()
	 << " on " << Time::getDateTimeString() << "\n\n";

    const int nrx = pdf.size( 0 );
    const int nry = pdf.size( 1 );
    const float xwidth = nrx == 1 ? 0.f : pdf.sampling(0).step;
    const float ywidth = nry == 1 ? 0.f : pdf.sampling(1).step;

    BufferString varnm( pdf.dimName(0) );
    if ( withunits )
	varnm.add( " (" ).add( pdf.getUOMSymbol(0) ).add( ")" );

    strm_ << "X Log Type	: " << varnm << '\n';
    strm_ << "X Mid Bin Minimum : " << pdf.sampling(0).start << '\n';
    strm_ << "X Bin Width       : " << xwidth << '\n';
    strm_ << "X No of Bins      : " << nrx << "\n\n";

    varnm.set( pdf.dimName(1) );
    if ( withunits )
	varnm.add( " (" ).add( pdf.getUOMSymbol(1) ).add( ")" );

    strm_ << "Y Log Type	: " << varnm << '\n';
    strm_ << "Y Mid Bin Minimum : " << pdf.sampling(1).start << '\n';
    strm_ << "Y Bin Width       : " << ywidth << '\n';
    strm_ << "Y No of Bins      : " << nry << "\n\n";

    const char* twentyspaces = "                    ";

    for ( int idx=0; idx<nrx; idx++ )
	strm_ << BufferString("X Bin ",idx,twentyspaces);

    for ( int iy=nry-1; iy>=0; iy-- )
    {
	strm_ << '\n';
	for ( int ix=0; ix<nrx; ix++ )
	{
	    BufferString txt( pdf.get( ix, iy ) );
	    const int len = txt.size();
	    txt += len < 20 ? twentyspaces + len : " ";
	    strm_ << txt;
	}
    }

    if ( !strm_.isOK() )
	{ errmsg_ = uiStrings::sCannotWrite(); strm_.addErrMsgTo( errmsg_ ); }

    return errmsg_.isEmpty();
}

    uiString		errmsg_;
    od_ostream		strm_;

};


bool uiExpRokDocPDF::acceptOK( CallBacker* )
{
    const IOObj* pdfioobj = inpfld_->ioobj();
    if ( !pdfioobj ) return false;
    uiString errmsg;
    PtrMan<ProbDenFunc> pdf = ProbDenFuncTranslator::read( *pdfioobj, &errmsg );
    if ( !pdf )
	{ uiMSG().error(errmsg); return false; }

    mDynamicCastGet(Sampled1DProbDenFunc*,s1dpdf,pdf.ptr())
    mDynamicCastGet(Sampled2DProbDenFunc*,s2dpdf,pdf.ptr())
    if ( !s1dpdf && !s2dpdf )
    {
	uiMSG().error(tr("Can only export 1D and 2D sampled PDFs"));
	return false;
    }

    const bool withunits = saveButtonChecked();

    RokDocExporter exp( outfld_->fileName() );
    if ( ( s1dpdf && !exp.put1DPDF(*s1dpdf,withunits) ) ||
	 ( s2dpdf && !exp.put2DPDF(*s2dpdf,withunits) ) )
    { uiMSG().error(exp.errmsg_); return false; }

    uiMSG().message( uiStrings::phrSuccessfullyExported(
					uiStrings::sProbDensFunc(false,1) ) );
    return false;
}
