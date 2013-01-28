/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : A.H. Bril
 * DATE     : Oct 2003
-*/

static const char* rcsID mUsedVar = "$Id$";

#include "uiseisiosimple.h"
#include "uiseisfmtscale.h"
#include "uiseissubsel.h"
#include "uiseislinesel.h"
#include "uiseissel.h"
#include "uimultcomputils.h"
#include "uifileinput.h"
#include "uiioobjsel.h"
#include "uitaskrunner.h"
#include "uiseparator.h"
#include "uiscaler.h"
#include "uilabel.h"
#include "uimsg.h"
#include "uibutton.h"
#include "seisioobjinfo.h"
#include "seistrctr.h"
#include "seispsioprov.h"
#include "seisselection.h"
#include "seisresampler.h"
#include "ctxtioobj.h"
#include "cubesampling.h"
#include "ioobj.h"
#include "iopar.h"
#include "survinfo.h"
#include "oddirs.h"
#include "file.h"
#include "filepath.h"
#include "keystrs.h"


static bool survChanged()
{
    static BufferString survnm;
    const bool issame = survnm.isEmpty() || survnm == SI().name();
    survnm = SI().name();
    return !issame;
}


#define mDefData(nm,geom) \
SeisIOSimple::Data& uiSeisIOSimple::data##nm() \
{ \
    static SeisIOSimple::Data* d = 0; \
    if ( !d ) d = new SeisIOSimple::Data( GetDataDir(), Seis::geom ); \
    return *d; \
}

mDefData(2d,Line)
mDefData(3d,Vol)
mDefData(ps,VolPS)


