/*
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert Bril
 Date:          Sep 2008
________________________________________________________________________

-*/
static const char* rcsID mUnusedVar = "$Id: uisegydef.cc,v 1.51 2012-07-10 08:05:36 cvskris Exp $";

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
#include "strmoper.h"
#include "strmprov.h"
#include "seisioobjinfo.h"

#include "uifileinput.h"
#include "uisegymanip.h"
#include "uilabel.h"
#include "uiseparator.h"
#include "uimainwin.h"
#include "uitoolbutton.h"
#include "uispinbox.h"
#include "uitabstack.h"
#include "uilineedit.h"
#include "uibutton.h"
#include "uimsg.h"

static const char* sKeyEnableByteSwapWrite = "Enable SEG-Y byte swap writing";
static int enabbyteswapwrite = -1;
static BufferString lastreaddir;
static BufferString lastwritedir;


//--- uiSEGYFileSpec ----


uiSEGYFileSpec::uiSEGYFileSpec( uiParent* p, const uiSEGYFileSpec::Setup& su )
    : uiSEGYDefGroup(p,"SEGY::FileSpec group",su.forread_)
    , multifld_(0)
    , is2d_(!su.canbe3d_)
    , manipbut_(0)
    , needmulti_(su.forread_ && su.needmultifile_)
    , swpd_(false)
    , fileSelected(this)
{
    SEGY::FileSpec spec; if ( su.pars_ ) spec.usePar( *su.pars_ );

    BufferString disptxt( forread_ ? "Input" : "Output" );
    disptxt += " SEG-Y file";
    if ( needmulti_ ) disptxt += "(s)";
    fnmfld_ = new uiFileInput( this, disptxt,
		uiFileInput::Setup(uiFileDialog::Gen)
		.forread(forread_).filter(fileFilter()) );
    BufferString defdir( forread_ ? lastreaddir : lastwritedir );
    if ( defdir.isEmpty() ) defdir = GetDataDir();
    fnmfld_->setDefaultSelectionDir( defdir );
    fnmfld_->valuechanged.notify( mCB(this,uiSEGYFileSpec,fileSel) );
    if ( forread_ )
    {
	manipbut_ = new uiPushButton( this, "&Manipulate",
			  mCB(this,uiSEGYFileSpec,manipFile), false );
	manipbut_->attach( rightOf, fnmfld_ );
	manipbut_->setSensitive( false );
    }

    if ( needmulti_ )
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
    if ( multifld_ && multifld_->isChecked() )
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

    multifld_->display( !is2d_ || needmulti_ );
}


void uiSEGYFileSpec::setFileName( const char* fnm )
{
    fnmfld_->setFileName( fnm );
    fileSel( 0 );
}


void uiSEGYFileSpec::setSpec( const SEGY::FileSpec& spec )
{
    setFileName( spec.fname_ );
    setMultiInput( spec.nrs_, spec.zeropad_ );
    fileSel( 0 );
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
	setFileName( "" );
	if ( multifld_ )
	    multifld_->setChecked( false );
	fileSel( 0 );
	return;
    }

    if ( !force && *fnmfld_->fileName() )
	return;

    mDynamicCastGet(const IOStream*,iostrm,ioobj)
    if ( !iostrm ) { pErrMsg("Wrong IOObj type"); return; }

    BufferString dispfnm( iostrm->getExpandedName(forread_,false) );
    replaceCharacter( dispfnm.buf(), '%', '*' );
    setFileName( dispfnm );
    if ( iostrm->isMulti() )
	setMultiInput( iostrm->fileNumbers(), iostrm->zeroPadding() );

    setInp2D( SeisTrcTranslator::is2D(*iostrm) );
}


void uiSEGYFileSpec::setInp2D( bool yn )
{
    is2d_ = yn;
    if ( multifld_ ) multifld_->display( needmulti_ || !is2d_ );
}


void uiSEGYFileSpec::fileSel( CallBacker* )
{
    if ( !forread_ ) return;

    const SEGY::FileSpec spec( getSpec() );
    const char* fnm = spec.getFileName();
    StreamData sd( StreamProvider(fnm).makeIStream() );
    const bool doesexist = sd.usable();
    manipbut_->setSensitive( doesexist );
    if ( !doesexist )
	return;

    StrmOper::seek( *sd.istrm, 3200, std::ios::beg );
    unsigned char buf[400];
    StrmOper::readBlock( *sd.istrm, buf, 400 );
    sd.close();
    SEGY::BinHeader bh; bh.setInput( buf );
    bh.guessIsSwapped();
    swpd_ = bh.isSwapped();

    fileSelected.trigger();
}


