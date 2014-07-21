/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert Bril
 Date:          Jun 2002
________________________________________________________________________

-*/
static const char* rcsID mUsedVar = "$Id$";

#include "uiseiscbvsimp.h"

#include "ctxtioobj.h"
#include "executor.h"
#include "file.h"
#include "filepath.h"
#include "ioman.h"
#include "iopar.h"
#include "iostrm.h"
#include "keystrs.h"
#include "oddirs.h"
#include "ptrman.h"
#include "scaler.h"
#include "seiscbvs.h"
#include "seisread.h"
#include "seisselection.h"
#include "seisselectionimpl.h"
#include "seissingtrcproc.h"
#include "seistrc.h"
#include "seistrcprop.h"
#include "seiswrite.h"
#include "survinfo.h"
#include "veldesc.h"
#include "velocitycalc.h"
#include "zdomain.h"

#include "uibutton.h"
#include "uicombobox.h"
#include "uifileinput.h"
#include "uiioobjsel.h"
#include "uimsg.h"
#include "uiscaler.h"
#include "uiseisfmtscale.h"
#include "uiseisioobjinfo.h"
#include "uiseislinesel.h"
#include "uiseissel.h"
#include "uiseistransf.h"
#include "uitaskrunner.h"
#include "od_helpids.h"

uiSeisImpCBVS::uiSeisImpCBVS( uiParent* p )
	: uiDialog(p,Setup(tr("Import CBVS cube"),
			   tr("Specify import parameters"),
			   mODHelpKey(mSeisImpCBVSHelpID) ))
	, inctio_(*mMkCtxtIOObj(SeisTrc))
	, outctio_(*uiSeisSel::mkCtxtIOObj(Seis::Vol,false))
	, modefld(0)
	, sstp_(0)
{
    setCtrlStyle( RunAndClose );

    init( false );
    modeSel(0);
}


uiSeisImpCBVS::uiSeisImpCBVS( uiParent* p, const IOObj* ioobj )
	: uiDialog(p,Setup(tr("Copy cube data"),
			   tr("Specify copy parameters"),
			   mODHelpKey(mSeisImpCBVSCopyHelpID) ))
	, inctio_(*uiSeisSel::mkCtxtIOObj(Seis::Vol,true))
	, outctio_(*uiSeisSel::mkCtxtIOObj(Seis::Vol,false))
	, modefld(0)
	, sstp_(0)
{
    setCtrlStyle( RunAndClose );

    if ( ioobj ) inctio_.ioobj = ioobj->clone();
    init( true );
    oinpSel(0);
}


