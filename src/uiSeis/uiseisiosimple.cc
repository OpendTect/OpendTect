/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uiseisiosimple.h"

#include "ctxtioobj.h"
#include "file.h"
#include "filepath.h"
#include "ioman.h"
#include "ioobj.h"
#include "iopar.h"
#include "od_helpids.h"
#include "oddirs.h"
#include "seisioobjinfo.h"
#include "seisresampler.h"
#include "seisselection.h"
#include "survinfo.h"
#include "trckeyzsampling.h"
#include "unitofmeasure.h"

#include "ui2dgeomman.h"
#include "uibutton.h"
#include "uicoordsystem.h"
#include "uifileinput.h"
#include "uiioobjsel.h"
#include "uilabel.h"
#include "uimsg.h"
#include "uiscaler.h"
#include "uiseislinesel.h"
#include "uiseissel.h"
#include "uiseissubsel.h"
#include "uiseparator.h"
#include "uitaskrunner.h"


static bool survChanged()
{
    mDefineStaticLocalObject( BufferString, survnm, );
    const bool issame = survnm.isEmpty() || survnm == SI().name();
    survnm = SI().name();
    return !issame;
}


#define mDefData(nm,geom) \
SeisIOSimple::Data& uiSeisIOSimple::data##nm() \
{ \
    static SeisIOSimple::Data* d = nullptr; \
    if ( !d ) d = new SeisIOSimple::Data( GetDataDir(), Seis::geom ); \
    return *d; \
}

mDefData(2d,Line)
mDefData(3d,Vol)
mDefData(ps,VolPS)


