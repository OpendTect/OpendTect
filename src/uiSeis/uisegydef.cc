/*
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert Bril
 Date:          Sep 2008
________________________________________________________________________

-*/
static const char* rcsID = "$Id: uisegydef.cc,v 1.36 2010-10-14 09:58:06 cvsbert Exp $";

#include "uisegydef.h"
#include "segythdef.h"
#include "seistrctr.h"
#include "ptrman.h"
#include "iostrm.h"
#include "ioman.h"
#include "iopar.h"
#include "survinfo.h"
#include "oddirs.h"
#include "envvars.h"
#include "settings.h"
#include "file.h"
#include "filepath.h"
#include "seisioobjinfo.h"

#include "uifileinput.h"
#include "uilabel.h"
#include "uiseparator.h"
#include "uimainwin.h"
#include "uibutton.h"
#include "uitabstack.h"
#include "uilineedit.h"
#include "pixmap.h"
#include "uimsg.h"

const char* uiSEGYFileSpec::sKeyLineNmToken()  { return "#L"; }
static const char* sgyfileflt = "SEG-Y files (*.sgy *.SGY *.segy)";
static const char* sKeyEnableByteSwapWrite = "Enable SEG-Y byte swap writing";
static int enabbyteswapwrite = -1;
static BufferString lastreaddir;
static BufferString lastwritedir;


//--- uiSEGYFileSpec ----


uiSEGYFileSpec::uiSEGYFileSpec( uiParent* p, const uiSEGYFileSpec::Setup& su )
    : uiSEGYDefGroup(p,"SEGY::FileSpec group",su.forread_)
    , multifld_(0)
    , is2d_(!su.canbe3d_)
{
    SEGY::FileSpec spec; if ( su.pars_ ) spec.usePar( *su.pars_ );
    const bool canbemulti = forread_ && su.canbe3d_;

    BufferString disptxt( forread_ ? "Input" : "Output" );
    disptxt += " SEG-Y file";
    if ( canbemulti ) disptxt += "(s)";
    fnmfld_ = new uiFileInput( this, disptxt,
		uiFileInput::Setup(uiFileDialog::Gen)
		.forread(forread_).filter(sgyfileflt) );
    BufferString defdir( forread_ ? lastreaddir : lastwritedir );
    if ( defdir.isEmpty() ) defdir = GetDataDir();
    fnmfld_->setDefaultSelectionDir( defdir );

    if ( canbemulti )
    {
	IntInpIntervalSpec inpspec( true );
	inpspec.setName( "File number start", 0 );
	inpspec.setName( "File number stop", 1 );
	inpspec.setName( "File number step", 2 );
	inpspec.setValue( StepInterval<int>(1,2,1) );
	multifld_ = new uiGenInput( this, "Multiple files; Numbers", inpspec );
	multifld_->setWithCheck( true ); multifld_->setChecked( false );
	multifld_->attach( alignedBelow, fnmfld_ );
    }

    if ( su.pars_ ) usePar( *su.pars_ );
    setHAlignObj( fnmfld_ );
}


SEGY::FileSpec uiSEGYFileSpec::getSpec() const
{
    SEGY::FileSpec spec( fnmfld_->fileName() );
    if ( !is2d_ && multifld_ && multifld_->isChecked() )
    {
	spec.nrs_ = multifld_->getIStepInterval();
	const char* txt = multifld_->text();
	mSkipBlanks(txt);
	if ( *txt == '0' )
	    spec.zeropad_ = strlen(txt);
	if ( spec.zeropad_ == 1 ) spec.zeropad_ = 0;
    }
    return spec;
}


#define mErrRet(s) { uiMSG().error( s ); return false; }

bool uiSEGYFileSpec::fillPar( IOPar& iop, bool perm ) const
{
    SEGY::FileSpec spec( getSpec() );

    if ( !perm )
    {
	if ( spec.fname_.isEmpty() )
	    mErrRet("No file name specified")

	if ( forread_ )
	{
	    if ( spec.isMultiFile() )
	    {
		if ( !strchr(spec.fname_.buf(),'*') )
		    mErrRet("Please put a wildcard ('*') in the file name")
	    }
	    else if ( !File::exists(spec.fname_) )
		mErrRet("Selected input file does not exist")
	}
    }

    (forread_ ? lastreaddir : lastwritedir) = FilePath(spec.fname_).pathOnly();
    spec.fillPar( iop );
    return true;
}


void uiSEGYFileSpec::setMultiInput( const StepInterval<int>& nrs, int zp )
{
    if ( !multifld_ ) return;

    if ( mIsUdf(nrs.start) )
	multifld_->setChecked( false );
    else
    {
	multifld_->setChecked( true );
	multifld_->setValue( nrs );

	if ( zp > 1 )
	{
	    UserInputObj* inpobj = multifld_->element( 0 );
	    mDynamicCastGet(uiLineEdit*,le,inpobj)
	    if ( le )
	    {
		BufferString txt( toString(nrs.start) );
		const int nrzeros = zp - txt.size();
		if ( nrzeros > 0 )
		{
		    txt = "0";
		    for ( int idx=1; idx<nrzeros; idx++ )
			txt += "0";
		    txt += nrs.start;
		}
		le->setText( txt );
	    }
	}
    }

    multifld_->display( !is2d_ );
}


