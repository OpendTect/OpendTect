/*+
 * COPYRIGHT: (C) dGB Beheer B.V.
 * AUTHOR   : A.H. Bril
 * DATE     : Oct 2003
-*/

static const char* rcsID = "$Id: uiseisiosimple.cc,v 1.5 2007-12-05 11:55:49 cvsbert Exp $";

#include "uiseisiosimple.h"
#include "uiseisfmtscale.h"
#include "uiseissubsel.h"
#include "uiseissel.h"
#include "uifileinput.h"
#include "uiioobjsel.h"
#include "uiexecutor.h"
#include "uiseparator.h"
#include "uiscaler.h"
#include "uilabel.h"
#include "uimsg.h"
#include "seistrctr.h"
#include "seisselection.h"
#include "seisresampler.h"
#include "ctxtioobj.h"
#include "cubesampling.h"
#include "ioobj.h"
#include "iopar.h"
#include "survinfo.h"
#include "oddirs.h"
#include "filegen.h"
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
    	, ctio(*mMkCtxtIOObj(SeisTrc))
    	, sdfld(0)
	, havenrfld(0)
	, nrdeffld(0)
    	, inldeffld(0)
    	, subselfld(0)
    	, isxyfld(0)
    	, lnmfld(0)
    	, isascfld(0)
    	, isimp_(imp)
    	, geom_(gt)
{
    if ( isPS() )
    {
	new uiLabel( this, "Pre-Stack not supported (yet)" );
	return;
    }

    data().clear( survChanged() );
    ctio.ctxt.forread = !isimp_;
    ctio.ctxt.trglobexpr = "CBVS";
    const bool is2d = is2D();

    uiSeparator* sep = 0;
    if ( isimp_ )
    {
	mkIsAscFld();
	fnmfld = new uiFileInput( this, "Input file", uiFileInput::Setup("")
			.forread( true )
			.withexamine( true ) );
	fnmfld->attach( alignedBelow, isascfld );
    }
    else
    {
	seisfld = new uiSeisSel( this, ctio, uiSeisSel::Setup(geom_) );
	seisfld->selectiondone.notify( mCB(this,uiSeisIOSimple,inpSeisSel) );
	sep = mkDataManipFlds();
    }

    haveposfld = new uiGenInput( this,
	    		isimp_ ? "Traces start with a position"
			      : "Output a position for every trace",
	   		BoolInpSpec(true) );
    haveposfld->setValue( data().havepos_ );
    haveposfld->valuechanged.notify( mCB(this,uiSeisIOSimple,haveposSel) );
    haveposfld->attach( alignedBelow, isimp_ ? fnmfld->attachObj()
	    				     : remnullfld->attachObj() );
    if ( sep ) haveposfld->attach( ensureBelow, sep );

    uiObject* attachobj = haveposfld->attachObj();
    if ( is2d )
    {
	BufferString txt( isimp_ ? "Trace number included"
				 : "Include trace number");
	txt += " (preceeding X/Y)";
	havenrfld = new uiGenInput( this, txt, BoolInpSpec(true) );
	havenrfld->setValue( data().havenr_ );
	havenrfld->attach( alignedBelow, attachobj );
	havenrfld->valuechanged.notify( mCB(this,uiSeisIOSimple,havenrSel) );
	attachobj = havenrfld->attachObj();
    }
    else
    {
	isxyfld = new uiGenInput( this, isimp_ ? "Position in file is"
					       : "Position in file will be",
				  BoolInpSpec(true,"X Y","Inline Xline") );
	isxyfld->setValue( data().isxy_ );
	isxyfld->attach( alignedBelow, attachobj );
	if ( !isimp_ ) attachobj = isxyfld->attachObj();
    }

    if ( isimp_ )
    {
	if ( !is2d )
	{
	    inldeffld = new uiGenInput( this, "Inline definition: start, step",
					      IntInpSpec(data().inldef_.start),
					      IntInpSpec(data().inldef_.step) );
	    inldeffld->attach( alignedBelow, attachobj );
	    crldeffld = new uiGenInput( this,
				"Xline definition: start, step, # per inline",
				IntInpSpec(data().crldef_.start),
				IntInpSpec(data().crldef_.step),
				IntInpSpec(data().nrcrlperinl_) );
	    crldeffld->attach( alignedBelow, inldeffld );
	    attachobj = crldeffld->attachObj();
	}
	else
	{
	    nrdeffld = new uiGenInput( this,
		    		"Trace number definition: start, step",
				 IntInpSpec(data().nrdef_.start),
				 IntInpSpec(data().nrdef_.step) );
	    nrdeffld->attach( alignedBelow, attachobj );
	    startposfld = new uiGenInput( this,
					  "Start position (X, Y, Trace number)",
					  PositionInpSpec(data().startpos_) );
	    startposfld->attach( alignedBelow, haveposfld );
	    stepposfld = new uiGenInput( this, "Step in X/Y/Number",
					 PositionInpSpec(data().steppos_) );
	    stepposfld->attach( alignedBelow, startposfld );
	    startnrfld = new uiGenInput( this, "",
		    			 IntInpSpec(data().nrdef_.start) );
	    startnrfld->setElemSzPol( uiObject::Small );
	    startnrfld->attach( rightOf, startposfld );
	    stepnrfld = new uiGenInput( this, "",
		    			IntInpSpec(data().nrdef_.step) );
	    stepnrfld->setElemSzPol( uiObject::Small );
	    stepnrfld->attach( rightOf, stepposfld );
	    attachobj = stepposfld->attachObj();
	}
    }

    havesdfld = new uiGenInput( this, isimp_
	    			    ? "File start contains sampling info"
				    : "Put sampling info in file start",
	    			    BoolInpSpec(true) );
    havesdfld->setValue( data().havesd_ );
    havesdfld->attach( alignedBelow, attachobj );
    havesdfld->valuechanged.notify( mCB(this,uiSeisIOSimple,havesdSel) );

    if ( isimp_ )
    {
	BufferString txt = "Sampling info: start, step ";
	txt += SI().getZUnit(true);
	txt += " and #samples";
	SamplingData<float> sd( data().sd_ );
	if ( SI().zIsTime() )
	    { sd.start *= 1000; sd.step *= 1000; }
	sdfld = new uiGenInput( this, txt, DoubleInpSpec(sd.start),
		    DoubleInpSpec(sd.step), IntInpSpec(data().nrsamples_) );
	sdfld->attach( alignedBelow, havesdfld );
	sep = mkDataManipFlds();
	seisfld = new uiSeisSel( this, ctio, uiSeisSel::Setup(geom_));
	seisfld->attach( alignedBelow, remnullfld );
	if ( is2d )
	{
	    lnmfld = new uiGenInput( this, "Line name in Set" );
	    lnmfld->attach( alignedBelow, seisfld );
	}
    }
    else
    {
	mkIsAscFld();
	isascfld->attach( alignedBelow, havesdfld );
	fnmfld = new uiFileInput( this, "Output file", uiFileInput::Setup("")
			.forread( false )
			.withexamine( false ) );
	fnmfld->attach( alignedBelow, isascfld );
    }

    fnmfld->setDefaultSelectionDir( FilePath(data().fname_).pathOnly() );
    finaliseDone.notify( mCB(this,uiSeisIOSimple,initFlds) );
}


