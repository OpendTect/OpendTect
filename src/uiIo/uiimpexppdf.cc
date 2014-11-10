/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Satyaki Maitra / Bert
 Date:          Jan 2010
________________________________________________________________________

-*/

static const char* rcsID mUsedVar = "$Id$";

#include "uiimpexppdf.h"

#include "uifileinput.h"
#include "uiioobjsel.h"
#include "uitoolbutton.h"
#include "uimsg.h"

#include "ascstream.h"
#include "ctxtioobj.h"
#include "ioobj.h"
#include "probdenfunctr.h"
#include "sampledprobdenfunc.h"
#include "strmprov.h"
#include "strmoper.h"
#include "odver.h"
#include "timefun.h"

#include "hiddenparam.h"

static HiddenParam<uiImpRokDocPDF,uiToolButton*> rokdocimpextendbut_( 0 );

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
static const char* filefilter = "Text (*.txt *.dat)";


uiImpRokDocPDF::uiImpRokDocPDF( uiParent* p )
    : uiDialog(p,uiDialog::Setup("Import Probability Density Function",
				 "Specify parameters","112.0.0"))
{
    setCtrlStyle( DoAndStay );

    inpfld_ = new uiFileInput( this, "Input ASCII File",
	    uiFileInput::Setup(uiFileDialog::Gen)
	    .withexamine(true).forread(true).filter(filefilter) );
    inpfld_->setSelectMode( uiFileDialog::ExistingFiles );
    inpfld_->valuechanged.notify( mCB(this,uiImpRokDocPDF,selChg) );

    varnmsfld_ = new uiGenInput( this, "Output variable names",
				 StringInpSpec(), StringInpSpec() );
    varnmsfld_->attach( alignedBelow, inpfld_ );

    uiGroup* grp = new uiGroup( this, "Output PDF sampling" );
    xrgfld_ = new uiGenInput( grp, "Output var1 Range", FloatInpIntervalSpec());
    xnrbinfld_ = new uiGenInput( grp, "Nr of Bins", IntInpSpec() );
    xnrbinfld_->setElemSzPol( uiObject::Small );
    xnrbinfld_->attach( rightOf, xrgfld_ );
    yrgfld_ = new uiGenInput( grp, "Output var2 Range", FloatInpIntervalSpec());
    ynrbinfld_ = new uiGenInput( grp, "Nr of Bins", IntInpSpec() );
    ynrbinfld_->setElemSzPol( uiObject::Small );
    yrgfld_->attach( alignedBelow, xrgfld_ );
    ynrbinfld_->attach( rightOf, yrgfld_ );
    grp->setHAlignObj( xrgfld_ );
    grp->attach( alignedBelow, varnmsfld_ );
    uiToolButton* extendbut = new uiToolButton( this, "extendpdf",
	    "Extend one row/col outward", mCB(this,uiImpRokDocPDF,extPDF) );
    extendbut->attach( centeredRightOf, grp );
    rokdocimpextendbut_.setParam( this, extendbut );

    IOObjContext ioobjctxt = mIOObjContext(ProbDenFunc);
    ioobjctxt.forread = false;
    outputfld_ = new uiIOObjSel( this, ioobjctxt );
    outputfld_->setLabelText( "Output PDF" );
    outputfld_->attach( alignedBelow, grp );

    setDisplayedFields( false, false );
}


void uiImpRokDocPDF::setDisplayedFields( bool dim1, bool dim2 )
{
    varnmsfld_->displayField( dim1, -1, 0 );
    varnmsfld_->displayField( dim2, -1, 1 );
    rokdocimpextendbut_.getParam( this )->display( dim1 || dim2 );
    xrgfld_->display( dim1 );
    xnrbinfld_->display( dim1 );
    yrgfld_->display( dim2 );
    ynrbinfld_->display( dim2 );
}