uiSeisIOSimple::uiSeisIOSimple( uiParent* p, Seis::GeomType gt, bool imp )
	: uiDialog( p, Setup( imp ? "Import seismics from simple flat file"
				  : "Export seismics to simple flat file",
			      "Specify parameters for I/O",
			      imp ? "103.0.11" : "103.0.12") )
	, ctio_(*uiSeisSel::mkCtxtIOObj(gt,!imp))
	, sdfld_(0)
	, havenrfld_(0)
	, haverefnrfld_(0)
	, nrdeffld_(0)
	, inldeffld_(0)
	, subselfld_(0)
	, isxyfld_(0)
	, lnmfld_(0)
	, isascfld_(0)
	, haveoffsbut_(0)
	, haveazimbut_(0)
	, pspposlbl_(0)
	, offsdeffld_(0)
	, isimp_(imp)
	, geom_(gt)
{
    setCtrlStyle( uiDialog::DoAndStay );

    data().clear( survChanged() );
    const bool is2d = is2D();
    const bool isps = isPS();

    uiSeisSel::Setup ssu( geom_ );
    uiSeparator* sep = 0;
    if ( isimp_ )
    {
	mkIsAscFld();
	fnmfld_ = new uiFileInput( this, "Input file", uiFileInput::Setup("")
			.forread( true )
			.withexamine( true ) );
	fnmfld_->attach( alignedBelow, isascfld_ );
    }
    else
    {
	ssu.steerpol(uiSeisSel::Setup::InclSteer);
	seisfld_ = new uiSeisSel( this, ctio_, ssu );
	seisfld_->selectionDone.notify( mCB(this,uiSeisIOSimple,inpSeisSel) );
	sep = mkDataManipFlds();
    }

    haveposfld_ = new uiGenInput( this,
	    		isimp_ ? "Traces start with a position"
			      : "Output a position for every trace",
	   		BoolInpSpec(true) );
    haveposfld_->setValue( data().havepos_ );
    haveposfld_->valuechanged.notify( mCB(this,uiSeisIOSimple,haveposSel) );
    haveposfld_->attach( alignedBelow, isimp_ ? fnmfld_->attachObj()
	    				     : remnullfld_->attachObj() );
    if ( sep ) haveposfld_->attach( ensureBelow, sep );

    uiObject* attachobj = haveposfld_->attachObj();
    if ( is2d )
    {
	BufferString txt( isimp_ ? "Trace number included"
				 : "Include trace number");
	txt += " (preceding X/Y)";
	havenrfld_ = new uiGenInput( this, txt, BoolInpSpec(true) );
	havenrfld_->setValue( data().havenr_ );
	havenrfld_->attach( alignedBelow, attachobj );
	havenrfld_->valuechanged.notify( mCB(this,uiSeisIOSimple,havenrSel) );
	txt = isimp_ ? "Ref/SP number included" : "Include Ref/SP number";
	txt += " (after trace number)";
	haverefnrfld_ = new uiGenInput( this, txt, BoolInpSpec(false) );
	haverefnrfld_->setValue( data().haverefnr_ );
	haverefnrfld_->attach( alignedBelow, havenrfld_ );
	havenrfld_->valuechanged.notify( mCB(this,uiSeisIOSimple,havenrSel) );
	attachobj = haverefnrfld_->attachObj();
    }
    else
    {
	isxyfld_ = new uiGenInput( this, isimp_ ? "Position in file is"
					       : "Position in file will be",
				 BoolInpSpec(true,"X Y","Inline Xline") );
	isxyfld_->setValue( data().isxy_ );
	isxyfld_->attach( alignedBelow, attachobj );
	if ( !isimp_ )
	{
	    if ( !isps )
		attachobj = isxyfld_->attachObj();
	    else
	    {
		haveoffsbut_ = new uiCheckBox( this, "Offset" );
		haveoffsbut_->setChecked( data().haveoffs_ );
		haveoffsbut_->attach( alignedBelow, isxyfld_ );
		haveazimbut_ = new uiCheckBox( this, "Azimuth" );
		haveazimbut_->setChecked( data().haveazim_ );
		haveazimbut_->attach( rightOf, haveoffsbut_ );
		pspposlbl_ = new uiLabel( this, "Include", haveoffsbut_ );
		attachobj = haveoffsbut_;
	    }
	}
    }

    if ( isimp_ )
    {
	if ( !is2d )
	{
	    inldeffld_ = new uiGenInput( this, "Inline definition: start, step",
				IntInpSpec(data().inldef_.start)
						.setName("Inl def start"),
			  	IntInpSpec(data().inldef_.step)
						.setName("Inl def step") );
	    inldeffld_->attach( alignedBelow, attachobj );
	    crldeffld_ = new uiGenInput( this,
			"Xline definition: start, step, # per inline",
			   IntInpSpec(data().crldef_.start)
			   			.setName("Crl def start"),
			   IntInpSpec(data().crldef_.step)
			   			.setName("Crl def step"),
			   IntInpSpec(data().nrcrlperinl_)
			   			.setName("per inl") );
	    crldeffld_->attach( alignedBelow, inldeffld_ );
	    attachobj = crldeffld_->attachObj();
	}
	else
	{
	    nrdeffld_ = new uiGenInput( this,
		    "Trace number definition: start, step",
		    IntInpSpec(data().nrdef_.start).setName("Trc def start"),
		    IntInpSpec(data().nrdef_.step).setName("Trc def step") );
	    nrdeffld_->attach( alignedBelow, havenrfld_ );
	    startposfld_ = new uiGenInput( this,
					  "Start position (X, Y, Trace number)",
					  PositionInpSpec(data().startpos_) );
	    startposfld_->attach( alignedBelow, haveposfld_ );
	    stepposfld_ = new uiGenInput( this, "Step in X/Y/Number",
					 PositionInpSpec(data().steppos_) );
	    stepposfld_->attach( alignedBelow, startposfld_ );
	    startnrfld_ = new uiGenInput( this, "",
		    			 IntInpSpec(data().nrdef_.start) );
	    startnrfld_->setElemSzPol( uiObject::Small );
	    startnrfld_->attach( rightOf, startposfld_ );
	    stepnrfld_ = new uiGenInput( this, "",
		    			IntInpSpec(data().nrdef_.step) );
	    stepnrfld_->setElemSzPol( uiObject::Small );
	    stepnrfld_->attach( rightOf, stepposfld_ );
	    attachobj = stepposfld_->attachObj();
	}
	if ( isps )
	{
	    haveoffsbut_ = new uiCheckBox( this, "Offset",
		    			 mCB(this,uiSeisIOSimple,haveoffsSel) );
	    haveoffsbut_->attach( alignedBelow, attachobj );
	    haveoffsbut_->setChecked( data().haveoffs_ );
	    haveazimbut_ = new uiCheckBox( this, "Azimuth" );
	    haveazimbut_->attach( rightOf, haveoffsbut_ );
	    haveazimbut_->setChecked( data().haveazim_ );
	    pspposlbl_ = new uiLabel( this, "Position includes", haveoffsbut_ );
	    const float stopoffs =
			data().offsdef_.atIndex(data().nroffsperpos_-1);
	    offsdeffld_ = new uiGenInput( this,
		    	   "Offset definition: start, stop, step",
			   FloatInpSpec(data().offsdef_.start).setName("Start"),
			   FloatInpSpec(stopoffs).setName("Stop"),
		   	   FloatInpSpec(data().offsdef_.step).setName("Step") );
	    offsdeffld_->attach( alignedBelow, haveoffsbut_ );
	    attachobj = offsdeffld_->attachObj();
	}
    }

    havesdfld_ = new uiGenInput( this, isimp_
	    			    ? "File start contains sampling info"
				    : "Put sampling info in file start",
				    BoolInpSpec(true)
				      .setName("Info in file start Yes",0)
	   			      .setName("Info in file start No",1) );
    havesdfld_->setValue( data().havesd_ );
    havesdfld_->attach( alignedBelow, attachobj );
    havesdfld_->valuechanged.notify( mCB(this,uiSeisIOSimple,havesdSel) );

    if ( isimp_ )
    {
	BufferString txt = "Sampling info: start, step ";
	txt += SI().getZUnitString(true);
	txt += " and #samples";
	SamplingData<float> sd( data().sd_ );
	if ( SI().zIsTime() )
	    { sd.start *= 1000; sd.step *= 1000; }
	sdfld_ = new uiGenInput( this, txt, 
			DoubleInpSpec(sd.start).setName("SampInfo start"),
			DoubleInpSpec(sd.step).setName("SampInfo step"),
			IntInpSpec(data().nrsamples_).setName("Nr samples") );
	sdfld_->attach( alignedBelow, havesdfld_ );
	sep = mkDataManipFlds();
	if ( !isps ) ssu.enabotherdomain( true );
	seisfld_ = new uiSeisSel( this, ctio_, ssu );
	seisfld_->attach( alignedBelow, remnullfld_ );
	if ( is2d )
	{
	    lnmfld_ = new uiSeis2DLineNameSel( this, false );
	    lnmfld_->fillWithAll();
	    lnmfld_->attach( alignedBelow, seisfld_ );
	    seisfld_->selectionDone.notify( mCB(this,uiSeisIOSimple,lsSel) );
	}
    }
    else
    {
	mkIsAscFld();
	isascfld_->attach( alignedBelow, havesdfld_ );
	fnmfld_ = new uiFileInput( this, "Output file", uiFileInput::Setup("")
			.forread( false )
			.withexamine( false ) );
	fnmfld_->attach( alignedBelow, isascfld_ );
    }

    fnmfld_->setDefaultSelectionDir( data().fname_ );
    postFinalise().notify( mCB(this,uiSeisIOSimple,initFlds) );
}