void uiSEGYFileSpec::manipFile( CallBacker* )
{
    const SEGY::FileSpec spec( getSpec() );
    uiSEGYFileManip dlg( this, spec.getFileName() );
    if ( dlg.go() )
	setFileName( dlg.fileName() );
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
	IntInpSpec iis( ispresent ? mNINT32(val)
				  : SI().zRange(false).nrSteps() + 1 );
	inp = new uiGenInput( grp, txt, iis );
    }
    else
    {
	if ( !mIsUdf(val) && isz ) val *= SI().zDomain().userFactor();
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
      "openset", "Retrieve saved setup", mCB(this,clss,readParsPush) );\
    rtb->attach( rightOf, sep )

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


void uiSEGYFilePars::setBytesSwapped( bool yn )
{
    if ( !byteswapfld_ ) return;
    byteswapfld_->setValue( yn ? 2 : 0 );
}


//--- class for building the file opts UI ----

class uiSEGYFOByteSpec : public uiGroup
{
public:

uiSEGYFOByteSpec( uiParent* p, SEGY::HdrEntry& he, bool wsz, const IOPar& iop,
		  bool isopt, const char* ky, bool wfr )
    : uiGroup(p,he.name())
    , he_(he)
    , isselbox_(0)
    , issmallfld_(0)
    , key_(ky)
    , checked(this)
{
    he_.usePar( iop, ky );
    if ( wfr ) setFrame( wfr );

    const BufferString fldnm( he.name(), " byte" );
    bytefld_ = new uiSpinBox( this, 0, BufferString(fldnm," value") );
    if ( !isopt )
	new uiLabel( this, fldnm, bytefld_ );
    else
    {
	isselbox_ = new uiCheckBox( this, fldnm );
	bytefld_->attach( rightOf, isselbox_ );
	isselbox_->setChecked( true );
	isselbox_->activated.notify( mCB(this,uiSEGYFOByteSpec,byteChck) );
    }
    bytefld_->setInterval( StepInterval<int>(1,239,2) );
    bytefld_->setValue( he_.isUdf() ? 1 : (int)he_.bytepos_+1 );

    if ( wsz )
    {
	BoolInpSpec bszspec( !he_.small_, "4", "2" );
	issmallfld_ = new uiCheckBox( this, " 2 bytes" );
	issmallfld_->attach( rightOf, bytefld_ );
    }

    setHAlignObj( bytefld_ );
}

bool fillPar( IOPar& iop ) const
{
    if ( !isChecked() )
	he_.removeFromPar( iop, key_ );
    else if ( !const_cast<uiSEGYFOByteSpec*>(this)->getVals() )
	return false;
    else
	he_.fillPar( iop, key_ );
    return true;
}

void usePar( const IOPar& iop )
{
    const char* res = iop.find( key_ );
    he_.usePar( iop, key_ );
    if ( he_.isUdf() || (isselbox_ && !iop.find(key_)) )
    {
	if ( isselbox_ )
	    isselbox_->setChecked( false );
    }
    else
    {
	if ( isselbox_ && !isselbox_->isChecked() )
	    isselbox_->setChecked( true );
	bytefld_->setValue( he_.bytepos_+1 );
	if ( issmallfld_ )
	    issmallfld_->setChecked( he_.small_ );
    }
}

void byteChck( CallBacker* )
{
    const bool issel = isChecked();
    bytefld_->setSensitive( issel );
    if ( issmallfld_ )
	issmallfld_->setSensitive( issel );
    checked.trigger();
}

bool isChecked() const
{
    return !isselbox_ || isselbox_->isChecked();
}

void setChecked( bool yn ) const
{
    if ( isselbox_ )
	isselbox_->setChecked( yn );
}

int byteNr() const
{
    return isChecked() ? bytefld_->getValue()-1 : -1;
}

inline bool isSmall() const
{
    return issmallfld_ && issmallfld_->isChecked();
}

inline int byteSize() const
{
    return isSmall() ? 2 : 4;
}

void addToReport( IOPar& iop ) const
{
    BufferString key( key_, " byte" );
    BufferString val;
    if ( !isChecked() )
	val = "-";
    else
	val.add(byteNr()).add(" (size: ").add(byteSize()).add(" bytes)");
    iop.set( key, val );
}

bool getVals()
{
    const int bytenr = byteNr();
    const int bytesz = byteSize();
    if ( bytenr+bytesz >= 115 && bytenr < 119 )
    {
	uiMSG().error( "Bytes 115-118 (number of samples and sample rate)\n"
			"must never be redefined" );
	return false;
    }

    if ( bytenr < 0 )
	he_.setUdf();
    else
    {
	he_.bytepos_ = (SEGY::HdrEntry::BytePos)(bytenr + 1);
	he_.small_ = isSmall();
    }

    return true;
}

