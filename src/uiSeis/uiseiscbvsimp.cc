/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert Bril
 Date:          Jun 2002
________________________________________________________________________

-*/
static const char* rcsID = "$Id: uiseiscbvsimp.cc,v 1.73 2010-11-09 09:16:28 cvsnanne Exp $";

#include "uiseiscbvsimp.h"
#include "uiseisioobjinfo.h"
#include "uiseissel.h"
#include "seistrctr.h"
#include "seisread.h"
#include "seiswrite.h"
#include "seisselection.h"
#include "seissingtrcproc.h"
#include "seisselectionimpl.h"
#include "seistrc.h"
#include "seistrcprop.h"
#include "ioman.h"
#include "iostrm.h"
#include "iopar.h"
#include "ctxtioobj.h"
#include "ptrman.h"
#include "survinfo.h"
#include "file.h"
#include "filepath.h"
#include "oddirs.h"
#include "keystrs.h"
#include "executor.h"
#include "scaler.h"
#include "zdomain.h"

#include "uibutton.h"
#include "uifileinput.h"
#include "uiioobjsel.h"
#include "uicombobox.h"
#include "uimsg.h"
#include "uiscaler.h"
#include "uiseissel.h"
#include "uiseisioobjinfo.h"
#include "uiseistransf.h"
#include "uiseislinesel.h"
#include "uiseisfmtscale.h"
#include "uitaskrunner.h"

uiSeisImpCBVS::uiSeisImpCBVS( uiParent* p )
	: uiDialog(p,Setup("Import CBVS cube",
		    	   "Specify import parameters",
			   "103.0.1"))
	, inctio_(*mMkCtxtIOObj(SeisTrc))
	, outctio_(*uiSeisSel::mkCtxtIOObj(Seis::Vol,false))
    	, modefld(0)
{
    setCtrlStyle( DoAndStay );
    
    init( false );
    modeSel(0);
}


uiSeisImpCBVS::uiSeisImpCBVS( uiParent* p, const IOObj* ioobj )
	: uiDialog(p,Setup("Copy cube data",
		    	   "Specify copy parameters",
			   "103.1.1"))
	, inctio_(*uiSeisSel::mkCtxtIOObj(Seis::Vol,true))
	, outctio_(*uiSeisSel::mkCtxtIOObj(Seis::Vol,false))
    	, modefld(0)
{
    setCtrlStyle( DoAndStay );
    
    if ( ioobj ) inctio_.ioobj = ioobj->clone();
    init( true );
    oinpSel(0);
}


void uiSeisImpCBVS::init( bool fromioobj )
{
    finpfld = 0; modefld = typefld = 0; oinpfld = 0; convertfld = 0;
    compfld_ = 0;
    ismc_ = false;
    setTitleText( fromioobj ? "Specify transfer parameters"
	    		    : "Create CBVS cube definition" );
    tmpid_ = "100010."; tmpid_ += IOObj::tmpID();

    uiSeisTransfer::Setup sts( Seis::Vol );
    uiGroup* attobj = 0;
    if ( fromioobj )
    {
	inctio_.ctxt.forread = true;
	inctio_.ctxt.toselect.allowtransls_ = "CBVS";
	uiSeisSel::Setup sssu( Seis::Vol );
	sssu.steerpol( uiSeisSel::Setup::InclSteer );
	oinpfld = new uiSeisSel( this, inctio_, sssu );
	oinpfld->selectionDone.notify( mCB(this,uiSeisImpCBVS,oinpSel) );
	compfld_ = new uiLabeledComboBox( this, "Component(s)" );
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
	typefld = new uiGenInput( this, "Cube type", spec );
	typefld->attach( alignedBelow, finpfld );
	typefld->valuechanged.notify( mCB(this,uiSeisImpCBVS,typeChg) );

	modefld = new uiGenInput( this, "Import mode",
			  BoolInpSpec(false,"Copy the data","Use in-place") );
	modefld->attach( alignedBelow, typefld );
	modefld->valuechanged.notify( mCB(this,uiSeisImpCBVS,modeSel) );
	attobj = modefld;

	convertfld = new uiCheckBox( this, 
		"Convert underscores to spaces in Output Cube name",
		mCB(this,uiSeisImpCBVS,convertSel) );
    }

    sts.withnullfill(fromioobj).withstep(true).onlyrange(false)
				.fornewentry(true);
    transffld = new uiSeisTransfer( this, sts );
    transffld->attach( alignedBelow, attobj );

    uiSeisSel::Setup sssu( Seis::Vol );
    sssu.steerpol( uiSeisSel::Setup::InclSteer );
    outctio_.ctxt.forread = false;
    outctio_.ctxt.toselect.allowtransls_ = "CBVS";
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
    iostrm->setTranslator( "CBVS" );
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
	    compfld_->box()->empty();
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

    char* ptr = inp.buf();
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
    ptr = strrchr( inp.buf(), '.' );
    if ( ptr && *(ptr+1) == 'c' && *(ptr+2) == 'b' && *(ptr+3) == 'v' 
	 && *(ptr+4) == 's' )
	*(ptr) = '\0';

}