void uiSeisIOSimple::mkIsAscFld()
{
    isascfld_ = new uiGenInput( this, "File type",
	    		       BoolInpSpec(true,"Ascii","Binary") );
    isascfld_->valuechanged.notify( mCB(this,uiSeisIOSimple,isascSel) );
    isascfld_->setValue( data().isasc_ );
}


uiSeparator* uiSeisIOSimple::mkDataManipFlds()
{
    uiSeparator* sep = new uiSeparator( this, "sep inp and outp pars" );
    if ( isimp_ )
	sep->attach( stretchedBelow, sdfld_ );
    else
    {
	subselfld_ = uiSeisSubSel::get( this, Seis::SelSetup(geom_)
						.onlyrange(false) );
	subselfld_->attachObj()->attach( alignedBelow, seisfld_ );
    }

    scalefld_ = new uiScaler( this, 0, true );
    scalefld_->attach( alignedBelow, isimp_ ? sdfld_->attachObj()
	    				   : subselfld_->attachObj() );
    if ( isimp_ ) scalefld_->attach( ensureBelow, sep );
    remnullfld_ = new uiGenInput( this, "Null traces",
				 BoolInpSpec(true,"Discard","Pass") );
    remnullfld_->attach( alignedBelow, scalefld_ );

    multcompfld_ = new uiGenInput( this, "Component to export",
	    			   StringListInpSpec() );
    multcompfld_->display( false );
    multcompfld_->setSensitive( false );

    if ( !isimp_ )
    {
	multcompfld_->attach( alignedBelow, remnullfld_ );
	sep->attach( stretchedBelow, multcompfld_ );
    }

    return sep;
}


