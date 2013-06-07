/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Satyaki Maitra / Bert
 Date:          Jan 2010
________________________________________________________________________

-*/

static const char* rcsID = "$Id$";

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
    uiToolButton* extendbut = new uiToolButton( this, "extendpdf.png",
	    "Extend one row/col outward", mCB(this,uiImpRokDocPDF,extPDF) );
    extendbut->attach( centeredRightOf, grp );

    IOObjContext ioobjctxt = mIOObjContext(ProbDenFunc);
    ioobjctxt.forread = false;
    outputfld_ = new uiIOObjSel( this, ioobjctxt );
    outputfld_->setLabelText( "Output PDF" );
    outputfld_->attach( alignedBelow, grp );
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

Sampled2DProbDenFunc* getPDF()
{
    if ( !sd_.usable() ) return 0;

    char buf[1024]; int wordnr = 0; int nrdims = 2;
    while ( StrmOper::wordFromLine(*sd_.istrm,buf,1024) )
    {
	wordnr++;
	if ( wordnr == 4 )
	    nrdims = buf[0] - '0';
    }
    if ( nrdims != 2 )
    {
	errmsg_ = "Can only handle 2D PDFs. Dimension found: ";
	errmsg_ += nrdims;
	return 0;
    }

    BufferString dim0nm("X"), dim1nm("Y");
    SamplingData<float> sd0(0,1), sd1(0,1);
    int nr0=-1, nr1=-1;
    bool cols_are_0 = true;
    ascistream astrm( *sd_.istrm, false );
    while ( !astrm.next().atEOS() )
    {
	if ( astrm.hasKeyword(sKeyXNoBins) )
	    nr0 = astrm.getIValue();
	else if ( astrm.hasKeyword(sKeyYNoBins) )
	    nr1 = astrm.getIValue();
	else if ( astrm.hasKeyword(sKeyXLogType) )
	    dim0nm = astrm.value();
	else if ( astrm.hasKeyword(sKeyYLogType) )
	    dim1nm = astrm.value();
	else if ( astrm.hasKeyword(sKeyXBinMin) )
	    sd0.start = astrm.getFValue();
	else if ( astrm.hasKeyword(sKeyXBinWidth) )
	    sd0.step = astrm.getFValue();
	else if ( astrm.hasKeyword(sKeyYBinMin) )
	    sd1.start = astrm.getFValue();
	else if ( astrm.hasKeyword(sKeyYBinWidth) )
	    sd1.step = astrm.getFValue();
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

    for ( int irow=0; irow<nrrows; irow++ )
    {
	const int rowsdidx = nrrows - irow - 1;
	for ( int icol=0; icol<nrcols; icol++ )
	{
	    float val = 0; *sd_.istrm >> val;
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


void uiImpRokDocPDF::selChg( CallBacker* )
{
    RokDocImporter imp( inpfld_->fileName() );
    Sampled2DProbDenFunc* pdf = imp.getPDF();
    if ( !pdf ) return;

    varnmsfld_->setText( pdf->dim0nm_, 0 );
    varnmsfld_->setText( pdf->dim1nm_, 1 );

    int sz = pdf->size( 0 );
    xnrbinfld_->setValue( sz );
    xrgfld_->setValue( pdf->sd0_.start, 0 );
    xrgfld_->setValue( pdf->sd0_.start + (sz-1)*pdf->sd0_.step, 1 );
    sz = pdf->size( 1 );
    ynrbinfld_->setValue( sz );
    yrgfld_->setValue( pdf->sd1_.start, 0 );
    yrgfld_->setValue( pdf->sd1_.start + (sz-1)*pdf->sd1_.step, 1 );

    delete pdf;
}


static bool getPDFFldRes( uiGenInput* rgfld, uiGenInput* szfld, 
			  StepInterval<float>& rg, int& sz )
{
    rg = rgfld->getFInterval(); sz = szfld->getIntValue();

    const bool res = !mIsUdf(rg.start) && !mIsUdf(rg.stop)
		  && rg.start != rg.stop
		  && sz > 1 && !mIsUdf(sz);
    if ( res )
	rg.step = (rg.stop - rg.start) / (sz - 1);

    return res;
}


static void extPDFFlds( uiGenInput* rgfld, uiGenInput* szfld )
{
    StepInterval<float> rg; int sz;
    if ( !getPDFFldRes(rgfld,szfld,rg,sz) ) return;

    rg.start -= rg.step; rg.stop += rg.step; sz += 2;

    rgfld->setValue( rg ); szfld->setValue( sz );
}


void uiImpRokDocPDF::extPDF( CallBacker* )
{
    extPDFFlds( xrgfld_, xnrbinfld_ );
    extPDFFlds( yrgfld_, ynrbinfld_ );
}


Sampled2DProbDenFunc* uiImpRokDocPDF::getAdjustedPDF( Sampled2DProbDenFunc* in )
{
    StepInterval<float> xrg; int xsz; StepInterval<float> yrg; int ysz;
    if ( !getPDFFldRes(xrgfld_,xnrbinfld_,xrg,xsz)
      || !getPDFFldRes(yrgfld_,ynrbinfld_,yrg,ysz) )
	{ delete in; return 0; }

    Array2DImpl<float> a2d( xsz, ysz );
    for ( int ix=0; ix<xsz; ix++ )
    {
	const float x = xrg.start + ix * xrg.step;
	for ( int iy=0; iy<ysz; iy++ )
	{
	    const float y = yrg.start + iy * yrg.step;
	    a2d.set( ix, iy, in->value( x, y ) );
	}
    }

    Sampled2DProbDenFunc* newpdf = new Sampled2DProbDenFunc( a2d );
    newpdf->sd0_.start = xrg.start; newpdf->sd0_.step = xrg.step;
    newpdf->sd1_.start = yrg.start; newpdf->sd1_.step = yrg.step;

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

    RokDocImporter imp( inpfld_->fileName() );
    Sampled2DProbDenFunc* pdf = imp.getPDF();
    if ( !pdf )
	{ uiMSG().error(imp.errmsg_); return false; }

    pdf = getAdjustedPDF( pdf );
    if ( !pdf )
	{ uiMSG().error("Invalid output X and/or Y range/size"); return false; }

    BufferString errmsg;
    if ( !ProbDenFuncTranslator::write(*pdf,*pdfioobj,&errmsg) )
	{ uiMSG().error(errmsg); delete pdf; return false; }

    BufferString msg( "Imported " );
    msg.add( pdf->size(0) ).add("x").add( pdf->size(1) ).add(" PDF.");
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

bool putPDF( const Sampled2DProbDenFunc& pdf )
{
    if ( !sd_.usable() ) return false;

    std::ostream& strm = *sd_.ostrm;

    strm << "Probability Density Function 2D \"" << pdf.name()
	 << "\" output by OpendTect " << GetFullODVersion()
	 << " on " << Time::getDateTimeString() << "\n\n";

    const int nrx = pdf.size( 0 );
    const int nry = pdf.size( 1 );

    strm << "X Log Type        : " << pdf.dimName(0) << '\n';
    strm << "X Mid Bin Minimum : " << pdf.sd0_.start << '\n';
    strm << "X Bin Width       : " << pdf.sd0_.step << '\n';
    strm << "X No of Bins      : " << nrx << "\n\n";

    strm << "Y Log Type        : " << pdf.dimName(1) << '\n';
    strm << "Y Mid Bin Minimum : " << pdf.sd1_.start << '\n';
    strm << "Y Bin Width       : " << pdf.sd1_.step << '\n';
    strm << "Y No of Bins      : " << nry << "\n\n";

    static const char* twentyspaces = "                    ";

    for ( int idx=0; idx<nrx; idx++ )
	strm << BufferString("X Bin ",idx,twentyspaces);

    for ( int iy=nry-1; iy>=0; iy-- )
    {
	strm << '\n';
	for ( int ix=0; ix<nrx; ix++ )
	{
	    BufferString txt; txt += pdf.bins_.get(ix,iy);
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

    mDynamicCastGet(Sampled2DProbDenFunc*,spdf,pdf)
    if ( !spdf )
	{ uiMSG().error("Can only export 2D sampled PDFs"); delete pdf;
	    return false; }

    RokDocExporter exp( outfld_->fileName() );
    if ( !exp.putPDF(*spdf) )
	{ uiMSG().error(exp.errmsg_); delete pdf; return false; }

    delete pdf;
    uiMSG().message( "Output file created" );
    return false;
}