uiSeisIOSimple::uiSeisIOSimple( uiParent* p, Seis::GeomType gt, bool imp )
    : uiDialog(p,Setup(uiStrings::sEmptyString(),
		       imp ? mODHelpKey(mSeisIOSimpleImpHelpID)
			   : mODHelpKey(mSeisIOSimpleExpHelpID)).modal(false))
    , geom_(gt)
    , isimp_(imp)
{
    const bool is2d = is2D();
    const bool isps = isPS();
    setCaption( tr("%1 %2 %3 ASCII/binary File")
	.arg(isimp_?"Import":"Export").arg( uiStrings::sSeismics(is2d,isps,1) )
	.arg(isimp_?"from":"to") );
    setOkCancelText( imp ? uiStrings::sImport() : uiStrings::sExport(),
		     uiStrings::sClose() );

    data().clear( survChanged() );

    coordsysselfld_ = new Coords::uiCoordSystemSel( this );
    coordsysselfld_->display( SI().hasProjection() );

    uiSeisSel::Setup ssu( geom_ );
    uiSeparator* sep = nullptr;
    if ( isimp_ )
    {
	mkIsAscFld();
	fnmfld_ = new uiASCIIFileInput( this,
		uiStrings::phrInput(uiStrings::sFile().toLower()), true );
	mAttachCB( fnmfld_->valueChanged, uiSeisIOSimple::inpFileSel );
	fnmfld_->attach( alignedBelow, isascfld_ );
    }
    else
    {
	const IOObjContext ctxt = uiSeisSel::ioContext( gt, !imp );
	ssu.enabotherdomain(true).steerpol(uiSeisSel::Setup::InclSteer).
	    withinserters(false);
	seisfld_ = new uiSeisSel( this, ctxt, ssu );
	mAttachCB( seisfld_->selectionDone, uiSeisIOSimple::inpSeisSel );
	sep = mkDataManipFlds();
    }

    if ( isimp_ && is2d )
	coordsysselfld_->attach( alignedBelow, fnmfld_);

    haveposfld_ = new uiGenInput( this,
			isimp_ ? tr("Traces start with a position")
			       : tr("Output a position for every trace"),
			BoolInpSpec(true) );
    haveposfld_->setValue( data().havepos_ );
    mAttachCB( haveposfld_->valueChanged, uiSeisIOSimple::haveposSel );
    if ( isimp_ && is2d )
	haveposfld_->attach( alignedBelow, coordsysselfld_ );
    else
	haveposfld_->attach( alignedBelow,
		    isimp_ ? fnmfld_->attachObj() : remnullfld_->attachObj() );

    if ( sep ) haveposfld_->attach( ensureBelow, sep );

    uiObject* attachobj = haveposfld_->attachObj();
    if ( is2d )
    {
	uiString txt= tr("%1 (preceding X/Y)").arg( isimp_ ?
		      tr("%1 included").arg( uiStrings::sTraceNumber() )
		      : tr("Include %1").arg( uiStrings::sTraceNumber() ) );
	havenrfld_ = new uiGenInput( this, txt, BoolInpSpec(true) );
	havenrfld_->setValue( data().havenr_ );
	havenrfld_->attach( alignedBelow, attachobj );
	mAttachCB( havenrfld_->valueChanged, uiSeisIOSimple::havenrSel );
	txt = tr("%1 (after trace number)").arg(isimp_ ?
	      tr("Ref/SP number included") : tr("Include Ref/SP number"));
	haverefnrfld_ = new uiGenInput( this, txt, BoolInpSpec(false) );
	haverefnrfld_->setValue( data().haverefnr_ );
	haverefnrfld_->attach( alignedBelow, havenrfld_ );
	mAttachCB( havenrfld_->valueChanged, uiSeisIOSimple::havenrSel );
	attachobj = haverefnrfld_->attachObj();
    }
    else
    {
	isxyfld_ = new uiGenInput( this, isimp_
		? tr("Position in file is") : tr("Position in file will be"),
		BoolInpSpec(true,tr("X/Y"),tr("Inl/Crl")) );
	isxyfld_->setValue( data().isxy_ );
	mAttachCB( isxyfld_->valueChanged,
		   uiSeisIOSimple::positionInFileSelChg );
	isxyfld_->attach( alignedBelow, attachobj );
	coordsysselfld_->attach( alignedBelow, isxyfld_ );
	coordsysselfld_->display( isxyfld_->getBoolValue() );

	attachobj = coordsysselfld_->attachObj();
    }

    if ( !isimp_ && isps )
    {
	haveoffsbut_ = new uiCheckBox( this, uiStrings::sOffset() );
	haveoffsbut_->setChecked( data().haveoffs_ );
	haveoffsbut_->attach( alignedBelow, attachobj );
	haveazimbut_ = new uiCheckBox( this, uiStrings::sAzimuth() );
	haveazimbut_->setChecked( data().haveazim_ );
	haveazimbut_->attach( rightOf, haveoffsbut_ );
	pspposlbl_ = new uiLabel( this, tr("Include"), haveoffsbut_ );
	attachobj = haveoffsbut_;
    }

    if ( isimp_ )
    {
	if ( !is2d )
	{
	    inldeffld_ = new uiGenInput( this,
					 tr("In-line definition: start, step"),
                                         IntInpSpec(data().inldef_.start_)
						.setName("Inl def start"),
                                         IntInpSpec(data().inldef_.step_)
						.setName("Inl def step") );
	    inldeffld_->attach( alignedBelow, haveposfld_ );
	    crldeffld_ = new uiGenInput( this,
			tr("Cross-line definition: start, step, # per inline"),
                                         IntInpSpec(data().crldef_.start_)
						.setName("Crl def start"),
                                         IntInpSpec(data().crldef_.step_)
						.setName("Crl def step"),
			   IntInpSpec(data().nrcrlperinl_)
						.setName("per inl") );
	    crldeffld_->attach( alignedBelow, inldeffld_ );
	    attachobj = crldeffld_->attachObj();
	}
	else
	{
	    nrdeffld_ = new uiGenInput( this,
		    tr("%1 definition: start, step")
			.arg( uiStrings::sTraceNumber() ),
					IntInpSpec(data().nrdef_.start_)
					   .setName("Trc def start"),
					IntInpSpec(data().nrdef_.step_)
					   .setName("Trc def step") );
	    nrdeffld_->attach( alignedBelow, havenrfld_ );
	    startposfld_ = new uiGenInput( this,
				tr("Start position (X, Y, Trace number)"),
				PositionInpSpec(data().startpos_) );
	    startposfld_->attach( alignedBelow, haveposfld_ );
	    stepposfld_ = new uiGenInput( this,
				tr("Increment (X, Y, Trace number)"),
				PositionInpSpec(data().steppos_) );
	    stepposfld_->attach( alignedBelow, startposfld_ );
	    startnrfld_ = new uiGenInput( this, uiString::emptyString(),
                                          IntInpSpec(data().nrdef_.start_) );
	    startnrfld_->setElemSzPol( uiObject::Small );
	    startnrfld_->attach( rightOf, startposfld_ );
	    stepnrfld_ = new uiGenInput( this, uiString::emptyString(),
                                         IntInpSpec(data().nrdef_.step_) );
	    stepnrfld_->setElemSzPol( uiObject::Small );
	    stepnrfld_->attach( rightOf, stepposfld_ );
	    attachobj = stepposfld_->attachObj();
	}

	if ( isps )
	{
	    haveoffsbut_ = new uiCheckBox( this, uiStrings::sOffset(),
					 mCB(this,uiSeisIOSimple,haveoffsSel) );
	    haveoffsbut_->attach( alignedBelow, attachobj );
	    haveoffsbut_->setChecked( data().haveoffs_ );
	    haveazimbut_ = new uiCheckBox( this, uiStrings::sAzimuth() );
	    haveazimbut_->attach( rightOf, haveoffsbut_ );
	    haveazimbut_->setChecked( data().haveazim_ );
	    pspposlbl_ = new uiLabel( this, tr("Position includes"),
				      haveoffsbut_ );
	    const float stopoffs =
			data().offsdef_.atIndex(data().nroffsperpos_-1);
	    offsdeffld_ = new uiGenInput( this,
					  tr("Offset definition: start, "
					     "stop, step"),
					  FloatInpSpec(data().offsdef_.start_)
					     .setName("Start"),
					  FloatInpSpec(stopoffs)
					     .setName("Stop"),
					  FloatInpSpec(data().offsdef_.step_)
					     .setName("Step"));
	    offsdeffld_->attach( alignedBelow, haveoffsbut_ );
	    attachobj = offsdeffld_->attachObj();
	}
    }

    havesdfld_ = new uiGenInput( this,
		isimp_ ? tr("File start contains sampling info")
		       : tr("Put sampling info in file start"),
		BoolInpSpec(true)
			.setName("Info in file start Yes",0)
			.setName("Info in file start No",1) );
    havesdfld_->setValue( data().havesd_ );
    havesdfld_->attach( alignedBelow, attachobj );
    mAttachCB( havesdfld_->valueChanged, uiSeisIOSimple::havesdSel );

    if ( isimp_ )
    {
	uiString txt = tr("Sampling info: start, step %1 and #samples").
		       arg(SI().getUiZUnitString(true));
	SamplingData<float> sd( data().sd_ );
	if ( SI().zIsTime() )
        { sd.start_ *= 1000; sd.step_ *= 1000; }
	sdfld_ = new uiGenInput( this, txt,
				 DoubleInpSpec(sd.start_)
				    .setName("SampInfo start"),
				 DoubleInpSpec(sd.step_)
				    .setName("SampInfo step"),
				 IntInpSpec(data().nrsamples_)
				    .setName("Nr samples"));
	sdfld_->attach( alignedBelow, havesdfld_ );
	sep = mkDataManipFlds();
	if ( !isps )
	    ssu.enabotherdomain( true );

	const IOObjContext ctxt = uiSeisSel::ioContext( gt, !imp );
	seisfld_ = new uiSeisSel( this, ctxt, ssu );
	mAttachCB( seisfld_->domainChanged, uiSeisIOSimple::zUnitChangedCB );
	mAttachCB( seisfld_->zUnitChanged, uiSeisIOSimple::zUnitChangedCB );
	seisfld_->attach( alignedBelow, remnullfld_ );
	if ( is2d )
	{
	    lnmfld_ = new uiSeis2DLineNameSel( this, false );
	    lnmfld_->attach( alignedBelow, seisfld_ );
	}
    }
    else
    {
	mkIsAscFld();
	isascfld_->attach( alignedBelow, havesdfld_ );
	fnmfld_ = new uiASCIIFileInput( this,
		uiStrings::phrOutput(uiStrings::sFile().toLower()), false );
	fnmfld_->attach( alignedBelow, isascfld_ );
    }

    mAttachCB( postFinalize(), uiSeisIOSimple::initFlds );
}