void uiSeisIOSimple::mkIsAscFld()
{
    isascfld = new uiGenInput( this, "File type",
	    		       BoolInpSpec(true,"Ascii","Binary") );
    isascfld->valuechanged.notify( mCB(this,uiSeisIOSimple,isascSel) );
    isascfld->setValue( data().isasc_ );
}


uiSeparator* uiSeisIOSimple::mkDataManipFlds()
{
    uiSeparator* sep = new uiSeparator( this, "sep inp and outp pars" );
    if ( isimp_ )
	sep->attach( stretchedBelow, sdfld );
    else
    {
	subselfld = uiSeisSubSel::get( this, Seis::SelSetup(geom_) );
	subselfld->attachObj()->attach( alignedBelow, seisfld );
    }

    scalefld = new uiScaler( this, 0, true );
    scalefld->attach( alignedBelow, isimp_ ? sdfld->attachObj()
	    				   : subselfld->attachObj() );
    if ( isimp_ ) scalefld->attach( ensureBelow, sep );
    remnullfld = new uiGenInput( this, "Null traces",
				 BoolInpSpec(true,"Discard","Pass") );
    remnullfld->attach( alignedBelow, scalefld );

    if ( !isimp_ )
	sep->attach( stretchedBelow, remnullfld );

    return sep;
}


void uiSeisIOSimple::initFlds( CallBacker* cb )
{
    havesdSel( cb );
    haveposSel( cb );
    isascSel( cb );
}


void uiSeisIOSimple::havesdSel( CallBacker* )
{
    if ( sdfld )
	sdfld->display( !havesdfld->getBoolValue() );
}


void uiSeisIOSimple::inpSeisSel( CallBacker* )
{
    seisfld->commitInput( !isimp_ );
    if ( ctio.ioobj )
	subselfld->setInput( *ctio.ioobj );
}



void uiSeisIOSimple::isascSel( CallBacker* )
{
    fnmfld->enableExamine( isascfld->getBoolValue() );
}