#define rmTmpIOObj() IOM().permRemove( MultiID(tmpid_.buf()) );

#define mErrRet(s) \
	{ uiMSG().error(s); return false; }

bool uiSeisImpCBVS::acceptOK( CallBacker* )
{
    if ( !outfld->commitInput() )
    {
	if ( outfld->isEmpty() )
	    uiMSG().error( "Please choose a name for the output data" );
	return false;
    }

    const bool dolink = modefld && !modefld->getBoolValue();
    if ( oinpfld )
    {
	if ( !oinpfld->commitInput() )
	{
	    uiMSG().error( "Please select an input cube" );
	    return false;
	}
	outctio_.ioobj->pars() = inctio_.ioobj->pars();
    }
    else
    {
	const char* fname = finpfld->text();
	if ( !fname || !*fname )
	{
	    uiMSG().error( "Please select the input filename" );
	    return false;
	}
	const int seltyp = typefld->getIntValue();
	if ( !seltyp )
	    outctio_.ioobj->pars().removeWithKey( "Type" );
	else
	    outctio_.ioobj->pars().set( sKey::Type, seltyp == 1
		    			? sKey::Attribute : sKey::Steering );

	outctio_.ioobj->setTranslator( "CBVS" );
	if ( !dolink )
	    inctio_.setObj( getfInpIOObj(fname) );
	else
	{
	    mDynamicCastGet(IOStream*,iostrm,outctio_.ioobj);
	    iostrm->setFileName( fname );
	}
    }

    IOM().commitChanges( *outctio_.ioobj );
    if ( dolink )
	return true;

    uiSeisIOObjInfo ioobjinfo( *outctio_.ioobj, true );
    if ( !ioobjinfo.checkSpaceLeft(transffld->spaceInfo()) )
	{ rmTmpIOObj(); return false; }

    const char* titl = oinpfld ? "Copying seismic data"
			       : "Importing CBVS seismic cube";
    const char* attrnm = oinpfld ? oinpfld->attrNm() : "";
    PtrMan<Executor> stp = transffld->getTrcProc( *inctio_.ioobj,
	    			*outctio_.ioobj, titl, "Loading data",
				attrnm );
    if ( !stp )
	{ rmTmpIOObj(); return false; }

    if ( ismc_ )
    {
	mDynamicCastGet(SeisSingleTraceProc*,sstp,stp.ptr())
	SeisTrcReader& rdr = const_cast<SeisTrcReader&>( *sstp->reader(0) );
	rdr.setComponent( compfld_->box()->currentItem() - 1 );
    }

    uiTaskRunner dlg( this );
    const bool rv = dlg.execute(*stp) && !ioobjinfo.is2D() &&
		    ioobjinfo.provideUserInfo();

    rmTmpIOObj();
    return rv;
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
    inattrnm_ = par.find( sKey::Attribute );
    PtrMan<IOPar> lspar = par.subselect( "Line" );
    if ( !lspar ) return;

    for ( int idx=0; idx<1024; idx++ )
    {
	PtrMan<IOPar> linepar = lspar->subselect( idx );
	if ( !linepar )
	    break;

	FixedString lnm = linepar->find( sKey::Name );
	StepInterval<int> trcrg;
	if ( !lnm || !linepar->get(sKey::TrcRange,trcrg) )
	    continue;

	sellines_.add( lnm );
	trcrgs_ += trcrg;
    }

    FixedString scalestr = par.find( sKey::Scale );
    if ( scalestr )
    {
	doscale_ = true;
	scaler_.fromString( scalestr );
    }

    StepInterval<float> zrg;
    if ( !par.get(sKey::ZRange,zrg) )
	zrg = SI().zRange( false );

    outattrnm_ = par.find( IOPar::compKey(sKey::Output,sKey::Attribute) );
    sd_.cubeSampling().zrg = zrg;
    sd_.lineKey().setAttrName( inattrnm_.buf() );
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
    if ( lineidx_ >= sellines_.size() || lineidx_ >= trcrgs_.size() )
	return false;

    sd_.cubeSampling().hrg.setCrlRange( trcrgs_[lineidx_] );
    sd_.lineKey().setLineName( sellines_.get(lineidx_).buf() );
    rdr_->setSelData( sd_.clone() );
    Seis::SelData* wrrsd = sd_.clone();
    wrrsd->setIsAll( true );
    wrrsd->lineKey().setAttrName( outattrnm_.buf() );
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

const char* nrDoneText() const
{ return "No. of traces copied"; }

protected:

    const IOObj*		inobj_;
    const IOObj*		outobj_;
    SeisTrcReader*		rdr_;
    SeisTrcWriter*		wrr_;
    Seis::RangeSelData		sd_;

    BufferStringSet		sellines_;
    TypeSet<StepInterval<int> > trcrgs_;
    LinScaler			scaler_;
    bool			doscale_;
    int				lineidx_;
    int				nrdone_;
    BufferString		inattrnm_;
    BufferString		outattrnm_;

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
	stpc.scale( scaler_.factor, scaler_.constant );
    }

    if ( !wrr_->put(trc) )
	return ErrorOccurred();

    nrdone_++;
    return MoreToDo();
}


};