uiSeisIOSimple::~uiSeisIOSimple()
{
    detachAllNotifiers();
}


void uiSeisIOSimple::zUnitChangedCB( CallBacker* )
{
    if ( !isimp_ )
	return;

    const ZDomain::Info& zinfo = seisfld_->getZDomain();
    const uiString lbl = tr("Sampling info: start, step %1 and #samples").
	arg(zinfo.unitStr(true));
    sdfld_->setTitleText( lbl );
}


void uiSeisIOSimple::mkIsAscFld()
{
    isascfld_ = new uiGenInput( this, tr("File type"),
				BoolInpSpec(true,uiStrings::sASCII(),
					    tr("Binary")) );
    mAttachCB( isascfld_->valueChanged, uiSeisIOSimple::isascSel );
    isascfld_->setValue( data().isasc_ );
}


uiSeparator* uiSeisIOSimple::mkDataManipFlds()
{
    uiSeparator* sep = new uiSeparator( this, "sep inp and outp pars" );
    if ( isimp_ )
	sep->attach( stretchedBelow, sdfld_ );
    else
    {
	Seis::SelSetup su( geom_ );
	su.fornewentry( false ).onlyrange( false );
	subselfld_ = new uiMultiZSeisSubSel( this, su );
	subselfld_->attachObj()->attach( alignedBelow, seisfld_ );
    }

    scalefld_ = new uiScaler( this, uiStrings::sEmptyString(), true );
    scalefld_->attach( alignedBelow, isimp_ ? sdfld_->attachObj()
					   : subselfld_->attachObj() );
    if ( isimp_ ) scalefld_->attach( ensureBelow, sep );
    remnullfld_ = new uiGenInput( this, tr("Null traces"),
		BoolInpSpec(true,uiStrings::sDiscard(),uiStrings::sPass()) );
    remnullfld_->attach( alignedBelow, scalefld_ );

    multcompfld_ = new uiGenInput( this, tr("Component to export"),
				   StringListInpSpec() );
    multcompfld_->display( false );
    multcompfld_->setSensitive( false );

    if ( !isimp_ )
    {
	if ( is2D() )
	{
	    coordsysselfld_->attach( alignedBelow, remnullfld_ );
	    multcompfld_->attach( alignedBelow, coordsysselfld_ );
	}
	else
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

    if ( startposfld_ )
    {
	startposfld_->setNrDecimals( 2, 0 );
	startposfld_->setNrDecimals( 2, 1 );
    }

    if ( stepposfld_ )
    {
	stepposfld_->setNrDecimals( 2, 0 );
	stepposfld_->setNrDecimals( 2, 1 );
    }
}


void uiSeisIOSimple::havesdSel( CallBacker* )
{
    if ( sdfld_ )
	sdfld_->display( !havesdfld_->getBoolValue() );
}


void uiSeisIOSimple::inpSeisSel( CallBacker* )
{
    const IOObj* ioobj = seisfld_->ioobj( true );
    if ( !ioobj )
	return;

    subselfld_->setInput( *ioobj );
    BufferStringSet compnms;
    SeisIOObjInfo::getCompNames( ioobj->key(), compnms );
    multcompfld_->newSpec( StringListInpSpec(compnms), 0 );
    multcompfld_->display( compnms.size()>1 );
    multcompfld_->setSensitive( compnms.size()>1 );

    const FilePath fp = ioobj->fullUserExpr();
    FilePath fnm( GetSurveyExportDir(), fp.baseName() );
    fnm.setExtension( "dat" );
    fnmfld_->setFileName( fnm.fullPath() );
}


void uiSeisIOSimple::inpFileSel( CallBacker* )
{
    FilePath fnmfp( fnmfld_->fileName() );
    seisfld_->setInputText( fnmfp.baseName() );
}


void uiSeisIOSimple::lsSel( CallBacker* )
{
    if ( !lnmfld_ )
	return;

    const IOObj* ioobj = seisfld_->ioobj( true );
    if ( ioobj )
	lnmfld_->setDataSet( ioobj->key() );
}



void uiSeisIOSimple::isascSel( CallBacker* )
{
    fnmfld_->enableExamine( isascfld_->getBoolValue() );
}


void uiSeisIOSimple::haveposSel( CallBacker* cb )
{
    const bool havenopos = !haveposfld_->getBoolValue();
    if ( havenrfld_ )
	havenrfld_->display( !havenopos );

    if ( isxyfld_ )
    {
	isxyfld_->display( !havenopos );
	coordsysselfld_->display(
		isxyfld_->getBoolValue() && isxyfld_->isDisplayed() );
    }

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


void uiSeisIOSimple::havenrSel( CallBacker* )
{
    if ( !havenrfld_ )
	return;

    const bool havepos = haveposfld_->getBoolValue();
    const bool havenr = havenrfld_->getBoolValue();
    if ( nrdeffld_ )
	nrdeffld_->display( havepos && !havenr );

    if ( haverefnrfld_ )
	haverefnrfld_->display( havepos && havenr );
}


void uiSeisIOSimple::haveoffsSel( CallBacker* )
{
    if ( !pspposlbl_ || !haveoffsbut_ )
	return;

    const bool havepos = haveposfld_->getBoolValue();
    const bool haveoffs = haveoffsbut_->isChecked();
    haveoffsbut_->display( havepos );
    haveazimbut_->display( havepos );
    pspposlbl_->display( havepos );
    if ( offsdeffld_ )
	offsdeffld_->display( !havepos || !haveoffs );
}


void uiSeisIOSimple::positionInFileSelChg( CallBacker* )
{
      coordsysselfld_->display( isxyfld_->getBoolValue() );
}


#define mErrRet(s) { uiMSG().error(s); return false; }

bool uiSeisIOSimple::acceptOK( CallBacker* )
{
    if ( !isascfld_ )
	return true;

    const BufferString fnm( fnmfld_->fileName() );
    if ( isimp_ && !File::exists(fnm) )
	mErrRet(tr("Input file does not exist or is unreadable"))

    const IOObj* ioobj = seisfld_->ioobj( true );
    if ( !ioobj )
	return false;

    data().subselpars_.setEmpty();
    auto* seissubsel = subselfld_ ? subselfld_->getSelGrp() : nullptr;
    if ( is2D() )
    {
	BufferString linenm;
	if ( lnmfld_ )
	    linenm = lnmfld_->getInput();
	else
	    linenm = static_cast<uiSeis2DSubSel*>(seissubsel)->selectedLine();

	if ( linenm.isEmpty() )
	    mErrRet( tr("Please enter a line name") )

	if ( isimp_ )
	{
	    data().geomid_ = Geom2DImpHandler::getGeomID( linenm );
	    if ( !Survey::is2DGeom(data().geomid_) )
		return false;
	}
	else
	    data().geomid_ = Survey::GM().getGeomID( linenm );

	data().linename_ = linenm;
    }
    else
	data().geomid_ = Survey::default3DGeomID();

    data().seiskey_ = ioobj->key();
    data().fname_ = fnm;

    data().setScaler( scalefld_->getScaler() );
    data().remnull_ = remnullfld_->getBoolValue();
    const bool ismulticomp = multcompfld_->sensitive();
    data().compidx_ = ismulticomp ? multcompfld_->getIntValue() : 0;

    data().isasc_ = isascfld_->getBoolValue();
    data().havesd_ = havesdfld_->getBoolValue();
    if ( sdfld_ && !data().havesd_ )
    {
        data().sd_.start_ = sdfld_->getFValue(0);
        data().sd_.step_ = sdfld_->getFValue(1);
	const LinScaler& scaler =
	    UnitOfMeasure::zUnit( seisfld_->getZDomain(), false )->scaler();
        data().sd_.start_ = scaler.scale( data().sd_.start_ );
        data().sd_.step_ = scaler.scale( data().sd_.step_ );
	data().nrsamples_ = sdfld_->getIntValue(2);
    }

    data().havepos_ = haveposfld_->getBoolValue();
    data().havenr_ = data().haverefnr_ = false;
    if ( coordsysselfld_ && coordsysselfld_->isDisplayed() )
	data().setCoordSys( coordsysselfld_->getCoordSystem().ptr() );

    if ( data().havepos_ )
    {
	data().isxy_ = is2D() || isxyfld_->getBoolValue();
	data().havenr_ = havenrfld_ && havenrfld_->getBoolValue();
	data().haverefnr_ = data().havenr_ && haverefnrfld_->getBoolValue();
	if ( isimp_ && nrdeffld_ && !data().havenr_ )
	{
            data().nrdef_.start_ = nrdeffld_->getIntValue(0);
            data().nrdef_.step_ = nrdeffld_->getIntValue(1);
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
            data().nrdef_.start_ = startnrfld_->getIntValue();
            data().nrdef_.step_ = stepnrfld_->getIntValue();
	}
	else
	{
            data().inldef_.start_ = inldeffld_->getIntValue(0);
            data().inldef_.step_ = inldeffld_->getIntValue(1);
            if ( data().inldef_.step_ == 0 ) data().inldef_.step_ = 1;
            data().crldef_.start_ = crldeffld_->getIntValue(0);
            data().crldef_.step_ = crldeffld_->getIntValue(1);
            if ( data().crldef_.step_ == 0 ) data().crldef_.step_ = 1;
	    int nrcpi = crldeffld_->getIntValue(2);
	    if ( nrcpi == 0 || crldeffld_->isUndef(2) )
	    {
		uiMSG().error( tr("Please define the number"
				  " of Xlines per Inline"));
		return false;
	    }
	    data().nrcrlperinl_ = nrcpi;
	}
    }

    if ( isPS() && !data().haveoffs_ && offsdeffld_ )
    {
        data().offsdef_.start_ = offsdeffld_->getFValue( 0 );
        data().offsdef_.step_ = offsdeffld_->getFValue( 2 );
	const float offsstop = offsdeffld_->getFValue( 1 );
	data().nroffsperpos_ = data().offsdef_.nearestIndex( offsstop ) + 1;
    }

    if ( subselfld_ )
    {
	subselfld_->getSelGrp()->fillPar( data().subselpars_ );
	if ( !seissubsel->isAll())
	{
	    TrcKeyZSampling cs;
	    seissubsel->getSampling( cs.hsamp_ );
	    seissubsel->getZRange( cs.zsamp_ );
	    data().setResampler( new SeisResampler(cs,is2D()) );
	}
    }

    seisfld_->getZDomain().fillPar( ioobj->pars() );
    if ( isimp_ && !IOM().commitChanges(*ioobj) )
    {

	uiMSG().error(uiStrings::phrCannotWriteDBEntry(ioobj->uiName()));
	return false;
    }

    SeisIOSimple sios( data(), isimp_ );
    uiTaskRunner dlg( this );
    const bool res = TaskRunner::execute( &dlg, sios ) && !ismulticomp;
    if ( res )
	uiMSG().message(tr("Data successfully %1")
		      .arg(isimp_ ? tr("imported.") : tr("exported.")));
    return false;
}