void uiSeisIOSimple::initFlds( CallBacker* cb )
{
    havesdSel( cb );
    haveposSel( cb );
    isascSel( cb );
    haveoffsSel( cb );
    if ( !isimp_ )
	inpSeisSel( cb );
}


void uiSeisIOSimple::havesdSel( CallBacker* )
{
    if ( sdfld_ )
	sdfld_->display( !havesdfld_->getBoolValue() );
}


void uiSeisIOSimple::inpSeisSel( CallBacker* )
{
    seisfld_->commitInput();
    if ( ctio_.ioobj )
    {
	subselfld_->setInput( *ctio_.ioobj );
	LineKey lkey( ctio_.ioobj->key() );
	BufferStringSet compnms;
	SeisIOObjInfo::getCompNames( lkey, compnms );
	multcompfld_->newSpec( StringListInpSpec(compnms), 0 );
	multcompfld_->display( compnms.size()>1 );
	multcompfld_->setSensitive( compnms.size()>1 );
    }
}


void uiSeisIOSimple::lsSel( CallBacker* )
{
    if ( !lnmfld_ ) return;
    seisfld_->commitInput();
    if ( ctio_.ioobj )
	lnmfld_->setLineSet( ctio_.ioobj->key() );
}



void uiSeisIOSimple::isascSel( CallBacker* )
{
    fnmfld_->enableExamine( isascfld_->getBoolValue() );
}


void uiSeisIOSimple::haveposSel( CallBacker* cb )
{
    const bool havenopos = !haveposfld_->getBoolValue();

    if ( havenrfld_ ) havenrfld_->display( !havenopos );
    if ( isxyfld_ ) isxyfld_->display( !havenopos );

    if ( isimp_ )
    {
	if ( !is2D() )
	{
	    inldeffld_->display( havenopos );
	    crldeffld_->display( havenopos );
	}
	else
	{
	    startposfld_->display( havenopos );
	    startnrfld_->display( havenopos );
	    stepposfld_->display( havenopos );
	    stepnrfld_->display( havenopos );
	}
    }

    havenrSel( cb );
    haveoffsSel( cb );
}


void uiSeisIOSimple::havenrSel( CallBacker* cb )
{
    if ( !havenrfld_ ) return;

    const bool havepos = haveposfld_->getBoolValue();
    const bool havenr = havenrfld_->getBoolValue();
    if ( nrdeffld_ )
	nrdeffld_->display( havepos && !havenr );
    if ( haverefnrfld_ )
	haverefnrfld_->display( havepos && havenr );
}


void uiSeisIOSimple::haveoffsSel( CallBacker* cb )
{
    if ( !pspposlbl_ || !haveoffsbut_ ) return;
    const bool havepos = haveposfld_->getBoolValue();
    const bool haveoffs = haveoffsbut_->isChecked();
    haveoffsbut_->display( havepos );
    haveazimbut_->display( havepos );
    pspposlbl_->display( havepos );
    if ( offsdeffld_ )
	offsdeffld_->display( !havepos || !haveoffs );
}


#define mErrRet(s) { uiMSG().error(s); return false; }

