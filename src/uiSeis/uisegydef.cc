/*
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Bert Bril
 Date:          Aug 2001
 RCS:		$Id: uisegydef.cc,v 1.1 2008-09-10 09:05:37 cvsbert Exp $
________________________________________________________________________

-*/

#include "uisegydef.h"
#include "segythdef.h"
#include "segytr.h"
#include "ptrman.h"
#include "iostrm.h"
#include "ioman.h"
#include "iopar.h"
#include "survinfo.h"

#include "uigeninput.h"
#include "uilabel.h"
#include "uimainwin.h"
#include "uibutton.h"
#include "uimsg.h"

static const char* sgyfileflt = "SEG-Y files (*.sgy *.SGY *.segy)";


uiSEGYDef::uiSEGYDef( uiParent* p, uiSEGYDef::Purpose purp,
		      Seis::GeomType gt, const IOPar* iop )
	: uiGroup(p,"SEG-Y definition")
	, xcoordbytefld_(0)
	, scalcofld_(0)
	, inlbytefld_(0)
	, crlbytefld_(0)
	, trnrbytefld_(0)
	, offsbytefld_(0)
	, positioningfld_(0)
        , timeshiftfld_(0)
	, sampleratefld_(0)
	, nrsamplesfld_(0)
	, ensurepsxylbl(0)
    	, purpose_(purp)
    	, geom_(gt)
    	, isrd_(purp != Write)
    	, isps_(Seis::isPS(gt))
    	, is2d_(Seis::is2D(gt))
{
    buildUI( iop );
    mainwin()->finaliseStart.notify( mCB(this,uiSEGYDef,positioningChg) );
}


uiSEGYDef::~uiSEGYDef()
{
}


void uiSEGYDef::getReport( IOPar& iop ) const
{
    const bool useic = haveIC();
    BufferString tmp;
#define mGetIndexNrByteRep(s,dir) { \
    tmp = dir##bytefld_->getIntValue(); \
    tmp += " ("; tmp += dir##byteszfld_->text(); tmp += " bytes)"; \
    iop.set( s, tmp ); }

    if ( useic )
    {
	mGetIndexNrByteRep("In-line byte",inl)
	mGetIndexNrByteRep("Cross-line byte",crl)
    }

    if ( trnrbytefld_ )
	mGetIndexNrByteRep("Trace number byte",trnr);

    if ( offsbytefld_ )
    {
	if ( posType() == 0 )
	{
	    mGetIndexNrByteRep("Offset byte",offs);
	    mGetIndexNrByteRep("Azimuth byte",azim);
	}
	else if ( posType() == 2 )
	{
	    tmp = "Start at ";
	    tmp += regoffsfld_->text(0); tmp += " then step ";
	    tmp += regoffsfld_->text(1);
	    iop.set( "Create offsets", tmp );
	}
    }

    if ( !useic && xcoordbytefld_ )
    {
	iop.set( "X-coord byte", xcoordbytefld_->getIntValue() );
	iop.set( "Y-coord byte", ycoordbytefld_->getIntValue() );
    }

#define mGetOverruleRep(s,fldnm) \
    if ( fldnm##fld_->isChecked() ) iop.set( s, fldnm##fld_->text() )

    if ( scalcofld_ )
    {
	mGetOverruleRep("Overrule.Number of samples",nrsamples);
	mGetOverruleRep("Overrule.coordinate scaling",scalco);
	mGetOverruleRep("Overrule.start time",timeshift);
	mGetOverruleRep("Overrule.sample interval",samplerate);
    }

    iop.set( SEGYSeisTrcTranslator::sNumberFormat, fmtfld_->text() );
}


//--- tools for building the UI ----

static uiGenInput* mkPosFld( uiGroup* grp, const char* key, const IOPar* iop,
			     int def, bool inrev1, const char* dispnm=0 )
{
    if ( iop ) iop->get( key, def );
    if ( def > 239 ) def = mUdf(int);
    IntInpSpec inpspec( def );
    inpspec.setLimits( Interval<int>( 1, 239 ) );
    BufferString txt;
    if ( inrev1 ) txt = "(*) ";
    txt += dispnm ? dispnm : key;
    return new uiGenInput( grp, txt, inpspec.setName(key) );
}