void uiSeisImpCBVS::init( bool fromioobj )
{
    finpfld = 0; modefld = typefld = 0; oinpfld = 0; convertfld = 0;
    compfld_ = 0;
    ismc_ = false;
    setTitleText( fromioobj ? tr("Specify transfer parameters")
			    : tr("Create CBVS cube definition") );
    tmpid_ = "100010."; tmpid_ += IOObj::tmpID();

    uiSeisTransfer::Setup sts( Seis::Vol );
    uiGroup* attobj = 0;
    if ( fromioobj )
    {
	inctio_.ctxt.forread = true;
	inctio_.ctxt.fixTranslator( CBVSSeisTrcTranslator::translKey() );
	uiSeisSel::Setup sssu( Seis::Vol );
	sssu.steerpol( uiSeisSel::Setup::InclSteer );
	oinpfld = new uiSeisSel( this, inctio_, sssu );
	oinpfld->selectionDone.notify( mCB(this,uiSeisImpCBVS,oinpSel) );
	compfld_ = new uiLabeledComboBox( this, tr("Component(s)") );
	attobj = compfld_;
	compfld_->attach( alignedBelow, oinpfld );
	if ( inctio_.ioobj )
	{
	    SeisIOObjInfo oinf( *inctio_.ioobj );
	    sts.zdomkey_ = oinf.zDomainDef().key();
	    if ( sts.zdomkey_ != ZDomain::SI().key() )
		oinpfld->setSensitive( false );
	}
    }
    else
    {
	uiFileInput::Setup fisu( uiFileDialog::Gen );
	fisu.filter("CBVS (*.cbvs)").defseldir( GetBaseDataDir() );
	finpfld = new uiFileInput( this, "(First) CBVS file name", fisu );
	finpfld->valuechanged.notify( mCB(this,uiSeisImpCBVS,finpSel) );

	StringListInpSpec spec;
	spec.addString( "Input data cube" );
	spec.addString( "Generated attribute cube" );
	spec.addString( "Steering cube" );
	typefld = new uiGenInput( this, tr("Cube type"), spec );
	typefld->attach( alignedBelow, finpfld );
	typefld->valuechanged.notify( mCB(this,uiSeisImpCBVS,typeChg) );

	modefld = new uiGenInput( this, tr("Import mode"),
			  BoolInpSpec(false,tr("Copy the data"),
                                      tr("Use in-place")) );
	modefld->attach( alignedBelow, typefld );
	modefld->valuechanged.notify( mCB(this,uiSeisImpCBVS,modeSel) );
	attobj = modefld;

	convertfld = new uiCheckBox( this,
		tr("Convert underscores to spaces in Output Cube name"),
		mCB(this,uiSeisImpCBVS,convertSel) );
    }

    sts.withnullfill(fromioobj).withstep(true).onlyrange(false)
				.fornewentry(true);
    transffld = new uiSeisTransfer( this, sts );
    transffld->attach( alignedBelow, attobj );

    uiSeisSel::Setup sssu( Seis::Vol );
    sssu.steerpol( uiSeisSel::Setup::InclSteer );
    outctio_.ctxt.forread = false;
    outctio_.ctxt.fixTranslator( CBVSSeisTrcTranslator::translKey() );
    IOM().to( outctio_.ctxt.getSelKey() );
    if ( !fromioobj )
	sssu.enabotherdomain( true );
    outfld = new uiSeisSel( this, outctio_, sssu );

    if ( convertfld )
    {
	convertfld->attach( ensureBelow, transffld );
	convertfld->attach( leftAlignedAbove, outfld );
    }

    outfld->attach( alignedBelow, transffld );
}


uiSeisImpCBVS::~uiSeisImpCBVS()
{
    delete outctio_.ioobj; delete &outctio_;
    delete inctio_.ioobj; delete &inctio_;
}


IOObj* uiSeisImpCBVS::getfInpIOObj( const char* inp ) const
{
    IOStream* iostrm = new IOStream( "_tmp", tmpid_ );
    iostrm->setGroup( outctio_.ctxt.trgroup->userName() );
    iostrm->setTranslator( CBVSSeisTrcTranslator::translKey() );
    iostrm->setDirName( "Seismics" );
    iostrm->setFileName( inp );
    return iostrm;
}


void uiSeisImpCBVS::modeSel( CallBacker* )
{
    if ( modefld )
	transffld->display( modefld->getBoolValue() );
}


void uiSeisImpCBVS::typeChg( CallBacker* )
{
    bool issteer = typefld ? typefld->getIntValue() == 2 : false;
    if ( oinpfld )
    {
	oinpfld->commitInput();
	if ( !inctio_.ioobj ) return;
	const char* res = inctio_.ioobj->pars().find( "Type" );
	issteer = res && *res == 'S';
    }

    transffld->setSteering( issteer );
}


void uiSeisImpCBVS::oinpSel( CallBacker* cb )
{
    if ( !oinpfld ) return;
    oinpfld->commitInput();
    ismc_ = false;
    if ( inctio_.ioobj )
    {
	transffld->updateFrom( *inctio_.ioobj );
	SeisIOObjInfo oinf( *inctio_.ioobj );
	ismc_ = oinf.isOK() && oinf.nrComponents() > 1;
	compfld_->display( ismc_ );
	if ( ismc_ )
	{
	    BufferStringSet cnms; oinf.getComponentNames( cnms );
	    compfld_->box()->setEmpty();
	    compfld_->box()->addItem( "<All>" );
	    compfld_->box()->addItems( cnms );
	}
    }
    typeChg( cb );
}