    SEGY::HdrEntry&		he_;
    BufferString		key_;
    uiSpinBox*			bytefld_;
    uiCheckBox*			isselbox_;
    uiCheckBox*			issmallfld_;
    Notifier<uiSEGYFOByteSpec>	checked;

}; // end class uiSEGYFOByteSpec


//--- uiSEGYFileOpts ----

#define mDefObjs(grp) \
{ \
    mDefRetrTB(uiSEGYFileOpts,grp); \
\
    uiToolButton* stb = new uiToolButton( grp->attachObj()->parent(), \
			     "prescan","Pre-scan the file(s)", \
				    mCB(this,uiSEGYFileOpts,preScanPush) );\
    stb->attach( alignedBelow, rtb ); \
\
    setHAlignObj( grp ); \
}


uiSEGYFileOpts::uiSEGYFileOpts( uiParent* p, const uiSEGYFileOpts::Setup& su,
				const IOPar* iop )
	: uiSEGYDefGroup(p,"SEG-Y Opts group",true)
    	, setup_(su)
	, scalcofld_(0)
	, xcoorddeffld_(0)
	, ycoorddeffld_(0)
	, inldeffld_(0)
	, crldeffld_(0)
	, trnrdeffld_(0)
	, refnrdeffld_(0)
	, offsdeffld_(0)
	, azimdeffld_(0)
	, psposfld_(0)
	, posfld_(0)
	, havecoordsinhdrfld_(0)
        , timeshiftfld_(0)
	, sampleratefld_(0)
	, ensurepsxylbl_(0)
    	, isps_(Seis::isPS(su.geom_))
    	, is2d_(Seis::is2D(su.geom_))
    	, ts_(0)
    	, thdef_(*new SEGY::TrcHeaderDef)
    	, readParsReq(this)
    	, preScanReq(this)
{
    thdef_.fromSettings();
    IOPar emptyiop;
    if ( !iop ) iop = &emptyiop;

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

    posgrp_ = mkposgrp ? mkPosGrp( *iop ) : 0;
    psgrp_ = mkpsgrp ? mkPSGrp( *iop ) : 0;
    orulegrp_ = mkorulegrp ? mkORuleGrp( *iop ) : 0;
    coordgrp_ = mkcoordgrp ? mkCoordGrp( *iop ) : 0;

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

    preFinalise().notify( mCB(this,uiSEGYFileOpts,initFlds) );
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
    if ( !havecoordsinhdrfld_ ) return;
    const bool havecoords = havecoordsinhdrfld_->getBoolValue();
    const bool isfile = readcoordsfld_->getBoolValue();
    const bool isextonly = isfile && !coordsspecfnmbox_->isChecked();

    xcoorddeffld_->display( havecoords );
    ycoorddeffld_->display( havecoords );
    readcoordsfld_->display( !havecoords );
    coordsspecfnmbox_->display( !havecoords && isfile );
    coordsstartfld_->display( !havecoords && !isfile );
    coordsstepfld_->display( !havecoords && !isfile );
    coordsfnmfld_->display( !havecoords && isfile && !isextonly );
    coordsextfld_->display( !havecoords && isfile && isextonly );
}