static uiGenInput* mkByteSzFld( uiGroup* grp, const char* key,
				const IOPar* iop, int nrbytes )
{
    if ( iop ) iop->get( key, nrbytes );
    BoolInpSpec bszspec( nrbytes != 2, "4", "2" );
    return new uiGenInput( grp, "Size",  bszspec
	    				.setName(BufferString("4",key),0)
	   				.setName(BufferString("2",key),1) );
}

static uiGenInput* mkOverruleFld( uiGroup* grp, const char* txt,
				  const IOPar* iop, const char* key,
				  bool isz, bool isint=false )
{
    float val = mUdf(float);
    const bool ispresent = iop && iop->get( key, val );
    uiGenInput* inp;
    if ( isint )
    {
	IntInpSpec iis( ispresent ? mNINT(val)
				  : SI().zRange(false).nrSteps() + 1 );
	inp = new uiGenInput( grp, txt, iis );
    }
    else
    {
	if ( !mIsUdf(val) && isz ) val *= SI().zFactor();
	FloatInpSpec fis( val );
	BufferString fldtxt( txt );
	if ( isz ) { fldtxt += " "; fldtxt += SI().getZUnit(); }
	inp = new uiGenInput( grp, fldtxt, fis );
    }

    inp->setWithCheck( true ); inp->setChecked( ispresent );
    return inp;
}


static bool setIf( IOPar& iop, bool yn, const char* key, uiGenInput* inp,
		   bool chkbyte, Seis::GeomType gt )
{
    if ( !yn )
	{ iop.removeWithKey( key ); return true; }

    if ( !chkbyte )
	{ iop.set( key, inp->text() ); return true; }

    int bytenr = inp->getIntValue();
    if ( mIsUdf(bytenr) || bytenr < 1 || bytenr > 239 )
	bytenr = 255;

    if ( bytenr > 111 && bytenr < 119 )
    {
	uiMSG().error( "Bytes 115-118 (number of samples and sample rate)\n"
			"should never be redefined" );
	return false;
    }
    else if ( bytenr > 177 && bytenr < 201 )
    {
#define mHndlByte(stdkey,b,geomyn,fldfill) \
	if ( !strcmp(key,SegyTraceheaderDef::stdkey) && bytenr != b && geomyn )\
	    fld = fldfill

	const char* fld = 0;
	const bool is2d = Seis::is2D( gt );
	mHndlByte(sXCoordByte,181,true,"X-coord");
	else mHndlByte(sYCoordByte,185,true,"Y-coord");
	else mHndlByte(sInlByte,189,!is2d,"In-line");
	else mHndlByte(sCrlByte,193,!is2d,"Cross-line");
	else mHndlByte(sTrNrByte,197,is2d,"Trace number");
	if ( fld )
	{
	    BufferString msg( "Please note that the byte for the " );
	    msg += fld;
	    msg += " may not be applied:\nIt clashes with standard SEG-Y "
		   "revision 1 contents\nContinue?";
	    if ( !uiMSG().askGoOn(msg) )
		return false;
	}
    }
    iop.set( key, bytenr );
    return true;
}


static void setToggled( IOPar& iop, const char* key, uiGenInput* inp,
       			bool isz=false )
{
    bool isdef = inp->isChecked() && *inp->text();
    if ( !isdef )
	iop.removeWithKey( key );
    else
    {
	if ( !isz )
	    iop.set( key, inp->text() );
	else
	    iop.set( key, inp->getfValue() / SI().zFactor() );
    }
}


static void setByteNrFld( uiGenInput* inp, const IOPar& iop, const char* key )
{
    int bnr = inp->getIntValue();
    if ( iop.get(key,bnr) )
	inp->setValue( bnr );
}


static void setByteSzFld( uiGenInput* inp, const IOPar& iop, const char* key )
{
    int nr = inp->getBoolValue() ? 4 : 2;
    if ( iop.get(key,nr) )
	inp->setValue( nr != 2 );
}


static void setToggledFld( uiGenInput* inp, const IOPar& iop, const char* key,
			   bool isz=false )
{
    float val = mUdf(float);
    const bool ispresent = iop.get( key, val );
    inp->setChecked( ispresent );
    if ( ispresent )
    {
	if ( !mIsUdf(val) && isz )
	    val *= SI().zFactor();
	inp->setValue( val );
    }
}


//--- building the UI ----