void uiSeisIOSimple::haveposSel( CallBacker* cb )
{
    const bool havenopos = !haveposfld->getBoolValue();

    if ( isxyfld ) isxyfld->display( !havenopos );
    if ( havenrfld ) havenrfld->display( !havenopos );

    if ( isimp_ )
    {
	if ( !is2D() )
	{
	    inldeffld->display( havenopos );
	    crldeffld->display( havenopos );
	}
	else
	{
	    startposfld->display( havenopos );
	    startnrfld->display( havenopos );
	    stepposfld->display( havenopos );
	    stepnrfld->display( havenopos );
	}
    }

    havenrSel( cb );
}


void uiSeisIOSimple::havenrSel( CallBacker* cb )
{
    if ( !nrdeffld ) return;
    nrdeffld->display( haveposfld->getBoolValue()
	    	    && !havenrfld->getBoolValue() );
}


#define mErrRet(s) { uiMSG().error(s); return false; }

bool uiSeisIOSimple::acceptOK( CallBacker* )
{
    if ( !isascfld ) return true;

    BufferString fnm( fnmfld->fileName() );
    if ( isimp_ && !File_exists(fnm) )
	mErrRet("Input file does not exist or is unreadable")
    if ( !seisfld->commitInput(isimp_) )
	mErrRet( isimp_ ? "Please choose a name for the imported data"
		       : "Please select the input seismics")

    data().subselpars_.clear();
    if ( is2D() )
    {
	BufferString attrnm = seisfld->attrNm();
	BufferString linenm;
	if ( isimp_ )
	{
	    linenm = lnmfld->text();
	    if ( linenm.isEmpty() )
		mErrRet( "Please enter a line name" )
	}
	LineKey lk( linenm, attrnm );
	data().subselpars_.set( sKey::LineKey, lk );
	data().subselpars_.set( sKey::Attribute, attrnm );
		// Needed because attrnm can disappear from line key
    }

    data().seiskey_ = ctio.ioobj->key();
    data().fname_ = fnm;

    data().setScaler( scalefld->getScaler() );
    data().remnull_ = remnullfld->getBoolValue();

    data().isasc_ = isascfld->getBoolValue();
    data().havesd_ = havesdfld->getBoolValue();
    if ( sdfld && !data().havesd_ )
    {
	data().sd_.start = sdfld->getfValue(0);
	data().sd_.step = sdfld->getfValue(1);
	if ( SI().zIsTime() )
	    { data().sd_.start *= 0.001; data().sd_.step *= 0.001; }
	data().nrsamples_ = sdfld->getIntValue(2);
    }

    data().havepos_ = haveposfld->getBoolValue();
    data().havenr_ = false;
    if ( data().havepos_ )
    {
	data().isxy_ = is2D() || isxyfld->getBoolValue();
	data().havenr_ = is2D() && havenrfld->getBoolValue();
	if ( isimp_ && data().havenr_ )
	{
	    data().nrdef_.start = nrdeffld->getIntValue(0);
	    data().nrdef_.step = nrdeffld->getIntValue(1);
	}
    }
    else if ( isimp_ )
    {
	if ( is2D() )
	{
	    data().startpos_ = startposfld->getCoord();
	    data().steppos_ = stepposfld->getCoord();
	    data().nrdef_.start = startnrfld->getIntValue();
	    data().nrdef_.step = stepnrfld->getIntValue();
	}
	else
	{
	    data().inldef_.start = inldeffld->getIntValue(0);
	    data().inldef_.step = inldeffld->getIntValue(1);
	    if ( data().inldef_.step == 0 ) data().inldef_.step = 1;
	    data().crldef_.start = crldeffld->getIntValue(0);
	    data().crldef_.step = crldeffld->getIntValue(1);
	    if ( data().crldef_.step == 0 ) data().crldef_.step = 1;
	    int nrcpi = crldeffld->getIntValue(2);
	    if ( nrcpi == 0 || crldeffld->isUndef(2) )
	    {
		uiMSG().error( "Please define the number of Xlines per Inline");
		return false;
	    }
	    data().nrcrlperinl_ = nrcpi;
	}
    }

    if ( subselfld )
    {
	subselfld->fillPar( data().subselpars_ );
	if ( !subselfld->isAll() )
	{
	    CubeSampling cs;
	    subselfld->getSampling( cs.hrg ); subselfld->getZRange( cs.zrg );
	    data().setResampler( new SeisResampler(cs,is2D()) );
	}
    }

    SeisIOSimple sios( data(), isimp_ );
    uiExecutor dlg( this, sios );
    return dlg.go();
}