void uiSeisImpCBVS::finpSel( CallBacker* )
{
    BufferString inp = finpfld->text();
    if ( inp.isEmpty() ) return;

    if ( !File::isEmpty(inp) )
    {
	PtrMan<IOObj> ioobj = getfInpIOObj( inp );
	transffld->updateFrom( *ioobj );
    }

    getOutputName( inp );
    outfld->setInputText( inp );
}


void uiSeisImpCBVS::convertSel( CallBacker* )
{
    BufferString inp = finpfld->text();
    getOutputName( inp );
    outfld->setInputText( inp );
}


void uiSeisImpCBVS::getOutputName( BufferString& inp ) const
{
    inp = FilePath( inp ).fileName();
    if ( inp.isEmpty() ) return;

    char* ptr = inp.getCStr();
    if ( convertfld->isChecked() )
    {
	// convert underscores to spaces
	while ( *ptr )
	{
	    if ( *ptr == '_' ) *ptr = ' ';
	    ptr++;
	}
    }

    // remove .cbvs extension
    ptr = inp.find( '.' );
    if ( ptr && *(ptr+1) == 'c' && *(ptr+2) == 'b' && *(ptr+3) == 'v'
	 && *(ptr+4) == 's' )
	*(ptr) = '\0';

}


#define rmTmpIOObj() IOM().permRemove( MultiID(tmpid_.buf()) );

bool uiSeisImpCBVS::acceptOK( CallBacker* )
{
    if ( !outfld->commitInput() )
    {
	if ( outfld->isEmpty() )
	    uiMSG().error( tr("Please choose a name for the output data") );
	return false;
    }

    const bool dolink = modefld && !modefld->getBoolValue();
    if ( oinpfld )
    {
	if ( !oinpfld->commitInput() )
	{
	    uiMSG().error( tr("Please select an input cube") );
	    return false;
	}
	outctio_.ioobj->pars() = inctio_.ioobj->pars();
    }
    else
    {
	BufferString fname = finpfld->text();
	if ( !fname.str() )
	{
	    uiMSG().error( tr("Please select the input filename") );
	    return false;
	}

	if ( dolink )
	{
	    //Check if it's under the survey dir
	    FilePath inputfile( fname );
	    inputfile.makeCanonical();

	    mDynamicCastGet(IOStream*,iostrm,outctio_.ioobj)
	    if ( iostrm )
	    {
		FilePath seismicsdir( iostrm->fullDirName() );
		seismicsdir.makeCanonical();

		if ( inputfile.makeRelativeTo( seismicsdir ) )
		    fname = inputfile.fullPath();
	    }
	}

	const int seltyp = typefld->getIntValue();
	if ( !seltyp )
	    outctio_.ioobj->pars().removeWithKey( "Type" );
	else
	    outctio_.ioobj->pars().set( sKey::Type(), seltyp == 1 ?
				        sKey::Attribute() : sKey::Steering() );

	outctio_.ioobj->setTranslator( CBVSSeisTrcTranslator::translKey() );
	if ( !dolink )
	    inctio_.setObj( getfInpIOObj(fname) );
	else
	{
	    mDynamicCastGet(IOStream*,iostrm,outctio_.ioobj);
	    iostrm->setFileName( fname );
	}
    }

    if ( !IOM().commitChanges(*outctio_.ioobj) )
    {
	uiMSG().error( tr("Cannot write new file\nSee log file for details") );
	return false;
    }

    if ( dolink )
    {
	uiMSG().message( tr("Import successful") );
	return false;
    }

    uiSeisIOObjInfo ioobjinfo( *outctio_.ioobj, true );
    if ( !ioobjinfo.checkSpaceLeft(transffld->spaceInfo()) )
	{ rmTmpIOObj(); return false; }

    const char* titl = oinpfld ? "Copying seismic data"
			       : "Importing CBVS seismic cube";
    PtrMan<Executor> stp = transffld->getTrcProc( *inctio_.ioobj,
				*outctio_.ioobj, titl, "Loading data" );
    if ( !stp )
	{ rmTmpIOObj(); return false; }

    mDynamicCastGet(SeisSingleTraceProc*,sstp,stp.ptr())
    sstp_ = sstp;

    if ( inctio_.ioobj->pars().isTrue( VelocityDesc::sKeyIsVelocity() ) )
	sstp_->proctobedone_.notify( mCB(this,uiSeisImpCBVS,procToBeDoneCB) );

    if ( ismc_ )
    {
	SeisTrcReader& rdr = const_cast<SeisTrcReader&>( *sstp_->reader(0) );
	rdr.setComponent( compfld_->box()->currentItem() - 1 );
    }

    uiTaskRunner dlg( this );
    const bool rv mUnusedVar = TaskRunner::execute( &dlg, *stp ) &&
			!ioobjinfo.is2D() && ioobjinfo.provideUserInfo();
    rmTmpIOObj();
    return false;
}