void uiSEGYDef::buildUI( const IOPar* iop )
{
    uiGroup* posgrp = mkPosGrp( iop );
    uiGroup* orulegrp = mkORuleGrp( iop );
    orulegrp->attach( isrd_ ? alignedBelow : centeredBelow, posgrp );

    StringListInpSpec slspec;
    if ( isrd_ ) slspec.addString( "From tape header" );
    slspec.addString( "1 - Floating point" );
    slspec.addString( "2 - Integer (4 byte)" );
    slspec.addString( "3 - Integer (2 byte)" );
    slspec.addString( "5 - IEEE float (4 byte)" );
    slspec.addString( "8 - Signed char (1 byte)" );
    fmtfld_ = new uiGenInput( this, "SEG-Y format", slspec );
    if ( iop ) setNumbFmtFromPar( *iop );
    fmtfld_->attach( alignedBelow, posgrp );
    fmtfld_->attach( ensureBelow, orulegrp );
}


uiGroup* uiSEGYDef::mkPosGrp( const IOPar* iop )
{
    SegyTraceheaderDef thdef; thdef.fromSettings();
    uiGroup* grp = new uiGroup( this, "Position group" );

    if ( isrd_ && !is2d_ )
	mkBinIDFlds( grp, iop, thdef );

    if ( is2d_ )
	mkTrcNrFlds( grp, iop, thdef );

    if ( is2d_ || !isps_ )
	mkCoordFlds( grp, iop, thdef );

    if ( isps_ && isrd_ )
	mkPreStackPosFlds( grp, iop, thdef );

    attachPosFlds( grp );
    if ( positioningfld_ )
	positioningfld_->valuechanged.notify( 
			mCB(this,uiSEGYDef,positioningChg) );
    return grp;
}


void uiSEGYDef::mkTrcNrFlds( uiGroup* grp, const IOPar* iop,
				   const SegyTraceheaderDef& thdef )
{
    trnrbytefld_ = mkPosFld( grp, SegyTraceheaderDef::sTrNrByte, iop,
			     thdef.trnr, false );
    trnrbyteszfld_ = mkByteSzFld( grp, SegyTraceheaderDef::sTrNrByteSz,
				iop, thdef.trnrbytesz );
    trnrbyteszfld_->attach( rightOf, trnrbytefld_ );
}


void uiSEGYDef::mkBinIDFlds( uiGroup* grp, const IOPar* iop,
				   const SegyTraceheaderDef& thdef )
{
    if ( purpose_ != Scan && !isps_ )
	positioningfld_ = new uiGenInput( grp, "Positioning is defined by",
		    BoolInpSpec(true,"Inline/Crossline","Coordinates") );

    inlbytefld_ = mkPosFld( grp, SegyTraceheaderDef::sInlByte, iop,
			   thdef.inl, true );
    inlbyteszfld_ = mkByteSzFld( grp, SegyTraceheaderDef::sInlByteSz,
				iop, thdef.inlbytesz );
    inlbyteszfld_->attach( rightOf, inlbytefld_ );
    crlbytefld_ = mkPosFld( grp, SegyTraceheaderDef::sCrlByte, iop,
			   thdef.crl, true );
    crlbytefld_->attach( alignedBelow, inlbytefld_ );
    crlbyteszfld_ = mkByteSzFld( grp, SegyTraceheaderDef::sCrlByteSz,
				iop, thdef.crlbytesz );
    crlbyteszfld_->attach( rightOf, crlbytefld_ );
}


void uiSEGYDef::mkPreStackPosFlds( uiGroup* grp, const IOPar* iop,
					 const SegyTraceheaderDef& thdef )
{
    static const char* choices[] = {
	"In file", "From src/rcv coordinates", "Not present", 0 };
    positioningfld_ = new uiGenInput( grp, "Offsets/azimuths",
				StringListInpSpec(choices) );

    offsbytefld_ = mkPosFld( grp, SegyTraceheaderDef::sOffsByte, iop,
			    thdef.offs, false );
    offsbyteszfld_ = mkByteSzFld( grp, SegyTraceheaderDef::sOffsByteSz,
				iop, thdef.offsbytesz );
    offsbyteszfld_->attach( rightOf, offsbytefld_ );
    azimbytefld_ = mkPosFld( grp, SegyTraceheaderDef::sAzimByte, iop,
			    thdef.azim, false, "Azimuth byte (empty=none)" );
    azimbyteszfld_ = mkByteSzFld( grp, SegyTraceheaderDef::sAzimByteSz,
				iop, thdef.azimbytesz );
    azimbyteszfld_->attach( rightOf, azimbytefld_ );
    azimbytefld_->attach( alignedBelow, offsbytefld_ );

    ensurepsxylbl = new uiLabel( grp, "Please be sure that bytes 73-88 actually"
			       " contain\nthe shot- and receiver locations" );

    regoffsfld_ = new uiGenInput( grp, "Set offsets to: start/step",
	    		FloatInpSpec(0), FloatInpSpec(SI().inlDistance()) );
}