bool uiSeisIOSimple::acceptOK( CallBacker* )
{
    if ( !isascfld_ ) return true;

    BufferString fnm( fnmfld_->fileName() );
    if ( isimp_ && !File::exists(fnm) )
	mErrRet("Input file does not exist or is unreadable")
    if ( !seisfld_->commitInput() )
	mErrRet( isimp_ ? "Please choose a name for the imported data"
		       : "Please select the input seismics")

    data().subselpars_.setEmpty();
    if ( is2D() )
    {
	BufferString linenm;
	if ( !lnmfld_ )
	    linenm = static_cast<uiSeis2DSubSel*>(subselfld_)->selectedLine();
	else
	    linenm = lnmfld_->getInput();
	if ( linenm.isEmpty() )
	    mErrRet( "Please enter a line name" )
	data().linekey_.setLineName( linenm );
	data().linekey_.setAttrName( seisfld_->attrNm() );
    }

    data().seiskey_ = ctio_.ioobj->key();
    data().fname_ = fnm;

    data().setScaler( scalefld_->getScaler() );
    data().remnull_ = remnullfld_->getBoolValue();
    const bool ismulticomp = multcompfld_->sensitive();
    data().compidx_ = ismulticomp ? multcompfld_->getIntValue() : 0;

    data().isasc_ = isascfld_->getBoolValue();
    data().havesd_ = havesdfld_->getBoolValue();
    if ( sdfld_ && !data().havesd_ )
    {
	data().sd_.start = sdfld_->getfValue(0);
	data().sd_.step = sdfld_->getfValue(1);
	if ( SI().zIsTime() )
	    { data().sd_.start *= 0.001; data().sd_.step *= 0.001; }
	data().nrsamples_ = sdfld_->getIntValue(2);
    }

    data().havepos_ = haveposfld_->getBoolValue();
    data().havenr_ = data().haverefnr_ = false;
    if ( data().havepos_ )
    {
	data().isxy_ = is2D() || isxyfld_->getBoolValue();
	data().havenr_ = havenrfld_ && havenrfld_->getBoolValue();
	data().haverefnr_ = data().havenr_ && haverefnrfld_->getBoolValue();
	if ( isimp_ && nrdeffld_ && !data().havenr_ )
	{
	    data().nrdef_.start = nrdeffld_->getIntValue(0);
	    data().nrdef_.step = nrdeffld_->getIntValue(1);
	}
	if ( isPS() )
	{
	    data().haveoffs_ = haveoffsbut_->isChecked();
	    data().haveazim_ = haveazimbut_->isChecked();
	}
    }
    else if ( isimp_ )
    {
	data().haveoffs_ = false;
	if ( is2D() )
	{
	    data().startpos_ = startposfld_->getCoord();
	    data().steppos_ = stepposfld_->getCoord();
	    data().nrdef_.start = startnrfld_->getIntValue();
	    data().nrdef_.step = stepnrfld_->getIntValue();
	}
	else
	{
	    data().inldef_.start = inldeffld_->getIntValue(0);
	    data().inldef_.step = inldeffld_->getIntValue(1);
	    if ( data().inldef_.step == 0 ) data().inldef_.step = 1;
	    data().crldef_.start = crldeffld_->getIntValue(0);
	    data().crldef_.step = crldeffld_->getIntValue(1);
	    if ( data().crldef_.step == 0 ) data().crldef_.step = 1;
	    int nrcpi = crldeffld_->getIntValue(2);
	    if ( nrcpi == 0 || crldeffld_->isUndef(2) )
	    {
		uiMSG().error( "Please define the number of Xlines per Inline");
		return false;
	    }
	    data().nrcrlperinl_ = nrcpi;
	}
    }

    if ( isPS() && !data().haveoffs_ && offsdeffld_ )
    {
	data().offsdef_.start = offsdeffld_->getfValue( 0 );
	data().offsdef_.step = offsdeffld_->getfValue( 2 );
	const float offsstop = offsdeffld_->getfValue( 1 );
	data().nroffsperpos_ =
			data().offsdef_.nearestIndex( offsstop ) + 1;
    }

    if ( subselfld_ )
    {
	subselfld_->fillPar( data().subselpars_ );
	if ( !subselfld_->isAll() )
	{
	    CubeSampling cs;
	    subselfld_->getSampling( cs.hrg ); subselfld_->getZRange( cs.zrg );
	    data().setResampler( new SeisResampler(cs,is2D()) );
	}
    }

    SeisIOSimple sios( data(), isimp_ );
    uiTaskRunner dlg( this );
    const bool res = TaskRunner::execute( &dlg, sios ) && !ismulticomp;
    if ( res )
	uiMSG().message( "Data successfully ",
			 isimp_ ? "imported." : "exported." );
    return false;
}