class RokDocImporter
{
public:

RokDocImporter( const char* fnm )
{
    sd_ = StreamProvider( fnm ).makeIStream();
    if ( !sd_.usable() )
	{ errmsg_ = "Cannot open input file"; return; }
}

~RokDocImporter()
{
    sd_.close();
}


#define mRewindStream() \
{ \
    StrmOper::seek( *sd_.istrm, 0 ); \
    if ( !sd_.usable() ) \
    { errmsg_ = "Cannot open input file"; return 0; } \
}

int getNrDims()
{
    mRewindStream()

    int nrdims = 0;
    ascistream astrm( *sd_.istrm, false );
    while ( !astrm.next().atEOS() )
    {
	if ( astrm.hasKeyword(sKeyXLogType) || astrm.hasKeyword(sKeyYLogType) )
	    nrdims++;
	else if ( matchString(sKeyFirstXBin,astrm.keyWord()) ||
		  matchString(sKeyFirstYBin,astrm.keyWord()) )
	{ break; }
    }

    if ( nrdims != 1 && nrdims != 2 )
    {
	errmsg_ = "Can only handle 1D and 2D PDFs. Dimension found: ";
	errmsg_ += nrdims;
	return 0;
    }

    return nrdims;
}


Sampled1DProbDenFunc* get1DPDF()
{
    mRewindStream()

    BufferString varnm("X");
    SamplingData<float> sd(0,1);
    int nr = -1;
    bool cols_are_0 = true;
    ascistream astrm( *sd_.istrm, false );
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
	else if ( matchString(sKeyFirstXBin,astrm.keyWord()) )
	    { cols_are_0 = true; break; }
    }

    if ( nr < 0 )
    {
	errmsg_ = "Could not find size for X variable";
	return 0;
    }

    Array1DImpl<float> a1d( nr );

    const int nrcols = cols_are_0 ? nr : 1;
    const int nrrows = cols_are_0 ? 1 : nr;

    float val = 0;
    for ( int irow=0; irow<nrrows; irow++ )
    {
	for ( int icol=0; icol<nrcols; icol++ )
	{
	    *sd_.istrm >> val;
	    a1d.set( cols_are_0 ? icol : irow, val );
	}
    }
    sd_.close();

    Sampled1DProbDenFunc* pdf = new Sampled1DProbDenFunc( a1d );
    pdf->setDimName( 0, varnm );
    pdf->sd_ = sd;
    return pdf;
}


Sampled2DProbDenFunc* get2DPDF()
{
    mRewindStream()

    BufferString dim0nm("X"), dim1nm("Y");
    SamplingData<float> sd0(0,1), sd1(0,1);
    int nr0=-1, nr1=-1;
    bool cols_are_0 = true;
    ascistream astrm( *sd_.istrm, false );
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
	else if ( matchString(sKeyFirstXBin,astrm.keyWord()) )
	    { cols_are_0 = true; break; }
	else if ( matchString(sKeyFirstYBin,astrm.keyWord()) )
	    { cols_are_0 = false; break; }
    }
    if ( nr0 < 0 || nr1 < 0 )
    {
	errmsg_ = "Could not find size for ";
	errmsg_ += nr0 < 0 ? "X variable" : "Y variable";
	return 0;
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
	    *sd_.istrm >> val;
	    a2d.set( cols_are_0 ? icol : rowsdidx,
		     cols_are_0 ? rowsdidx : icol, val );
	}
    }
    sd_.close();

    Sampled2DProbDenFunc* pdf = new Sampled2DProbDenFunc( a2d );
    pdf->setDimName( 0, dim0nm ); pdf->setDimName( 1, dim1nm );
    pdf->sd0_ = sd0; pdf->sd1_ = sd1;
    return pdf;
}

    BufferString	errmsg_;
    StreamData		sd_;

};

#define mInitPDFRead(act) \
{ \
    const int nrdims = imp.getNrDims(); \
    if ( nrdims != 1 && nrdims != 2 ) \
    { uiMSG().error( "Can only import 1D and 2D sampled PDFs" ); act; } \
\
    if ( nrdims == 1 ) \
	pdf = imp.get1DPDF(); \
    else \
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
	act; \
}