void uiSEGYFileSpec::setSpec( const SEGY::FileSpec& spec )
{
    fnmfld_->setFileName( spec.fname_ );
    setMultiInput( spec.nrs_, spec.zeropad_ );
}


void uiSEGYFileSpec::usePar( const IOPar& iop )
{
    SEGY::FileSpec spec; spec.usePar( iop );
    setSpec( spec );
}


void uiSEGYFileSpec::use( const IOObj* ioobj, bool force )
{
    SeisIOObjInfo oinf( ioobj );
    if ( !ioobj || !oinf.isOK() )
    {
	if ( !force )
	    return;
	fnmfld_->setFileName( "" );
	if ( multifld_ )
	    multifld_->setChecked( false );
	return;
    }

    if ( !force && *fnmfld_->fileName() )
	return;

    mDynamicCastGet(const IOStream*,iostrm,ioobj)
    if ( !iostrm ) { pErrMsg("Wrong IOObj type"); return; }

    BufferString dispfnm( iostrm->getExpandedName(forread_,false) );
    replaceCharacter( dispfnm.buf(), '%', '*' );
    fnmfld_->setFileName( dispfnm );
    if ( iostrm->isMulti() )
	setMultiInput( iostrm->fileNumbers(), iostrm->zeroPadding() );

    setInp2D( SeisTrcTranslator::is2D(*iostrm) );
}


void uiSEGYFileSpec::setInp2D( bool yn )
{
    is2d_ = yn;
    if ( multifld_ ) multifld_->display( !is2d_ );
}


//--- uiSEGYFilePars ----


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
	if ( isz ) { fldtxt += " "; fldtxt += SI().getZUnitString(); }
	inp = new uiGenInput( grp, fldtxt, fis );
    }

    inp->setWithCheck( true ); inp->setChecked( ispresent );
    return inp;
}


#define mDefRetrTB(clss,grp) \
    uiSeparator* sep = new uiSeparator( grp->attachObj()->parent(), \
	    		"Vert sep", false );\
    sep->attach( rightOf, grp ); \
    sep->attach( heightSameAs, grp ); \
    uiToolButton* rtb = new uiToolButton( grp->attachObj()->parent(), \
      "Retrieve setup", ioPixmap("openset.png"), mCB(this,clss,readParsPush) );\
    rtb->attach( rightOf, sep ); \
    rtb->setToolTip( "Retrieve saved setup" )

uiSEGYFilePars::uiSEGYFilePars( uiParent* p, bool forread, IOPar* iop )
    : uiSEGYDefGroup(p,"SEGY::FilePars group",forread)
    , nrsamplesfld_(0)
    , byteswapfld_(0)
    , readParsReq(this)
{
    if ( enabbyteswapwrite == -1 )
    {
	bool enab = false;
	Settings::common().getYN( sKeyEnableByteSwapWrite, enab );
	enabbyteswapwrite = enab ? 1 : 0;
    }

    uiGroup* grp = new uiGroup( this, "Main uiSEGYFilePars group" );
    if ( forread )
	nrsamplesfld_ = mkOverruleFld( grp, "Overrule SEG-Y number of samples",
				      iop, SEGY::FilePars::sKeyNrSamples(),
				      false, true );

    fmtfld_ = new uiGenInput( grp, "SEG-Y 'format'",
	    	StringListInpSpec(SEGY::FilePars::getFmts(forread)) );
    if ( nrsamplesfld_ )
	fmtfld_->attach( alignedBelow, nrsamplesfld_ );

    if ( forread || enabbyteswapwrite )
    {
	int bs = 0; iop->get( SEGY::FilePars::sKeyByteSwap(), bs );
	const char* strs[] = { "No", "Only data", "All", 0 };
	const char* txt = forread ? "Bytes swapped" : "Swap bytes";
	byteswapfld_ = new uiGenInput( grp, txt, StringListInpSpec(strs) );
	byteswapfld_->setValue( bs );
	byteswapfld_->attach( alignedBelow, fmtfld_ );
    }

    mDefRetrTB( uiSEGYFilePars, grp );

    if ( iop ) usePar( *iop );
    grp->setHAlignObj( fmtfld_ );
    setHAlignObj( grp );
}


void uiSEGYFilePars::readParsPush( CallBacker* )
{
    readParsReq.trigger();
}


SEGY::FilePars uiSEGYFilePars::getPars() const
{
    SEGY::FilePars fp( forread_ );
    if ( nrsamplesfld_ && nrsamplesfld_->isChecked() )
	fp.ns_ = nrsamplesfld_->getIntValue();
    fp.fmt_ = SEGY::FilePars::fmtOf( fmtfld_->text(), forread_ );
    fp.byteswap_ = byteswapfld_ ? byteswapfld_->getIntValue() : 0;
    return fp;
}