void uiSEGYFileOpts::getReport( IOPar& iop ) const
{
    if ( psgrp_ )
    {
	const int pspostyp = psPosType();
	if ( psposfld_ )
	    iop.set( "Offsets/azimuths", psposfld_->text() );
	if ( pspostyp == 0 )
	{
	    offsdeffld_->addToReport( iop );
	    azimdeffld_->addToReport( iop );
	}
	else if ( pspostyp == 2 )
	{
	    BufferString tmp( "Start at " );
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

    if ( trnrdeffld_ )
    {
	trnrdeffld_->addToReport( iop );
	if ( refnrdeffld_ )
	    refnrdeffld_->addToReport( iop );
    }
    else if ( inldeffld_ )
    {
	inldeffld_->addToReport( iop );
	crldeffld_->addToReport( iop );
    }
    if ( xcoorddeffld_ )
    {
	xcoorddeffld_->addToReport( iop );
	ycoorddeffld_->addToReport( iop );
    }
}


void uiSEGYFileOpts::mkBinIDFlds( uiGroup* grp, const IOPar& iop )
{
#define mMkDefFld(p,fldnm,kystr,wsz,opt,wfr) \
    fldnm##deffld_ = new uiSEGYFOByteSpec( p, thdef_.fldnm##_, wsz, iop, opt, \
	   		 SEGY::TrcHeaderDef::s##kystr##Byte(), wfr )

    mMkDefFld( grp, inl, Inl, true, false, true );
    mMkDefFld( grp, crl, Crl, true, false, true );
    crldeffld_->attach( alignedBelow, inldeffld_ );

    if ( !forScan() )
    {
	int icopt = 0;
	iop.get( SEGY::FileReadOpts::sKeyICOpt(), icopt );
	posfld_ = new uiGenInput( grp, "Base positioning on",
		    BoolInpSpec(icopt>=0,"Inline/Crossline","Coordinates") );
	posfld_->attach( alignedBelow, crldeffld_ );
    }
}


void uiSEGYFileOpts::mkCoordFlds( uiGroup* grp, const IOPar& iop )
{
    mMkDefFld( grp, xcoord, XCoord, false, false, true );
    mMkDefFld( grp, ycoord, YCoord, false, false, true );
    ycoorddeffld_->attach( alignedBelow, xcoorddeffld_ );
    if ( is2d_ )
    {
	const bool isreadpost2d = forread_ && !isps_ && is2d_;
	if ( !isreadpost2d )
	    xcoorddeffld_->attach( alignedBelow, trnrdeffld_ );
	else
	{
	    havecoordsinhdrfld_ = new uiGenInput( grp,
		    "Header contains coordinates", BoolInpSpec(true) );
	    havecoordsinhdrfld_->attach( alignedBelow, trnrdeffld_ );
	    havecoordsinhdrfld_->valuechanged.notify(
		    			mCB(this,uiSEGYFileOpts,crdChk) );
	    xcoorddeffld_->attach( alignedBelow, havecoordsinhdrfld_ );
	    readcoordsfld_ = new uiGenInput( grp, "Coordinate source",
			     BoolInpSpec(false,"'Nr X Y' file","Generate") );
	    readcoordsfld_->attach( alignedBelow, havecoordsinhdrfld_ );
	    readcoordsfld_->valuechanged.notify(
		    			mCB(this,uiSEGYFileOpts,crdChk) );
	    coordsfnmfld_ = new uiFileInput( grp, "Name",
			uiFileInput::Setup(uiFileDialog::Gen)
			.forread(forread_) );
	    coordsfnmfld_->attach( alignedBelow, readcoordsfld_ );
	    coordsextfld_ = new uiGenInput( grp, "Extension",
		    			    StringInpSpec("crd") );
	    coordsextfld_->attach( alignedBelow, readcoordsfld_ );
	    coordsspecfnmbox_ = new uiCheckBox( grp, "Specify file" );
	    coordsspecfnmbox_->setChecked( true );
	    coordsspecfnmbox_->attach( leftOf, coordsextfld_ );
	    coordsspecfnmbox_->activated.notify(
		    			mCB(this,uiSEGYFileOpts,crdChk) );

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

uiGroup* uiSEGYFileOpts::mkPosGrp( const IOPar& iop )
{
    mDeclGroup( "Position group" );

    if ( !is2d_ )
    {
	mkBinIDFlds( grp, iop );
	grp->setHAlignObj( inldeffld_ );
    }
    else
    {
	mMkDefFld( grp, trnr, TrNr, true, false, true );
	if ( setup_.revtype_ == uiSEGYRead::Rev0 )
	{
	    mMkDefFld( grp, refnr, RefNr, true, true, false );
	    refnrdeffld_->attach( rightOf, trnrdeffld_ );
	    mkCoordFlds( grp, iop );
	}
	grp->setHAlignObj( trnrdeffld_ );
    }

    if ( ts_ ) ts_->addTab( grp, "Locations" );
    return grp;
}


uiGroup* uiSEGYFileOpts::mkCoordGrp( const IOPar& iop )
{
    if ( is2d_ ) return 0;
    mDeclGroup( "Coordinates group" );

    mkCoordFlds( grp, iop );
    grp->setHAlignObj( xcoorddeffld_ );

    if ( ts_ ) ts_->addTab( grp, "Coordinates" );
    return grp;
}


uiGroup* uiSEGYFileOpts::mkORuleGrp( const IOPar& iop )
{
    mDeclGroup( "Overrule group" );

    scalcofld_ = mkOverruleFld( grp,
		    "Overrule SEG-Y coordinate scaling", &iop,
		    SEGY::FileReadOpts::sKeyCoordScale(), false );
    BufferString overrulestr = "Overrule SEG-Y start ";
    overrulestr += SI().zIsTime() ? "time" : "depth";
    timeshiftfld_ = mkOverruleFld( grp, overrulestr, &iop,
			    SEGY::FileReadOpts::sKeyTimeShift(),
			    true );
    timeshiftfld_->attach( alignedBelow, scalcofld_ );
    sampleratefld_ = mkOverruleFld( grp, "Overrule SEG-Y sample rate", &iop,
			    SEGY::FileReadOpts::sKeySampleIntv(),
			    true );
    sampleratefld_->attach( alignedBelow, timeshiftfld_ );

    grp->setHAlignObj( scalcofld_ );
    if ( ts_ ) ts_->addTab( grp, "Overrules" );
    return grp;
}


uiGroup* uiSEGYFileOpts::mkPSGrp( const IOPar& iop )
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

    mMkDefFld( grp, offs, Offs, true, false, true );
    if ( psposfld_ ) offsdeffld_->attach( alignedBelow, psposfld_ );
    mMkDefFld( grp, azim, Azim, true, true, false );
    azimdeffld_->attach( alignedBelow, offsdeffld_ );

    if ( forread_ )
    {
	ensurepsxylbl_ = new uiLabel( grp,"Please be sure that the fields\n"
			"at bytes 73, 77 (source) and 81, 85 (receiver)\n"
		       "actually contain the right coordinates" );
	ensurepsxylbl_->attach( ensureBelow, psposfld_ );

	const float inldist = SI().inlDistance();
	regoffsfld_ = new uiGenInput( grp, "Set offsets to: start/step",
			    IntInpSpec(0), IntInpSpec(mNINT32(inldist)) );
	regoffsfld_->attach( alignedBelow, psposfld_ );
    }

    grp->setHAlignObj( offsdeffld_ );
    if ( ts_ ) ts_->addTab( grp, "Offset/Azimuth" );
    return grp;
}


void uiSEGYFileOpts::toggledFldFillPar( uiGenInput* inp, const IOPar& iop,
					const char* key, bool isz )
{
    if ( !inp ) return;
    float val = mUdf(float);
    const bool ispresent = iop.get( key, val );
    inp->setChecked( ispresent );
    if ( ispresent )
    {
	if ( !mIsUdf(val) && isz )
	    val *= SI().zDomain().userFactor();
	inp->setValue( val );
    }
}


#define mCoordOptVal (havecoordsinhdrfld_->getBoolValue() \
		? 0 : (readcoordsfld_->getBoolValue() ? 1 : 2))


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

    if ( xcoorddeffld_ )
	{ xcoorddeffld_->usePar(iop); ycoorddeffld_->usePar(iop); }
    if ( inldeffld_ )
	{ inldeffld_->usePar(iop); crldeffld_->usePar(iop); }
    if ( trnrdeffld_ )
    {
	trnrdeffld_->usePar(iop);
	if ( refnrdeffld_ ) refnrdeffld_->usePar(iop);
    }

    if ( orulegrp_ )
    {
	toggledFldFillPar( scalcofld_, iop,
			   SEGY::FileReadOpts::sKeyCoordScale() );
	toggledFldFillPar( timeshiftfld_, iop,
			   SEGY::FileReadOpts::sKeyTimeShift(), true );
	toggledFldFillPar( sampleratefld_, iop,
			   SEGY::FileReadOpts::sKeySampleIntv(), true );
    }

    if ( isps_ )
    {
	if ( psopt == 0 )
	    { offsdeffld_->usePar(iop); azimdeffld_->usePar(iop); }
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

    if ( havecoordsinhdrfld_ )
    {
	const char* res = iop.find( SEGY::FileReadOpts::sKeyCoordOpt() );
	const int coordopt = res && *res ? toInt(res) : mCoordOptVal;
	havecoordsinhdrfld_->setValue( coordopt < 1 || coordopt > 2 );
	readcoordsfld_->setValue( coordopt == 1 );

	Coord crd( coordsstartfld_->getCoord() );
	iop.get( SEGY::FileReadOpts::sKeyCoordStart(), crd );
	coordsstartfld_->setValue( crd );
	crd = coordsstepfld_->getCoord();
	iop.get( SEGY::FileReadOpts::sKeyCoordStep(), crd );
	coordsstepfld_->setValue( crd );
	BufferString fnm( coordsfnmfld_->fileName() );
	if ( !coordsspecfnmbox_->isChecked() )
	    { fnm = "ext="; fnm.add( coordsextfld_->text() ); }
	iop.get( SEGY::FileReadOpts::sKeyCoordFileName(), fnm );
	const bool isext = matchString( "ext=", fnm );
	coordsspecfnmbox_->setChecked( !isext );
	if ( isext )
	    coordsextfld_->setText( fnm.buf() + 4 );
	else
	    coordsfnmfld_->setFileName( fnm );
    }

    crdChk(0);
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
    offsdeffld_->display( pspostyp == 0 );
    azimdeffld_->display( pspostyp == 0 );
    ensurepsxylbl_->display( pspostyp == 1 );
    regoffsfld_->display( pspostyp == 2 );
}


void uiSEGYFileOpts::setToggled( IOPar& iop, const char* key,
				 uiGenInput* inp, bool isz ) const
{
    bool isdef = inp->isChecked() && *inp->text();
    if ( !isdef )
	iop.removeWithKey( key );
    else
    {
	if ( !isz )
	    iop.set( key, inp->text() );
	else
	    iop.set( key, inp->getfValue() / SI().zDomain().userFactor() );
    }
}


bool uiSEGYFileOpts::fillPar( IOPar& iop, bool perm ) const
{
    iop.setYN( SeisTrcTranslator::sKeyIs2D(), is2d_ );
    iop.setYN( SeisTrcTranslator::sKeyIsPS(), isps_ );

    iop.set( SEGY::FileReadOpts::sKeyICOpt(),
	     !posfld_ ? 0 : (posfld_->getBoolValue() ? 1 : -1) );

#define mFillDefPar(fldnm) \
    if ( fldnm##deffld_ && !fldnm##deffld_->fillPar(iop) ) \
	return false

    mFillDefPar( inl );
    mFillDefPar( crl );
    mFillDefPar( xcoord );
    mFillDefPar( ycoord );
    mFillDefPar( trnr );
    mFillDefPar( refnr );

    if ( psgrp_ )
    {
	const int pspostyp = psPosType();
	iop.set( SEGY::FileReadOpts::sKeyPSOpt(), pspostyp );
	if ( pspostyp == 0 )
	{
	    mFillDefPar( offs );
	    mFillDefPar( azim );
	}
	else if ( pspostyp == 2 )
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
    const int opt = havecoordsinhdrfld_ ? mCoordOptVal : 0;
    if ( opt == 0 )
	iop.removeWithKey( SEGY::FileReadOpts::sKeyCoordOpt() );
    else
    {
	iop.set( SEGY::FileReadOpts::sKeyCoordOpt(), opt );
	iop.removeWithKey( SEGY::TrcHeaderDef::sXCoordByte() );
	iop.removeWithKey( SEGY::TrcHeaderDef::sYCoordByte() );
	if ( opt == 1 )
	{
	    BufferString fnm( coordsfnmfld_->fileName() );
	    if ( !coordsspecfnmbox_->isChecked() )
		{ fnm = "ext="; fnm.add( coordsextfld_->text() ); }
	    iop.set( SEGY::FileReadOpts::sKeyCoordFileName(), fnm );
	}
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