void uiSeisImpCBVS::procToBeDoneCB( CallBacker* c )
{
    SeisTrc& trc = sstp_->getTrace();
    const SeisTrc intrc = sstp_->getInputTrace();
    const FixedString typestr =
	outctio_.ioobj->pars().find( VelocityDesc::sKeyVelocityType() );
    if ( typestr.isEmpty() ) return;

    const int compnr = compfld_ && compfld_->box()->visible() ?
		compfld_->box()->currentItem() - 1 : 0;
    TypeSet<float> trcvals;
    TypeSet<float> timevals;
    const int sizein = intrc.data().size(compnr);
    const float sampstep = intrc.info().sampling.step;
    for ( int idx=0; idx<sizein; idx++ )
    {
	trcvals += intrc.get( idx, compnr );
	timevals += intrc.startPos() + idx * sampstep;
    }

    const float* vin = trcvals.arr();
    const float* tin = timevals.arr();
    const int sizeout = trc.data().size(compnr);
    const SamplingData<double> sdout = trc.info().sampling;
    mAllocVarLenArr( float, vout, sizeout );
    if ( !mIsVarLenArrOK(vout) ) return;

    if ( typestr == VelocityDesc::TypeNames()[VelocityDesc::Interval] )
	sampleVint( vin, tin, sizein, sdout, vout, sizeout );
    else if ( typestr == VelocityDesc::TypeNames()[VelocityDesc::RMS] )
	sampleVrms( vin, 0, 0, tin, sizein, sdout, vout, sizeout );
    else if ( typestr == VelocityDesc::TypeNames()[VelocityDesc::Avg] )
	sampleVavg( vin, tin, sizein, sdout, vout, sizeout );

    for ( int idx=0; idx<sizeout; idx++ )
	trc.set( idx, vout[idx], compnr );
}


class Seis2DCopier : public Executor
{
public:
Seis2DCopier( const IOObj* inobj, const IOObj* outobj, const IOPar& par )
    : Executor("Copying 2D Seismic Data")
    , lineidx_(-1)
    , inobj_(inobj),outobj_(outobj)
    , rdr_(0),wrr_(0)
    , doscale_(false)
{
    PtrMan<IOPar> lspar = par.subselect( "Line" );
    if ( !lspar ) return;

    for ( int idx=0; idx<1024; idx++ )
    {
	PtrMan<IOPar> linepar = lspar->subselect( idx );
	if ( !linepar )
	    break;

	Pos::GeomID geomid = Survey::GeometryManager::cUndefGeomID();
	if ( !linepar->get(sKey::GeomID(),geomid) )
	    continue;

	StepInterval<int> trcrg;
	StepInterval<float> zrg;
	if ( !linepar->get(sKey::TrcRange(),trcrg) ||
		!linepar->get(sKey::ZRange(),zrg))
	    continue;

	selgeomids_ += geomid;
	trcrgs_ += trcrg;
	zrgs_ += zrg;
    }

    FixedString scalestr = par.find( sKey::Scale() );
    if ( scalestr )
    {
	doscale_ = true;
	scaler_.fromString( scalestr );
    }
}


~Seis2DCopier()
{
    delete rdr_; delete wrr_;
}


bool initNextLine()
{
    delete rdr_; delete wrr_;
    rdr_ = 0; wrr_ = 0;
    if ( inobj_ )
	rdr_ = new SeisTrcReader( inobj_ );

    if ( outobj_ )
	wrr_ = new SeisTrcWriter( outobj_ );

    if ( !rdr_ || !wrr_ )
	return false;

    lineidx_++;
    if ( lineidx_ >= selgeomids_.size() || lineidx_ >= trcrgs_.size() )
	return false;

    sd_.cubeSampling().hrg.setCrlRange( trcrgs_[lineidx_] );
    sd_.cubeSampling().zrg = zrgs_[lineidx_];
    sd_.setGeomID( selgeomids_[lineidx_] );
    rdr_->setSelData( sd_.clone() );
    Seis::SelData* wrrsd = sd_.clone();
    wrrsd->setIsAll( true );
    wrr_->setSelData( wrrsd );
    rdr_->prepareWork();
    return true;
}


od_int64 totalNr() const
{
    od_int64 nr = 0;
    for ( int idx=0; idx<trcrgs_.size(); idx++ )
	nr += ( trcrgs_[idx].nrSteps() + 1 );

    return nr;
}


od_int64 nrDone() const
{ return nrdone_; }

uiString uiNrDoneText() const
{ return "No. of traces copied"; }

protected:

    const IOObj*		inobj_;
    const IOObj*		outobj_;
    SeisTrcReader*		rdr_;
    SeisTrcWriter*		wrr_;
    Seis::RangeSelData		sd_;

    TypeSet<Pos::GeomID>	selgeomids_;
    TypeSet<StepInterval<int> > trcrgs_;
    TypeSet<StepInterval<float> > zrgs_;
    LinScaler			scaler_;
    bool			doscale_;
    int				lineidx_;
    int				nrdone_;

int nextStep()
{
    if ( lineidx_ < 0 && !initNextLine() )
	return ErrorOccurred();

    SeisTrc trc;
    const int res = rdr_->get( trc.info() );
    if ( res < 0 ) return ErrorOccurred();
    if ( res == 0 ) return initNextLine() ? MoreToDo() : Finished();

    if ( !rdr_->get(trc) )
	return ErrorOccurred();

    if ( doscale_ )
    {
	SeisTrcPropChg stpc( trc );
	stpc.scale( (float) scaler_.factor, (float) scaler_.constant );
    }

    if ( !wrr_->put(trc) )
	return ErrorOccurred();

    nrdone_++;
    return MoreToDo();
}


};


uiSeisCopyLineSet::uiSeisCopyLineSet( uiParent* p, const IOObj* obj )
    : uiDialog(p,Setup("Copy 2D Seismic Data",uiStrings::sEmptyString(),
                       mODHelpKey(mSeisCopyLineSetHelpID) ))
{
    IOObjContext ioctxt = uiSeisSel::ioContext( Seis::Line, true );
    inpfld_ = new uiSeisSel( this, ioctxt, uiSeisSel::Setup(Seis::Line) );

    subselfld_ = new uiSeis2DMultiLineSel( this, "Selet Lines to copy", true );
    subselfld_->attach( alignedBelow, inpfld_ );
    if ( obj )
	subselfld_->setInput( obj->key() );

    scalefld_ = new uiScaler( this, "Scale values", true );
    scalefld_->attach( alignedBelow, subselfld_ );

    ioctxt.forread = false;
    outpfld_ = new uiSeisSel( this, ioctxt, uiSeisSel::Setup(Seis::Line) );
    outpfld_->attach( alignedBelow, scalefld_ );
}


bool uiSeisCopyLineSet::acceptOK( CallBacker* )
{
    IOPar par;
    subselfld_->fillPar( par );
    Scaler* scaler = scalefld_->getScaler();
    if ( scaler )
	par.set( sKey::Scale(), scaler->toString() );

    Seis2DCopier exec( inpfld_->ioobj(), outpfld_->ioobj(), par );
    uiTaskRunner dlg( this );

    return TaskRunner::execute( &dlg, exec );
}