void uiSEGYDef::mkCoordFlds( uiGroup* grp, const IOPar* iop,
				   const SegyTraceheaderDef& thdef )
{
    xcoordbytefld_ = mkPosFld( grp, SegyTraceheaderDef::sXCoordByte,
				iop, thdef.xcoord, true );
    ycoordbytefld_ = mkPosFld( grp, SegyTraceheaderDef::sYCoordByte,
			      iop, thdef.ycoord, true );
    ycoordbytefld_->attach( alignedBelow, xcoordbytefld_ );
}


void uiSEGYDef::attachPosFlds( uiGroup* grp )
{
    if ( inlbytefld_ || trnrbytefld_ )
	grp->setHAlignObj( inlbytefld_ ? inlbytefld_ : trnrbytefld_ );
    else
    {
	grp->setHAlignObj( xcoordbytefld_ );
	return;
    }

    if ( positioningfld_ )
    {
	if ( isps_ )
	{
	    if ( crlbytefld_ )
		positioningfld_->attach( alignedBelow, crlbytefld_ );
	    else
	    {
		xcoordbytefld_->attach( alignedBelow, trnrbytefld_ );
		positioningfld_->attach( alignedBelow, ycoordbytefld_ );
	    }
	}
	else if ( xcoordbytefld_ )
	    xcoordbytefld_->attach( alignedBelow, positioningfld_ );
    }
    else
    {
	if ( trnrbytefld_ )
	    xcoordbytefld_->attach( alignedBelow, trnrbytefld_ );
	else if ( purpose_ == Scan )
	    xcoordbytefld_->attach( alignedBelow, crlbytefld_ );
	return;
    }


    if ( !offsbytefld_ )
    {
	if ( inlbytefld_ )
	    inlbytefld_->attach( alignedBelow, positioningfld_ );
    }
    else
    {
	offsbytefld_->attach( alignedBelow, positioningfld_ );
	regoffsfld_->attach( alignedBelow, positioningfld_ );
	ensurepsxylbl->attach( ensureBelow, positioningfld_ );
    }
}


uiGroup* uiSEGYDef::mkORuleGrp( const IOPar* iop )
{
    uiGroup* grp = new uiGroup( this, "Overrule" );

    if ( !isrd_ )
    {
	uiLabel* lbl = new uiLabel( grp,
			"(*) additional to required SEG-Y Rev. 1 fields" );
	lbl->attach( hCentered );
	return grp;
    }

    forcerev0fld_ = new uiCheckBox( grp,
	    		"Use (*) fields even if input is SEG-Y Rev. 1" );
    forcerev0fld_->setName( "SEG-Y Rev.1" );
    forcerev0fld_->attach( hCentered );

    scalcofld_ = mkOverruleFld( grp,
		    "Overrule SEG-Y coordinate scaling", iop,
		    SEGYSeisTrcTranslator::sExternalCoordScaling, false );
    scalcofld_->attach( ensureBelow, forcerev0fld_ );
    BufferString overrulestr = "Overrule SEG-Y start ";
    overrulestr += SI().zIsTime() ? "time" : "depth";
    timeshiftfld_ = mkOverruleFld( grp, overrulestr, iop,
			    SEGYSeisTrcTranslator::sExternalTimeShift,
			    true );
    timeshiftfld_->attach( alignedBelow, scalcofld_ );
    sampleratefld_ = mkOverruleFld( grp, "Overrule SEG-Y sample rate", iop,
			    SEGYSeisTrcTranslator::sExternalSampleRate,
			    true );
    sampleratefld_->attach( alignedBelow, timeshiftfld_ );
    nrsamplesfld_ = mkOverruleFld( grp, "Overrule SEG-Y number of samples",
	    			  iop,
				  SEGYSeisTrcTranslator::sExternalNrSamples,
	   			  false, true );
    nrsamplesfld_->attach( alignedBelow, sampleratefld_ );

    grp->setHAlignObj( scalcofld_ );
    return grp;
}