void uiImpRokDocPDF::selChg( CallBacker* )
{
    RokDocImporter imp( inpfld_->fileName() );
    ArrayNDProbDenFunc* pdf = 0;
    mInitPDFRead(return)

    Sampled1DProbDenFunc* pdf1d = 0;
    Sampled2DProbDenFunc* pdf2d = 0;
    mCastPDF(pdf1d,pdf2d,pdf,return)

    const SamplingData<float> sdx = pdf1d ? pdf1d->sd_ : pdf2d->sd0_;

    varnmsfld_->setText( pdf1d ? pdf1d->varnm_ : pdf2d->dim0nm_, 0 );
    const int szx = pdf->size( 0 );
    xrgfld_->setValue( sdx.start, 0 );
    xrgfld_->setValue( sdx.atIndex( szx-1 ), 1 );
    xnrbinfld_->setValue( szx );

    if ( pdf2d )
    {
	varnmsfld_->setText( pdf2d->dim1nm_, 1 );
	const int szy = pdf2d->size( 1 );
	yrgfld_->setValue( pdf2d->sd1_.start, 0 );
	yrgfld_->setValue( pdf2d->sd1_.atIndex( szy-1 ), 1 );
	ynrbinfld_->setValue( szy );
    }

    setDisplayedFields( pdf1d || pdf2d, pdf2d );

    delete pdf;
}


static bool getPDFFldRes( uiGenInput* rgfld, uiGenInput* szfld,
			  StepInterval<float>& rg )
{
    rg = rgfld->getFInterval();
    const int sz = szfld->getIntValue();
    if ( mIsUdf(sz) )
	return false;

    rg.step = sz == 1 ? mUdf(float) : rg.width() / (sz - 1);

    return !rg.isUdf() || sz == 1;
}


static void extPDFFlds( uiGenInput* rgfld, uiGenInput* szfld )
{
    StepInterval<float> rg;
    if ( !getPDFFldRes(rgfld,szfld,rg) ) return;

    if ( mIsUdf(rg.step) ) return;

    rg.start -= rg.step; rg.stop += rg.step;

    rgfld->setValue( rg ); szfld->setValue( rg.nrSteps() + 1 );
}


void uiImpRokDocPDF::extPDF( CallBacker* )
{
    extPDFFlds( xrgfld_, xnrbinfld_ );
    extPDFFlds( yrgfld_, ynrbinfld_ );
}


ArrayNDProbDenFunc* uiImpRokDocPDF::getAdjustedPDF(
						ArrayNDProbDenFunc* in ) const
{
    Sampled1DProbDenFunc* inpdf1d = 0;
    Sampled2DProbDenFunc*inpdf2d = 0;
    mCastPDF(inpdf1d,inpdf2d,in,return 0)

    StepInterval<float> xrg, yrg;
    if ( !getPDFFldRes(xrgfld_,xnrbinfld_,xrg) ||
	 ( inpdf2d && !getPDFFldRes(yrgfld_,ynrbinfld_,yrg) ) )
    { uiMSG().error("Invalid output range/size"); return 0; }

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

    delete in; in = 0;
    ArrayNDProbDenFunc* newpdf = 0;
    if ( pdfis1d )
    {
	newpdf = new Sampled1DProbDenFunc( a1d );
	Sampled1DProbDenFunc* newpdf1d = 0;
	mDynamicCast(Sampled1DProbDenFunc*,newpdf1d,newpdf)
	if ( !newpdf1d )
	    return 0;

	newpdf1d->varnm_ = varnmsfld_->text( 0 );
	newpdf1d->sd_ = SamplingData<float>( xrg );
	return newpdf;
    }

    newpdf = new Sampled2DProbDenFunc( a2d );
    Sampled2DProbDenFunc* newpdf2d = 0;
    mDynamicCast(Sampled2DProbDenFunc*,newpdf2d,newpdf)
    if ( !newpdf2d )
	return 0;

    newpdf2d->dim0nm_ = varnmsfld_->text( 0 );
    newpdf2d->dim1nm_ = varnmsfld_->text( 1 );
    newpdf2d->sd0_ = SamplingData<float>( xrg );
    newpdf2d->sd1_ = SamplingData<float>( yrg );

    return newpdf;
}