bool uiSEGYFilePars::fillPar( IOPar& iop, bool ) const
{
    getPars().fillPar( iop );
    return true;
}


void uiSEGYFilePars::usePar( const IOPar& iop )
{
    SEGY::FilePars fp( forread_ ); fp.usePar( iop );
    setPars( fp );
}


void uiSEGYFilePars::setPars( const SEGY::FilePars& fp )
{
    const bool havens = fp.ns_ > 0;
    if ( nrsamplesfld_ )
    {
	nrsamplesfld_->setChecked( havens );
	if ( havens ) nrsamplesfld_->setValue( fp.ns_ );
    }

    fmtfld_->setText( SEGY::FilePars::nameOfFmt(fp.fmt_,forread_) );
    if ( byteswapfld_ ) byteswapfld_->setValue( fp.byteswap_ );
}


void uiSEGYFilePars::use( const IOObj* ioobj, bool force )
{
    SEGY::FilePars fp( forread_ );
    if ( !ioobj && !force )
	return;

    if ( ioobj )
	fp.usePar( ioobj->pars() );

    setPars( fp );
}


//--- tools for building the file opts UI ----

static uiGenInput* mkPosFld( uiGroup* grp, const char* key, const IOPar* iop,
			     int def, const char* dispnm=0 )
{
    if ( iop ) iop->get( key, def );
    if ( def > 239 ) def = mUdf(int);
    IntInpSpec inpspec( def );
    inpspec.setLimits( Interval<int>( 1, 239 ) );
    BufferString fldnm( dispnm ? dispnm : key );
    if ( fldnm[0] == '[' )
	fldnm += "]";
    return new uiGenInput( grp, fldnm, inpspec.setName(key) );
}


static uiGenInput* mkByteSzFld( uiGroup* grp, const char* key,
				const IOPar* iop, int nrbytes )
{
    if ( iop ) iop->get( key, nrbytes );
    BoolInpSpec bszspec( nrbytes != 2, "4", "2" );
    uiGenInput* ret = new uiGenInput( grp, "size (bytes)",  bszspec
	    				.setName(BufferString("4",key),0)
	   				.setName(BufferString("2",key),1) );
    ret->setElemSzPol( uiObject::SmallVar );
    return ret;
}