void uiSEGYDef::usePar( const IOPar& iop )
{
    int icopt = 0, oaopt = 0;

    if ( positioningfld_ )
    {
	if ( !isps_ )
	{
	    icopt = positioningfld_->getBoolValue() ? 1 : -1;
	    iop.get( SegylikeSeisTrcTranslator::sKeyIC2XYOpt, icopt );
	    positioningfld_->setValue( icopt > 0 );
	}
	else
	{
	    oaopt = positioningfld_->getIntValue();
	    iop.get( SegylikeSeisTrcTranslator::sKeyOffsAzimOpt, oaopt );
	    positioningfld_->setValue( oaopt );
	    if ( oaopt == 2 )
	    {
		int start = regoffsfld_->getIntValue(0);
		int step = regoffsfld_->getIntValue(1);
		iop.get( SegylikeSeisTrcTranslator::sKeyOffsDef,
			    start, step );
		regoffsfld_->setValue( start, 0 );
		regoffsfld_->setValue( step, 1 );
	    }
	}
	positioningChg(0);
    }

    if ( icopt >= 0 && inlbytefld_ )
    {
	setByteNrFld( inlbytefld_, iop, SegyTraceheaderDef::sInlByte );
	setByteSzFld( inlbyteszfld_, iop, SegyTraceheaderDef::sInlByteSz );
	setByteNrFld( crlbytefld_, iop, SegyTraceheaderDef::sCrlByte );
	setByteSzFld( crlbyteszfld_, iop, SegyTraceheaderDef::sCrlByteSz );
    }
    if ( icopt <= 0 && xcoordbytefld_ )
    {
	setByteNrFld( xcoordbytefld_, iop, SegyTraceheaderDef::sXCoordByte );
	setByteNrFld( ycoordbytefld_, iop, SegyTraceheaderDef::sYCoordByte );
    }
    if ( trnrbytefld_ )
    {
	setByteNrFld( trnrbytefld_, iop, SegyTraceheaderDef::sTrNrByte );
	setByteSzFld( trnrbyteszfld_, iop, SegyTraceheaderDef::sTrNrByteSz );
    }
    if ( oaopt == 0 && offsbytefld_ )
    {
	setByteNrFld( offsbytefld_, iop, SegyTraceheaderDef::sOffsByte );
	setByteSzFld( offsbyteszfld_, iop, SegyTraceheaderDef::sOffsByteSz );
	setByteNrFld( azimbytefld_, iop, SegyTraceheaderDef::sAzimByte );
	setByteSzFld( azimbyteszfld_, iop, SegyTraceheaderDef::sAzimByteSz );
    }

    if ( scalcofld_ )
    {
	forcerev0fld_->setChecked(
			iop.isTrue(SEGYSeisTrcTranslator::sForceRev0) );
	setToggledFld( scalcofld_, iop,
		       SEGYSeisTrcTranslator::sExternalCoordScaling );
	setToggledFld( timeshiftfld_, iop,
		       SEGYSeisTrcTranslator::sExternalTimeShift, true );
	setToggledFld( sampleratefld_, iop,
		       SEGYSeisTrcTranslator::sExternalSampleRate, true );
	setToggledFld( nrsamplesfld_, iop,
		       SEGYSeisTrcTranslator::sExternalNrSamples );
    }

    setNumbFmtFromPar( iop );
}


bool uiSEGYDef::haveIC() const
{
    if ( purpose_ == Scan ) return true;
    return is2d_ ? false : (isps_ ? true : posType()==1);
}


bool uiSEGYDef::haveXY() const
{
    if ( purpose_ == Scan ) return true;
    return is2d_ ? true : (isps_ ? false : posType()==0);
}


int uiSEGYDef::posType() const
{
    return positioningfld_ ? positioningfld_->getIntValue() : 0;
}


void uiSEGYDef::positioningChg( CallBacker* c )
{
    if ( !isrd_ ) return;
    const bool havexy = haveXY();
    const int postyp = posType();

    if ( inlbytefld_ )
    {
	const bool haveic = haveIC();
	inlbytefld_->display( haveic );
	inlbyteszfld_->display( haveic );
	crlbytefld_->display( haveic );
	crlbyteszfld_->display( haveic );
    }
    if ( xcoordbytefld_ )
    {
	xcoordbytefld_->display( havexy );
	ycoordbytefld_->display( havexy );
    }
    if ( scalcofld_ )
	scalcofld_->display( havexy || (isps_ && postyp==1) );
    if ( isps_ )
    {
	offsbytefld_->display( postyp == 0 );
	offsbyteszfld_->display( postyp == 0 );
	azimbytefld_->display( postyp == 0 );
	azimbyteszfld_->display( postyp == 0 );
	ensurepsxylbl->display( postyp == 1 );
	regoffsfld_->display( postyp == 2 );
    }
    if ( trnrbytefld_ )
    {
	trnrbytefld_->display( is2d_ );
	trnrbyteszfld_->display( is2d_ );
    }
}