uiSeisCopyLineSet::uiSeisCopyLineSet( uiParent* p, const IOObj* obj )
    : uiDialog(p,Setup("Copy 2D Seismic Data","","103.1.8"))
    , outctio_(*uiSeisSel::mkCtxtIOObj(Seis::Line,false))
{
    uiSeis2DMultiLineSel::Setup su( "Select Lineset to copy" );
    inpfld_ = new uiSeis2DMultiLineSel( this, su.withattr(true).withz(true) );
    if ( obj )
	inpfld_->setLineSet( obj->key() );

    scalefld_ = new uiScaler( this, "Scale values", true );
    scalefld_->attach( alignedBelow, inpfld_ );

    outpfld_ = new uiSeisSel( this, outctio_, uiSeisSel::Setup(Seis::Line) );
    outpfld_->attach( alignedBelow, scalefld_ );
}


uiSeisCopyLineSet::~uiSeisCopyLineSet()
{
    delete &outctio_;
}


bool uiSeisCopyLineSet::acceptOK( CallBacker* )
{
    IOPar par;
    inpfld_->fillPar( par );
    Scaler* scaler = scalefld_->getScaler();
    if ( scaler )
	par.set( sKey::Scale, scaler->toString() );

    par.set( IOPar::compKey(sKey::Output,sKey::Attribute), outpfld_->attrNm() );
    Seis2DCopier exec( inpfld_->getIOObj(), outpfld_->ioobj(true), par );
    uiTaskRunner dlg( this );

    return dlg.execute( exec );
}