Sampled2DProbDenFunc* uiImpRokDocPDF::getAdjustedPDF( Sampled2DProbDenFunc* in )
{
    StepInterval<float> xrg; StepInterval<float> yrg;
    if ( !getPDFFldRes(xrgfld_,xnrbinfld_,xrg) ||
         !getPDFFldRes(yrgfld_,ynrbinfld_,yrg) )
	{ delete in; return 0; }

    const int xsz = xrg.nrSteps() + 1;
    const int ysz = yrg.nrSteps() + 1;
    Array2DImpl<float> a2d( xsz, ysz );
    for ( int idx=0; idx<xsz; idx++ )
    {
	const float xval = xrg.atIndex( idx );
	for ( int idy=0; idy<ysz; idy++ )
	{
	    const float yval = yrg.atIndex( idy );
	    a2d.set( idx, idy, in->value( xval, yval ) );
	}
    }

    Sampled2DProbDenFunc* newpdf = new Sampled2DProbDenFunc( a2d );
    newpdf->sd0_ = SamplingData<float>( xrg );
    newpdf->sd1_ = SamplingData<float>( yrg );

    newpdf->dim0nm_ = varnmsfld_->text( 0 );
    newpdf->dim1nm_ = varnmsfld_->text( 1 );
    if ( newpdf->dim0nm_.isEmpty() )
	newpdf->dim0nm_ = in->dim0nm_;
    if ( newpdf->dim1nm_.isEmpty() )
	newpdf->dim1nm_ = in->dim1nm_;

    delete in; return newpdf;
}


bool uiImpRokDocPDF::acceptOK( CallBacker* )
{
    const IOObj* pdfioobj = outputfld_->ioobj();
    if ( !pdfioobj ) return false;

    if ( !varnmsfld_->text(0) || !varnmsfld_->text(1) )
    { uiMSG().error( "Please enter a variable name" ); return false; }

    RokDocImporter imp( inpfld_->fileName() );
    ArrayNDProbDenFunc* pdf = 0;
    mInitPDFRead(return false)

    pdf = getAdjustedPDF( pdf );
    if ( !pdf )
	return false;

    Sampled1DProbDenFunc* pdf1d = 0;
    Sampled2DProbDenFunc* pdf2d = 0;
    mCastPDF(pdf1d,pdf2d,pdf,return false)

    BufferString errmsg;
    if ( ( pdf1d && !ProbDenFuncTranslator::write(*pdf1d,*pdfioobj,&errmsg) ) ||	 ( pdf2d && !ProbDenFuncTranslator::write(*pdf2d,*pdfioobj,&errmsg) ) )
    { uiMSG().error(errmsg); delete pdf; return false; }

    BufferString msg( "Imported " );
    msg.add( pdf->size(0) ).add("x").add( pdf2d ?  pdf2d->size(1) : 1 );
    msg.add(" PDF.");
    uiMSG().message( msg );

    delete pdf;
    return false;
}


uiExpRokDocPDF::uiExpRokDocPDF( uiParent* p )
    : uiDialog(p,uiDialog::Setup("Export Probability Density Function",
				 "Specify parameters","112.0.1"))
{
    setCtrlStyle( DoAndStay );

    IOObjContext ioobjctxt = mIOObjContext(ProbDenFunc);
    ioobjctxt.forread = true;
    inpfld_ = new uiIOObjSel( this, ioobjctxt );
    inpfld_->setLabelText( "Input PDF" );

    outfld_ = new uiFileInput( this, "Output File",
	    uiFileInput::Setup(uiFileDialog::Gen)
	    .withexamine(false).forread(false).filter(filefilter) );
    outfld_->setSelectMode( uiFileDialog::AnyFile );
    outfld_->attach( alignedBelow, inpfld_ );
}