bool uiSEGYDef::fillPar( IOPar& iop ) const
{
    iop.setYN( SeisTrcTranslator::sKeyIs2D, is2d_ );
    iop.setYN( SeisTrcTranslator::sKeyIsPS, isps_ );

    const bool haveic = haveIC();

    if ( !positioningfld_ )
    {
	iop.setYN( SegylikeSeisTrcTranslator::sKeyUseHdrCoords, true );
	iop.set( SegylikeSeisTrcTranslator::sKeyIC2XYOpt, is2d_ ? -1 : 0 );
    }
    else
    {
	if ( !haveic )
	{
	    // Just to be sure
	    iop.set( SegyTraceheaderDef::sInlByte, -1 );
	    iop.set( SegyTraceheaderDef::sCrlByte, -1 );
	    iop.setYN( SegylikeSeisTrcTranslator::sKeyUseHdrCoords, true );
	}
	iop.set( SegylikeSeisTrcTranslator::sKeyIC2XYOpt, haveic ? 1 : -1 );
    }

    const bool iswrite = purpose_ == Write;
#define mSetByteIf(yn,key,fld) \
    if ( !setIf(iop,yn,SegyTraceheaderDef::key,fld,iswrite,geom_) ) \
         return false
#define mSetSzIf(yn,key,fld) \
    if ( !setIf(iop,yn,SegyTraceheaderDef::key,fld,false,geom_) ) \
         return false

    if ( inlbytefld_ )
    {
	mSetByteIf( haveic, sInlByte, inlbytefld_ );
	mSetSzIf( haveic, sInlByteSz, inlbyteszfld_ );
	mSetByteIf( haveic, sCrlByte, crlbytefld_ );
	mSetSzIf( haveic, sCrlByteSz, crlbyteszfld_ );
    }

    if ( xcoordbytefld_ )
    {
	const bool havexy = haveXY();
	mSetByteIf( havexy, sXCoordByte, xcoordbytefld_ );
	mSetByteIf( havexy, sYCoordByte, ycoordbytefld_ );
    }
    if ( trnrbytefld_ )
    {
	mSetByteIf( is2d_, sTrNrByte, trnrbytefld_ );
	mSetSzIf( is2d_, sTrNrByteSz, trnrbyteszfld_ );
    }

    if ( isps_ )
    {
	const int postyp = posType();
	iop.set( SegylikeSeisTrcTranslator::sKeyOffsAzimOpt, postyp );
	if ( postyp == 0 )
	{
	    mSetByteIf( isps_, sOffsByte, offsbytefld_ );
	    mSetSzIf( isps_, sOffsByteSz, offsbyteszfld_ );
	    mSetByteIf( isps_, sAzimByte, azimbytefld_ );
	    mSetSzIf( isps_, sAzimByteSz, azimbyteszfld_ );
	}
	else if ( postyp == 2 )
	    iop.set( SegylikeSeisTrcTranslator::sKeyOffsDef,
		regoffsfld_->getIntValue(0), regoffsfld_->getIntValue(1) );
    }

    if ( scalcofld_ )
    {
	iop.setYN( SEGYSeisTrcTranslator::sForceRev0,
			forcerev0fld_->isChecked() );
	setToggled( iop, SEGYSeisTrcTranslator::sExternalNrSamples,
			   nrsamplesfld_ );
	setToggled( iop, SEGYSeisTrcTranslator::sExternalCoordScaling,
			   scalcofld_ );
	setToggled( iop, SEGYSeisTrcTranslator::sExternalTimeShift,
			   timeshiftfld_, true );
	setToggled( iop, SEGYSeisTrcTranslator::sExternalSampleRate,
			   sampleratefld_, true );
    }

    iop.set( SEGYSeisTrcTranslator::sNumberFormat, fmtfld_->text() );

    return true;
}


void uiSEGYDef::setNumbFmtFromPar( const IOPar& iop )
{
    const char* res = iop.find(SEGYSeisTrcTranslator::sNumberFormat);
    if ( res )
	fmtfld_->setText( res );
    else
	fmtfld_->setValue(0);
}