static bool setIf( IOPar& iop, bool yn, const char* key, uiGenInput* inp,
		   bool chkbyte, Seis::GeomType gt )
{
    if ( !yn || !inp )
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
	if ( !strcmp(key,SEGY::TrcHeaderDef::stdkey) && bytenr != b && geomyn )\
	    fld = fldfill

	const char* fld = 0;
	const bool is2d = Seis::is2D( gt );
	mHndlByte(sXCoordByte(),181,true,"X-coord");
	else mHndlByte(sYCoordByte(),185,true,"Y-coord");
	else mHndlByte(sInlByte(),189,!is2d,"In-line");
	else mHndlByte(sCrlByte(),193,!is2d,"Cross-line");
	else mHndlByte(sTrNrByte(),5,is2d,sKey::TraceNr);
	else mHndlByte(sRefNrByte(),197,is2d,"RefNr");
	if ( fld )
	{
	    BufferString msg( "Please note that the byte for the " );
	    msg += fld;
	    msg += " may not be applied:\nIt clashes with standard SEG-Y "
		   "revision 1 contents\nContinue?";
	    if ( !uiMSG().askContinue(msg) )
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
    if ( !inp ) return;
    int bnr = inp->getIntValue();
    if ( iop.get(key,bnr) )
	inp->setValue( bnr );
}


static void setByteSzFld( uiGenInput* inp, const IOPar& iop, const char* key )
{
    if ( !inp ) return;
    int nr = inp->getBoolValue() ? 4 : 2;
    if ( iop.get(key,nr) )
	inp->setValue( nr != 2 );
}


static void setToggledFld( uiGenInput* inp, const IOPar& iop, const char* key,
			   bool isz=false )
{
    if ( !inp ) return;
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


//--- uiSEGYFileOpts ----

#define mDefObjs(grp) \
{ \
    mDefRetrTB(uiSEGYFileOpts,grp); \
\
    uiToolButton* stb = new uiToolButton( grp->attachObj()->parent(), \
     "Pre-scan",ioPixmap("prescan.png"),mCB(this,uiSEGYFileOpts,preScanPush) );\
    stb->attach( alignedBelow, rtb ); \
    stb->setToolTip( "Pre-scan the file(s)" ); \
\
    setHAlignObj( grp ); \
}


uiSEGYFileOpts::uiSEGYFileOpts( uiParent* p, const uiSEGYFileOpts::Setup& su,
				const IOPar* iop )
	: uiSEGYDefGroup(p,"SEG-Y Opts group",true)
    	, setup_(su)
	, xcoordbytefld_(0)
	, scalcofld_(0)
	, inlbytefld_(0)
	, crlbytefld_(0)
	, trnrbytefld_(0)
	, refnrbytefld_(0)
	, refnrbyteszfld_(0)
	, psposfld_(0)
	, posfld_(0)
	, readcoordsfld_(0)
        , timeshiftfld_(0)
	, sampleratefld_(0)
	, ensurepsxylbl_(0)
    	, isps_(Seis::isPS(su.geom_))
    	, is2d_(Seis::is2D(su.geom_))
    	, ts_(0)
    	, readParsReq(this)
    	, preScanReq(this)
{
    int nrtabs = 0;
    const bool mkpsgrp = isps_;
    const bool mkorulegrp = forread_ && setup_.revtype_ != uiSEGYRead::Rev1;
    const bool mkposgrp = is2d_ || setup_.revtype_ == uiSEGYRead::Rev0;
    const bool mkcoordgrp = mkposgrp && !is2d_;
    if ( mkpsgrp ) nrtabs++;
    if ( mkorulegrp ) nrtabs++;
    if ( mkposgrp ) nrtabs++;
    if ( mkcoordgrp ) nrtabs++;
    if ( nrtabs < 1 )
	return;
    else if ( nrtabs > 1 )
	ts_ = new uiTabStack( this, "SEG-Y definition tab stack" );

    SEGY::TrcHeaderDef thdef; thdef.fromSettings();
    posgrp_ = mkposgrp ? mkPosGrp( iop, thdef ) : 0;
    psgrp_ = mkpsgrp ? mkPSGrp( iop, thdef ) : 0;
    orulegrp_ = mkorulegrp ? mkORuleGrp( iop ) : 0;
    coordgrp_ = mkcoordgrp ? mkCoordGrp( iop, thdef ) : 0;

    if ( ts_ )
    {
	ts_->tabGroup()->setHAlignObj( posgrp_ ? posgrp_
				       : (psgrp_ ? psgrp_ : orulegrp_) );
	ts_->setHAlignObj( ts_->tabGroup() );
	mDefObjs( ts_ )
    }
    else if ( posgrp_ )
	mDefObjs( posgrp_ )
    else if ( psgrp_ )
	mDefObjs( psgrp_ )
    else if ( orulegrp_ )
	mDefObjs( orulegrp_ )
    else if ( coordgrp_ )
	mDefObjs( coordgrp_ )

    mainwin()->finaliseStart.notify( mCB(this,uiSEGYFileOpts,initFlds) );
}


uiSEGYFileOpts::~uiSEGYFileOpts()
{
}


void uiSEGYFileOpts::initFlds( CallBacker* cb )
{
    psPosChg( cb );
    crdChk( cb );
}


void uiSEGYFileOpts::readParsPush( CallBacker* )
{
    readParsReq.trigger();
}


void uiSEGYFileOpts::preScanPush( CallBacker* )
{
    preScanReq.trigger();
}


void uiSEGYFileOpts::crdChk( CallBacker* )
{
    if ( !readcoordsfld_ ) return;
    const bool ischckd = xcoordbytefld_->isChecked();
    const bool isfile = readcoordsfld_->getBoolValue();

    ycoordbytefld_->display( ischckd );
    readcoordsfld_->display( !ischckd );
    coordsfnmfld_->display( !ischckd && isfile );
    coordsstartfld_->display( !ischckd && !isfile );
    coordsstepfld_->display( !ischckd && !isfile );
    if ( refnrbytefld_ )
    {
	refnrbytefld_->display( ischckd );
	refnrbyteszfld_->display( ischckd );
    }
}


void uiSEGYFileOpts::getReport( IOPar& iop ) const
{
    BufferString tmp;
#   define mGetIndexNrByteRep(s,dir) { \
    tmp = dir##bytefld_->getIntValue(); \
    tmp += " ("; tmp += dir##byteszfld_->text(); tmp += " bytes)"; \
    iop.set( s, tmp ); }

    if ( psgrp_ )
    {
	const int pspostyp = psPosType();
	if ( psposfld_ )
	    iop.set( "Offsets/azimuths", psposfld_->text() );
	if ( pspostyp == 0 )
	{
	    mGetIndexNrByteRep("Offset byte",offs);
	    mGetIndexNrByteRep("Azimuth byte",azim);
	}
	else if ( pspostyp == 2 )
	{
	    tmp = "Start at ";
	    tmp += regoffsfld_->text(0); tmp += " then step ";
	    tmp += regoffsfld_->text(1);
	    iop.set( "Create offsets", tmp );
	}
    }
    if ( orulegrp_ )
    {
#	define mGetOverruleRep(s,fldnm) \
	if ( fldnm##fld_->isChecked() ) iop.set( s, fldnm##fld_->text() )
	mGetOverruleRep("Overrule.coordinate scaling",scalco);
	mGetOverruleRep("Overrule.start time",timeshift);
	mGetOverruleRep("Overrule.sample interval",samplerate);
    }

    if ( trnrbytefld_ )
    {
	mGetIndexNrByteRep("Trace number byte",trnr)
	mGetIndexNrByteRep("Reference number byte",refnr)
    }
    else if ( inlbytefld_ )
    {
	mGetIndexNrByteRep("In-line byte",inl)
	mGetIndexNrByteRep("Cross-line byte",crl)
    }
    if ( xcoordbytefld_ )
    {
	iop.set( "X-coord byte", xcoordbytefld_->getIntValue() );
	iop.set( "Y-coord byte", ycoordbytefld_->getIntValue() );
    }
}


void uiSEGYFileOpts::mkTrcNrFlds( uiGroup* grp, const IOPar* iop,
				   const SEGY::TrcHeaderDef& thdef )
{
    trnrbytefld_ = mkPosFld( grp, SEGY::TrcHeaderDef::sTrNrByte(), iop,
			     thdef.trnr );
    trnrbyteszfld_ = mkByteSzFld( grp, SEGY::TrcHeaderDef::sTrNrByteSz(),
				iop, thdef.trnrbytesz );
    trnrbyteszfld_->attach( rightOf, trnrbytefld_ );
}


void uiSEGYFileOpts::mkBinIDFlds( uiGroup* grp, const IOPar* iop,
				   const SEGY::TrcHeaderDef& thdef )
{
    inlbytefld_ = mkPosFld( grp, SEGY::TrcHeaderDef::sInlByte(), iop,
			   thdef.inl );
    inlbyteszfld_ = mkByteSzFld( grp, SEGY::TrcHeaderDef::sInlByteSz(),
				iop, thdef.inlbytesz );
    inlbyteszfld_->attach( rightOf, inlbytefld_ );
    crlbytefld_ = mkPosFld( grp, SEGY::TrcHeaderDef::sCrlByte(), iop,
			   thdef.crl );
    crlbytefld_->attach( alignedBelow, inlbytefld_ );
    crlbyteszfld_ = mkByteSzFld( grp, SEGY::TrcHeaderDef::sCrlByteSz(),
				iop, thdef.crlbytesz );
    crlbyteszfld_->attach( rightOf, crlbytefld_ );

    if ( !forScan() )
    {
	int icopt = 0;
	if ( iop ) iop->get( SEGY::FileReadOpts::sKeyICOpt(), icopt );
	posfld_ = new uiGenInput( grp, "Base positioning on",
		    BoolInpSpec(icopt>=0,"Inline/Crossline","Coordinates") );
	posfld_->attach( alignedBelow, crlbytefld_ );
    }
}


void uiSEGYFileOpts::mkCoordFlds( uiGroup* grp, const IOPar* iop,
				   const SEGY::TrcHeaderDef& thdef )
{
    xcoordbytefld_ = mkPosFld( grp, SEGY::TrcHeaderDef::sXCoordByte(),
				iop, thdef.xcoord );
    ycoordbytefld_ = mkPosFld( grp, SEGY::TrcHeaderDef::sYCoordByte(),
			      iop, thdef.ycoord );
    ycoordbytefld_->attach( alignedBelow, xcoordbytefld_ );
    if ( is2d_ )
    {
	xcoordbytefld_->attach( alignedBelow, trnrbytefld_ );
	if ( forread_ && !isps_ )
	{
	    xcoordbytefld_->setWithCheck( true );
	    xcoordbytefld_->setChecked( true );
	    xcoordbytefld_->checked.notify( mCB(this,uiSEGYFileOpts,crdChk) );
	    readcoordsfld_ = new uiGenInput( grp, "Coordinate source",
			     BoolInpSpec(false,"'Nr X Y' file","Generate") );
	    readcoordsfld_->attach( alignedBelow, xcoordbytefld_ );
	    readcoordsfld_->valuechanged.notify(
		    			mCB(this,uiSEGYFileOpts,crdChk) );
	    coordsfnmfld_ = new uiFileInput( grp, "File name",
			uiFileInput::Setup(uiFileDialog::Gen)
			.forread(forread_) );
	    coordsfnmfld_->attach( alignedBelow, readcoordsfld_ );
	    coordsstartfld_ = new uiGenInput( grp, "Start coordinate",
		    			DoubleInpSpec(), DoubleInpSpec() );
	    coordsstartfld_->attach( alignedBelow, readcoordsfld_ );
	    coordsstartfld_->setElemSzPol( uiObject::Small );
	    coordsstepfld_ = new uiGenInput( grp, "Step",
			DoubleInpSpec(SI().crlDistance()), DoubleInpSpec(0) );
	    coordsstepfld_->attach( rightOf, coordsstartfld_ );
	    coordsstepfld_->setElemSzPol( uiObject::Small );
	}
    }
}


#define mDeclGroup(s) \
    uiGroup* grp; \
    if ( ts_ ) \
	grp = new uiGroup( ts_->tabGroup(), s ); \
    else \
	grp = new uiGroup( this, s )

uiGroup* uiSEGYFileOpts::mkPosGrp( const IOPar* iop,
				   const SEGY::TrcHeaderDef& thdef )
{
    mDeclGroup( "Position group" );

    if ( !is2d_ )
    {
	mkBinIDFlds( grp, iop, thdef );
	grp->setHAlignObj( inlbytefld_ );
    }
    else
    {
	mkTrcNrFlds( grp, iop, thdef );
	if ( setup_.revtype_ == uiSEGYRead::Rev0 )
	{
	    mkCoordFlds( grp, iop, thdef );
	    refnrbytefld_ = mkPosFld( grp, SEGY::TrcHeaderDef::sRefNrByte(),
				      iop, thdef.refnr, "[Ref/SP number" );
	    refnrbytefld_->attach( alignedBelow, ycoordbytefld_ );
	    refnrbyteszfld_ = mkByteSzFld( grp,
		    		SEGY::TrcHeaderDef::sRefNrByteSz(),
				iop, thdef.refnrbytesz );
	    refnrbyteszfld_->attach( rightOf, refnrbytefld_ );
	}
	grp->setHAlignObj( trnrbytefld_ );
    }

    if ( ts_ ) ts_->addTab( grp, "Locations" );
    return grp;
}


uiGroup* uiSEGYFileOpts::mkCoordGrp( const IOPar* iop,
				     const SEGY::TrcHeaderDef& thdef )
{
    if ( is2d_ ) return 0;
    mDeclGroup( "Coordinates group" );

    mkCoordFlds( grp, iop, thdef );
    grp->setHAlignObj( xcoordbytefld_ );

    if ( ts_ ) ts_->addTab( grp, "Coordinates" );
    return grp;
}


uiGroup* uiSEGYFileOpts::mkORuleGrp( const IOPar* iop )
{
    mDeclGroup( "Overrule group" );

    scalcofld_ = mkOverruleFld( grp,
		    "Overrule SEG-Y coordinate scaling", iop,
		    SEGY::FileReadOpts::sKeyCoordScale(), false );
    BufferString overrulestr = "Overrule SEG-Y start ";
    overrulestr += SI().zIsTime() ? "time" : "depth";
    timeshiftfld_ = mkOverruleFld( grp, overrulestr, iop,
			    SEGY::FileReadOpts::sKeyTimeShift(),
			    true );
    timeshiftfld_->attach( alignedBelow, scalcofld_ );
    sampleratefld_ = mkOverruleFld( grp, "Overrule SEG-Y sample rate", iop,
			    SEGY::FileReadOpts::sKeySampleIntv(),
			    true );
    sampleratefld_->attach( alignedBelow, timeshiftfld_ );

    grp->setHAlignObj( scalcofld_ );
    if ( ts_ ) ts_->addTab( grp, "Overrules" );
    return grp;
}


uiGroup* uiSEGYFileOpts::mkPSGrp( const IOPar* iop,
				  const SEGY::TrcHeaderDef& thdef )
{
    mDeclGroup( "PS group" );

    static const char* choices[] = {
	"In file", "From src/rcv coordinates", "Not present", 0 };
    if ( forread_ )
    {
	psposfld_ = new uiGenInput( grp, "Offsets/azimuths",
				    StringListInpSpec(choices) );
	psposfld_->valuechanged.notify( mCB(this,uiSEGYFileOpts,psPosChg) );
    }

    offsbytefld_ = mkPosFld( grp, SEGY::TrcHeaderDef::sOffsByte(), iop,
			    thdef.offs );
    if ( psposfld_ ) offsbytefld_->attach( alignedBelow, psposfld_ );
    offsbyteszfld_ = mkByteSzFld( grp, SEGY::TrcHeaderDef::sOffsByteSz(),
				iop, thdef.offsbytesz );
    offsbyteszfld_->attach( rightOf, offsbytefld_ );
    azimbytefld_ = mkPosFld( grp, SEGY::TrcHeaderDef::sAzimByte(), iop,
			    thdef.azim, "Azimuth byte (empty=none)" );
    azimbyteszfld_ = mkByteSzFld( grp, SEGY::TrcHeaderDef::sAzimByteSz(),
				iop, thdef.azimbytesz );
    azimbyteszfld_->attach( rightOf, azimbytefld_ );
    azimbytefld_->attach( alignedBelow, offsbytefld_ );

    if ( forread_ )
    {
	ensurepsxylbl_ = new uiLabel( grp,"Please be sure that the fields\n"
			"at bytes 73, 77 (source) and 81, 85 (receiver)\n"
		       "actually contain the right coordinates" );
	ensurepsxylbl_->attach( ensureBelow, psposfld_ );

	const float inldist = SI().inlDistance();
	regoffsfld_ = new uiGenInput( grp, "Set offsets to: start/step",
			    IntInpSpec(0), IntInpSpec(mNINT(inldist)) );
	regoffsfld_->attach( alignedBelow, psposfld_ );
    }

    grp->setHAlignObj( offsbytefld_ );
    if ( ts_ ) ts_->addTab( grp, "Offset/Azimuth" );
    return grp;
}


void uiSEGYFileOpts::usePar( const IOPar& iop )
{
    int icopt = 0, psopt = 0;

    if ( posfld_ )
    {
	icopt = posfld_->getBoolValue() ? 1 : -1;
	iop.get( SEGY::FileReadOpts::sKeyICOpt(), icopt );
	posfld_->setValue( icopt >= 0 );
    }
    if ( psposfld_ )
    {
	psopt = psposfld_->getIntValue();
	iop.get( SEGY::FileReadOpts::sKeyPSOpt(), psopt );
	psposfld_->setValue( psopt );
    }

    if ( xcoordbytefld_ )
    {
	setByteNrFld( xcoordbytefld_, iop, SEGY::TrcHeaderDef::sXCoordByte() );
	setByteNrFld( ycoordbytefld_, iop, SEGY::TrcHeaderDef::sYCoordByte() );
    }
    if ( inlbytefld_ )
    {
	setByteNrFld( inlbytefld_, iop, SEGY::TrcHeaderDef::sInlByte() );
	setByteSzFld( inlbyteszfld_, iop, SEGY::TrcHeaderDef::sInlByteSz() );
	setByteNrFld( crlbytefld_, iop, SEGY::TrcHeaderDef::sCrlByte() );
	setByteSzFld( crlbyteszfld_, iop, SEGY::TrcHeaderDef::sCrlByteSz() );
    }
    if ( trnrbytefld_ )
    {
	setByteNrFld( trnrbytefld_, iop, SEGY::TrcHeaderDef::sTrNrByte() );
	setByteSzFld( trnrbyteszfld_, iop, SEGY::TrcHeaderDef::sTrNrByteSz() );
	setByteNrFld( refnrbytefld_, iop, SEGY::TrcHeaderDef::sRefNrByte() );
	setByteSzFld( refnrbyteszfld_, iop, SEGY::TrcHeaderDef::sRefNrByteSz());
    }

    if ( orulegrp_ )
    {
	setToggledFld( scalcofld_, iop, SEGY::FileReadOpts::sKeyCoordScale() );
	setToggledFld( timeshiftfld_, iop,
		       SEGY::FileReadOpts::sKeyTimeShift(), true );
	setToggledFld( sampleratefld_, iop,
		       SEGY::FileReadOpts::sKeySampleIntv(), true );
    }

    if ( isps_ )
    {
	if ( psopt == 0 )
	{
	    setByteNrFld( offsbytefld_, iop, SEGY::TrcHeaderDef::sOffsByte() );
	    setByteSzFld( offsbyteszfld_,iop,SEGY::TrcHeaderDef::sOffsByteSz());
	    setByteNrFld( azimbytefld_, iop, SEGY::TrcHeaderDef::sAzimByte() );
	    setByteSzFld( azimbyteszfld_,iop,SEGY::TrcHeaderDef::sAzimByteSz());
	}
	else if ( psopt == 2 )
	{
	    int start = regoffsfld_->getIntValue(0);
	    int step = regoffsfld_->getIntValue(1);
	    iop.get( SEGY::FileReadOpts::sKeyOffsDef(), start, step );
	    regoffsfld_->setValue( start, 0 );
	    regoffsfld_->setValue( step, 1 );
	}
    }

    if ( psgrp_ )
	psPosChg(0);

    if ( readcoordsfld_ )
    {
#define mCoordOptVal \
	(xcoordbytefld_->isChecked() ? 0 \
				: (readcoordsfld_->getBoolValue() ? 1 : 2))
	const char* res = iop.find( SEGY::FileReadOpts::sKeyCoordOpt() );
	int coordopt = res && *res ? toInt(res) : mCoordOptVal;
	if ( res && *res )
	    coordopt = toInt( res );
	xcoordbytefld_->setChecked( coordopt < 1 || coordopt > 2 );
	readcoordsfld_->setValue( coordopt == 1 );

	Coord crd( coordsstartfld_->getCoord() );
	iop.get( SEGY::FileReadOpts::sKeyCoordStart(), crd );
	coordsstartfld_->setValue( crd );
	crd = coordsstepfld_->getCoord();
	iop.get( SEGY::FileReadOpts::sKeyCoordStep(), crd );
	coordsstepfld_->setValue( crd );
	BufferString fnm( coordsfnmfld_->fileName() );
	iop.get( SEGY::FileReadOpts::sKeyCoordFileName(), fnm );
	coordsfnmfld_->setFileName( fnm );
	crdChk( 0 );
    }
}


void uiSEGYFileOpts::use( const IOObj* ioobj, bool force )
{
    //TODO don't ignore force
    if ( ioobj )
	usePar( ioobj->pars() );
}


int uiSEGYFileOpts::psPosType() const
{
    return psposfld_ ? psposfld_->getIntValue() : 0;
}


void uiSEGYFileOpts::psPosChg( CallBacker* c )
{
    if ( !psgrp_ ) return;

    const int pspostyp = psPosType();
    offsbytefld_->display( pspostyp == 0 );
    offsbyteszfld_->display( pspostyp == 0 );
    azimbytefld_->display( pspostyp == 0 );
    azimbyteszfld_->display( pspostyp == 0 );
    ensurepsxylbl_->display( pspostyp == 1 );
    regoffsfld_->display( pspostyp == 2 );
}


bool uiSEGYFileOpts::fillPar( IOPar& iop, bool perm ) const
{
    iop.setYN( SeisTrcTranslator::sKeyIs2D(), is2d_ );
    iop.setYN( SeisTrcTranslator::sKeyIsPS(), isps_ );

    iop.set( SEGY::FileReadOpts::sKeyICOpt(),
	     !posfld_ ? 0 : (posfld_->getBoolValue() ? 1 : -1) );

#define mSetByteIf(yn,key,fld) \
    if ( !setIf(iop,yn,SEGY::TrcHeaderDef::key,fld,!forread_,setup_.geom_) \
	&& !perm ) return false
#define mSetSzIf(yn,key,fld) \
    if ( !setIf(iop,yn,SEGY::TrcHeaderDef::key,fld,false,setup_.geom_) \
	&& !perm ) return false

    mSetByteIf( inlbytefld_, sInlByte(), inlbytefld_ );
    mSetSzIf( inlbytefld_, sInlByteSz(), inlbyteszfld_ );
    mSetByteIf( crlbytefld_, sCrlByte(), crlbytefld_ );
    mSetSzIf( crlbytefld_, sCrlByteSz(), crlbyteszfld_ );
    mSetByteIf( xcoordbytefld_, sXCoordByte(), xcoordbytefld_ );
    mSetByteIf( xcoordbytefld_, sYCoordByte(), ycoordbytefld_ );
    mSetByteIf( trnrbytefld_, sTrNrByte(), trnrbytefld_ );
    mSetSzIf( trnrbytefld_, sTrNrByteSz(), trnrbyteszfld_ );
    mSetByteIf( refnrbytefld_, sRefNrByte(), refnrbytefld_ );
    mSetSzIf( refnrbytefld_, sRefNrByteSz(), refnrbyteszfld_ );

    if ( psgrp_ )
    {
	const int pspostyp = psPosType();
	iop.set( SEGY::FileReadOpts::sKeyPSOpt(), pspostyp );
	    mSetByteIf( pspostyp == 0, sOffsByte(), offsbytefld_ );
	    mSetSzIf( pspostyp == 0, sOffsByteSz(), offsbyteszfld_ );
	    mSetByteIf( pspostyp == 0, sAzimByte(), azimbytefld_ );
	    mSetSzIf( pspostyp == 0, sAzimByteSz(), azimbyteszfld_ );
	if ( pspostyp == 2 )
	    iop.set( SEGY::FileReadOpts::sKeyOffsDef(),
		regoffsfld_->getIntValue(0), regoffsfld_->getIntValue(1) );
	else
	    iop.removeWithKey( SEGY::FileReadOpts::sKeyOffsDef() );
    }

    if ( !orulegrp_ )
    {
	iop.removeWithKey( SEGY::FileReadOpts::sKeyCoordScale() );
	iop.removeWithKey( SEGY::FileReadOpts::sKeyTimeShift() );
	iop.removeWithKey( SEGY::FileReadOpts::sKeySampleIntv() );
    }
    else
    {
	setToggled( iop, SEGY::FileReadOpts::sKeyCoordScale(), scalcofld_ );
	setToggled( iop, SEGY::FileReadOpts::sKeyTimeShift(),
			   timeshiftfld_, true );
	setToggled( iop, SEGY::FileReadOpts::sKeySampleIntv(),
			   sampleratefld_, true );
    }

    iop.removeWithKey( SEGY::FileReadOpts::sKeyCoordStart() );
    iop.removeWithKey( SEGY::FileReadOpts::sKeyCoordStep() );
    iop.removeWithKey( SEGY::FileReadOpts::sKeyCoordFileName() );
    if ( !readcoordsfld_ )
	iop.removeWithKey( SEGY::FileReadOpts::sKeyCoordOpt() );
    else
    {
	const int opt = mCoordOptVal;
	iop.set( SEGY::FileReadOpts::sKeyCoordOpt(), opt );
	if ( opt == 1 )
	    iop.set( SEGY::FileReadOpts::sKeyCoordFileName(),
		     coordsfnmfld_->fileName() );
	else if ( opt == 2 )
	{
	    iop.set( SEGY::FileReadOpts::sKeyCoordStart(),
		     coordsstartfld_->getCoord() );
	    iop.set( SEGY::FileReadOpts::sKeyCoordStep(),
		     coordsstepfld_->getCoord() );
	}
    }

    if ( setup_.revtype_ == uiSEGYRead::Rev0 )
	iop.setYN( SEGY::FileDef::sKeyForceRev0(), true );

    return true;
}