class RokDocExporter
{
public:

RokDocExporter( const char* fnm )
{
    sd_ = StreamProvider( fnm ).makeOStream();
    if ( !sd_.usable() )
	{ errmsg_ = "Cannot open output file"; return; }
}

~RokDocExporter()
{
    sd_.close();
}


bool put1DPDF( const Sampled1DProbDenFunc& pdf )
{
    if ( !sd_.usable() ) return false;

    std::ostream& strm = *sd_.ostrm;

    strm << "Probability Density Function 1D \"" << pdf.name()
	 << "\" output by OpendTect " << GetFullODVersion()
	 << " on " << Time::getDateTimeString() << "\n\n";

    const int nrx = pdf.size( 0 );

    strm << "X Log Type        : " << pdf.dimName(0) << '\n';
    strm << "X Mid Bin Minimum : " << pdf.sd_.start << '\n';
    strm << "X Bin Width       : " << pdf.sd_.step << '\n';
    strm << "X No of Bins      : " << nrx << "\n\n";

    static const char* twentyspaces = "                    ";

    for ( int idx=0; idx<nrx; idx++ )
	strm << BufferString("X Bin ",idx,twentyspaces);

    strm << '\n';

    for ( int idx=0; idx<nrx; idx++ )
    {
	BufferString txt; txt += pdf.bins_.get(idx);
	const int len = txt.size();
	txt += len < 20 ? twentyspaces + len : " ";
	strm << txt;
    }

    const bool ret = strm.good();
    if ( !ret )
	errmsg_ = "Error during write";
    sd_.close();
    return ret;
}


bool put2DPDF( const Sampled2DProbDenFunc& pdf )
{
    if ( !sd_.usable() ) return false;

    std::ostream& strm = *sd_.ostrm;

    strm << "Probability Density Function 2D \"" << pdf.name()
	 << "\" output by OpendTect " << GetFullODVersion()
	 << " on " << Time::getDateTimeString() << "\n\n";

    const int nrx = pdf.size( 0 );
    const int nry = pdf.size( 1 );
    const float xwidth = nrx == 1 ? 0.f : pdf.sd0_.step;
    const float ywidth = nry == 1 ? 0.f : pdf.sd1_.step;

    strm << "X Log Type        : " << pdf.dimName(0) << '\n';
    strm << "X Mid Bin Minimum : " << pdf.sd0_.start << '\n';
    strm << "X Bin Width       : " << xwidth << '\n';
    strm << "X No of Bins      : " << nrx << "\n\n";

    strm << "Y Log Type        : " << pdf.dimName(1) << '\n';
    strm << "Y Mid Bin Minimum : " << pdf.sd1_.start << '\n';
    strm << "Y Bin Width       : " << ywidth << '\n';
    strm << "Y No of Bins      : " << nry << "\n\n";

    static const char* twentyspaces = "                    ";

    for ( int idx=0; idx<nrx; idx++ )
	strm << BufferString("X Bin ",idx,twentyspaces);

    for ( int idy=nry-1; idy>=0; idy-- )
    {
	strm << '\n';
	for ( int idx=0; idx<nrx; idx++ )
	{
	    BufferString txt; txt += pdf.bins_.get(idx,idy);
	    const int len = txt.size();
	    txt += len < 20 ? twentyspaces + len : " ";
	    strm << txt;
	}
    }

    const bool ret = strm.good();
    if ( !ret )
	errmsg_ = "Error during write";
    sd_.close();
    return ret;
}

    BufferString	errmsg_;
    StreamData		sd_;

};


bool uiExpRokDocPDF::acceptOK( CallBacker* )
{
    const IOObj* pdfioobj = inpfld_->ioobj();
    if ( !pdfioobj ) return false;
    BufferString errmsg;
    ProbDenFunc* pdf = ProbDenFuncTranslator::read( *pdfioobj, &errmsg );
    if ( !pdf )
	{ uiMSG().error(errmsg); return false; }

    mDynamicCastGet(Sampled1DProbDenFunc*,s1dpdf,pdf)
    mDynamicCastGet(Sampled2DProbDenFunc*,s2dpdf,pdf)
    if ( !s1dpdf && !s2dpdf )
	{ uiMSG().error("Can only export 1D and 2D sampled PDFs"); delete pdf;
	    return false; }

    RokDocExporter exp( outfld_->fileName() );
    if ( ( s1dpdf && !exp.put1DPDF(*s1dpdf) ) ||
	 ( s2dpdf && !exp.put2DPDF(*s2dpdf) ) )
	{ uiMSG().error(exp.errmsg_); delete pdf; return false; }

    delete pdf;
    uiMSG().message( "Output file created" );
    return false;
}
