/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uiclusterjobprov.h"

#include "uifileinput.h"
#include "uilabel.h"
#include "uimsg.h"
#include "uiseparator.h"
#include "uitaskrunner.h"

#include "dirlist.h"
#include "envvars.h"
#include "executor.h"
#include "file.h"
#include "filepath.h"
#include "genc.h"
#include "hostdata.h"
#include "ioman.h"
#include "iopar.h"
#include "iostrm.h"
#include "jobdescprov.h"
#include "keystrs.h"
#include "oddirs.h"
#include "od_ostream.h"
#include "oscommand.h"
#include "settings.h"
#include "statrand.h"
#include "systeminfo.h"
#include "transl.h"
#include "od_helpids.h"


const char* uiClusterJobProv::sKeySeisOutIDKey()
{ return "Output Seismics Key"; }

const char* uiClusterJobProv::sKeyOutputID()
{ return "Output.0.Seismic.ID"; }


static BufferString getDefTempStorDir()
{
    static Stats::RandGen gen;
    BufferString stordir( "Proc_" );
    stordir.add( System::localHostNameWoDomain() )
	   .add( "_" ).add( gen.getIndex(100000) );
    const FilePath fp( GetDataDir(), "Seismics", stordir );
    if ( !File::createDir(fp.fullPath()) )
	return BufferString(File::getTempPath());

    return fp.fullPath();
}


static BufferString getExecScript( const char* instdir )
{
    StringView instdirstr = instdir;
    if ( instdirstr == GetSoftwareDir(false) )
	return GetExecScript(0);

    FilePath fp( instdir );
    fp.add( "bin" ).add( "od_exec" );
    return fp.fullPath( FilePath::Unix );
}


static BufferString convertPath( const char* fullpath,
				 const char* localdir,
				 const char* clusterdir )
{
    FilePath relpath;
    FilePath fp( fullpath );
    if ( !fp.isSubDirOf(localdir,&relpath) )
	return fullpath;

    fp.set( clusterdir );
    fp.add( relpath.fullPath() );
    return fp.fullPath( FilePath::Unix );
}



class ClusterJobCreator : public Executor
{ mODTextTranslationClass(ClusterJobCreator);
public:
ClusterJobCreator( const InlineSplitJobDescProv& jobprov, const char* scriptdir,
		   const char* dataroot, const char* instdir,
		   const char* prognm )
    : Executor("Job generator")
    , jobprov_(jobprov),dirnm_(scriptdir),prognm_(prognm)
    , localdataroot_(GetBaseDataDir())
    , dataroot_(dataroot), instdir_(instdir)
    , curidx_(0)
{
    FilePath fp( dirnm_.buf() ); fp.add( "X" );
    DirList dl( dirnm_.buf(), File::FilesInDir );
    for ( int idx=0; idx<dl.size(); idx++ )
    {
	fp.setFileName( dl.get(idx) );
	File::remove( fp.fullPath() );
    }
}

od_int64 nrDone() const override
{ return curidx_; }

uiString uiNrDoneText() const override
{ return tr("Nr jobs created"); }

od_int64 totalNr() const override
{ return jobprov_.nrJobs(); }



#define mSetEnvVar(s) \
{ \
    const char* envval = GetEnvVar( s ); \
    strm << "setenv " << s << " " << (envval ? envval : "") << od_endl; \
}


BufferString getClusterPath( const char* fnm )
{
    return convertPath( fnm, localdataroot_, dataroot_ );
}


bool writeScriptFile( const char* scrfnm, const char* desc )
{
    od_ostream strm( scrfnm );
    if ( !strm.isOK() )
	return false;

    strm << "#!/bin/csh -f " << od_endl;

    strm << "setenv DTECT_DATA " << dataroot_ << od_endl;
    mSetEnvVar("LD_LIBRARY_PATH")
    mSetEnvVar("OD_APPL_PLUGIN_DIR")
    mSetEnvVar("OD_USER_PLUGIN_DIR")
    strm << getExecScript(instdir_) << " " << prognm_ << " \\" << od_endl;
    FilePath fp( scrfnm );
    fp.setExtension( ".par" );
    strm << getClusterPath(fp.fullPath().buf()) << od_endl;
    strm << "set exitcode = $status" << od_endl;
    strm << "echo \""; strm << desc;
    strm << " finished with code ${exitcode}\" >> \\" << od_endl;
    fp.setExtension( ".log" );
    strm << getClusterPath(fp.fullPath().buf()) << od_endl;
    strm << "exit ${exitcode}" << od_endl;
    strm.close();
    File::setPermissions( scrfnm, "711", 0 );
    return true;
}


int nextStep() override
{
    if ( curidx_ >= totalNr() )
	return Finished();

    IOPar iop;
    jobprov_.getJob( curidx_, iop );
    BufferString desc( "In-line " );
    desc.add( jobprov_.objName(curidx_++) );
    iop.set( sKey::Desc(), desc.buf() );
    BufferString filenm( "Job" );
    filenm += curidx_;
    FilePath fp( dirnm_.buf() );
    fp.add( filenm );
    fp.setExtension( "par" );
    BufferString parfnm = fp.fullPath();
    fp.setExtension( "log" );
    BufferString logfnm = getClusterPath( fp.fullPath() );
    iop.set( sKey::LogFile(), logfnm );
    iop.set( sKey::DataRoot(), dataroot_ );
    iop.set( sKey::Survey(), IOM().surveyName() );
    if ( !iop.write(parfnm.buf(),sKey::Pars()) )
	return ErrorOccurred();

    fp.setExtension( "scr" );
    BufferString scrfnm = fp.fullPath();
    if ( !writeScriptFile(scrfnm.buf(),desc.buf()) )
	return ErrorOccurred();

    return MoreToDo();
}

protected:

    const InlineSplitJobDescProv&	jobprov_;
    BufferString			dirnm_;
    BufferString			prognm_;
    BufferString			localdataroot_;
    BufferString			dataroot_;
    BufferString			instdir_;
    int					curidx_;

};


static const char* sKeyClusterProcCommand()
{ return "dTect.Cluster Proc Command"; }

static const char* sKeyClusterDataRoot()
{ return "dTect.Data Root"; }

static const char* sKeyClusterInstDir()
{ return "dTect.Installation Dir"; }

uiClusterJobProv::uiClusterJobProv( uiParent* p, const IOPar& iop,
				    const char* prognm, const char* parfnm,
				    Batch::ID* batchid )
    : uiDialog(p,uiDialog::Setup(tr("Cluster job generator"),
				 uiString::emptyString(),
				 mODHelpKey(mClusterJobProvHelpID))
			.oktext(uiStrings::sContinue()))
    , iopar_(*new IOPar(iop))
    , prognm_(prognm)
    , tempstordir_(getDefTempStorDir())
    , parfnm_(parfnm)
    , datarootfld_(nullptr)
    , instdirfld_(nullptr)
    , batchid_( batchid ? *batchid : Batch::JobDispatcher::getInvalid() )
{
    jobprov_ = new InlineSplitJobDescProv( iop );

    const int nrinl = InlineSplitJobDescProv::defaultNrInlPerJob();
    nrinlfld_ = new uiGenInput( this, tr("Nr of inlines per job"),
				IntInpSpec(nrinl) );
    nrinlfld_->valuechanging.notify( mCB(this,uiClusterJobProv,nrJobsCB) );

    nrjobsfld_ = new uiLabel( this, tr("Total nr of jobs: 0000") );
    nrjobsfld_->attach( rightTo, nrinlfld_ );

    BufferString cmd = "qsub";
    if ( !Settings::common().get(sKeyClusterProcCommand(),cmd) )
	Settings::common().get( "Queue.Submit", cmd );

    cmdfld_ = new uiGenInput( this, tr("Cluster Processing command"),
			      StringInpSpec(cmd) );
    cmdfld_->setElemSzPol( uiObject::Wide );
    cmdfld_->attach( alignedBelow, nrinlfld_ );

    bool showpathflds = __iswin__;
#ifdef __debug__
    showpathflds = true;
#endif
    if ( showpathflds )
    {
	uiSeparator* sep = new uiSeparator( this );
	sep->attach( stretchedBelow, cmdfld_ );

	datarootfld_ = new uiGenInput( this, tr("Survey DataRoot on cluster") );
	datarootfld_->setElemSzPol( uiObject::Wide );
	datarootfld_->attach( alignedBelow, cmdfld_ );
	datarootfld_->attach( ensureBelow, sep );

	instdirfld_ = new uiGenInput( this,
			tr("OpendTect installation folder on cluster") );
	instdirfld_->setElemSzPol( uiObject::Wide );
	instdirfld_->attach( alignedBelow, datarootfld_ );

	BufferString dirstr = GetBaseDataDir();
	Settings::common().get( sKeyClusterDataRoot(), dirstr );
	datarootfld_->setText( dirstr );
	dirstr = GetSoftwareDir( false );
	Settings::common().get( sKeyClusterInstDir(), dirstr );
	instdirfld_->setText( dirstr );
    }

    postFinalize().notify( mCB(this,uiClusterJobProv,nrJobsCB) );
}


uiClusterJobProv::~uiClusterJobProv()
{
    delete jobprov_; delete &iopar_;
}


void uiClusterJobProv::nrJobsCB( CallBacker* )
{
    jobprov_->setNrInlsPerJob( nrinlfld_->getIntValue() );
    uiString lbltxt = tr("Total no. of jobs: %1").arg(jobprov_->nrJobs());
    nrjobsfld_->setText( lbltxt );
    return;
}


static BufferString getScriptDir( const char* parfnm )
{
    FilePath fp( parfnm );
    fp.setExtension( nullptr );
    BufferString filenm = fp.fileName();
    filenm += "_scriptdir";
    fp.setFileName( filenm.buf() );
    if ( !File::isDirectory(fp.fullPath()) )
	File::createDir( fp.fullPath() );

    return fp.fullPath();
}


#define mErrRet(s) { uiMSG().error(s); return false; }
bool uiClusterJobProv::acceptOK( CallBacker* )
{
    const int nrinlperjob = nrinlfld_->getIntValue();
    if ( mIsUdf(nrinlperjob) || nrinlperjob < 1 )
	mErrRet( tr("Please specify number of inlines per job"))

    StringView cmd = cmdfld_->text();
    if ( cmd.isEmpty() )
	mErrRet(tr("Please enter a valid command for submitting jobs"))

    scriptdir_ = getScriptDir( parfnm_ );

    const MultiID tmpid = getTmpID( tempstordir_.buf() );
    MultiID outseisid;
    iopar_.get( getOutPutIDKey(), outseisid );
    iopar_.set( getOutPutIDKey(), tmpid );
    delete jobprov_;
    jobprov_ = new InlineSplitJobDescProv( iopar_ );
    jobprov_->setNrInlsPerJob( nrinlperjob );
    iopar_.set( "Output.ID", outseisid );

    BufferString dataroot = datarootfld_ ? datarootfld_->text() : "";
    if ( dataroot.isEmpty() )
	dataroot = GetBaseDataDir();
    BufferString instdir = instdirfld_ ? instdirfld_->text() : "";
    if ( instdir.isEmpty() )
	instdir = GetSoftwareDir(false);

    Settings& setts = Settings::common();
    setts.set( sKeyClusterProcCommand(), cmd.buf() );
    setts.set( sKeyClusterDataRoot(), dataroot.buf() );
    setts.set( sKeyClusterInstDir(), instdir.buf() );
    setts.write();

    const BufferString clusterscriptdir =
		convertPath(scriptdir_,GetBaseDataDir(),dataroot);
    iopar_.set( "Script dir", clusterscriptdir );
    iopar_.set( sKey::TmpStor(),
		convertPath(tempstordir_,GetBaseDataDir(),dataroot) );
    iopar_.set( sKey::DataRoot(), dataroot );
    iopar_.set( "Command", cmd );
    if ( !iopar_.write(parfnm_.buf(),sKey::Pars()) )
	mErrRet(tr("Failed to write parameter file"))

    if ( !createJobScripts(dataroot,instdir) )
	mErrRet(tr("Failed to split jobs"))

    uiString msg = tr("Job scripts have been created successfully.");
    if ( !__iswin__ )
    {
	uiString execmsg = msg;
	execmsg.append( tr("Execute now?"), true );
	if ( uiMSG().askGoOn(execmsg) )
	{
	    OS::MachineCommand machcomm( "od_ClusterProc" );
	    machcomm.addFlag( "dosubmit" );
	    machcomm.addKeyedArg( "parfile", parfnm_ );
	    OS::CommandLauncher cl( machcomm );
	    if ( !cl.execute(OS::RunInBG) )
	    {
		uiMSG().error( cl.errorMsg() );
		return false;
	    }

	    return true;
	}

	msg.setEmpty();
    }

    msg.append( tr("Scripts are located in %1\n"
		   "and ready to be executed on your cluster")
		.arg(clusterscriptdir), true );
    uiMSG().message( msg );

    return true;
}


bool uiClusterJobProv::createJobScripts( const char* dataroot,
					 const char* instdir )
{
    if ( !jobprov_ || !jobprov_->nrJobs() )
	mErrRet(tr("No jobs to generate"))

    ClusterJobCreator exec( *jobprov_, scriptdir_, dataroot, instdir, prognm_ );
    uiTaskRunner dlg( this );
    return TaskRunner::execute( &dlg, exec);
}


const char* uiClusterJobProv::getOutPutIDKey() const
{
    StringView res = iopar_.find( sKeySeisOutIDKey() );
    if ( !res )
	res = sKeyOutputID();

    return res;
}


MultiID uiClusterJobProv::getTmpID( const char* tmpdir ) const
{
    CtxtIOObj ctio( IOObjContext(&TranslatorGroup::getGroup("Seismic Data")) );
    ctio.ctxt_.stdseltype_ = IOObjContext::Seis;
    FilePath fp( tmpdir );
    BufferString objnm( "~" );
    objnm += fp.fileName();
    ctio.setName( objnm );
    IOM().to( ctio.ctxt_.getSelKey() );
    IOM().getEntry( ctio );
    if ( !ctio.ioobj_ )
	return MultiID::udf();

    fp.add( "i.*");
    MultiID ret = ctio.ioobj_->key();
    mDynamicCastGet(IOStream*,iostrm,ctio.ioobj_)
    if ( !iostrm )
	return MultiID::udf();

    StepInterval<int> fnrs;
    jobprov_->getRange( fnrs );
    iostrm->fileSpec().setFileName( fp.fullPath() );
    iostrm->fileSpec().nrs_ = fnrs;
    IOM().commitChanges( *iostrm );
    return ret;
}


bool Batch::SimpleClusterProgDef::isSuitedFor( const char* pnm ) const
{
    StringView prognm = pnm;
    return prognm == Batch::JobSpec::progNameFor( Batch::JobSpec::Attrib )
	|| prognm == Batch::JobSpec::progNameFor( Batch::JobSpec::AttribEM );
}


bool Batch::SimpleClusterProgDef::canHandle( const JobSpec& js ) const
{
    if ( !isSuitedFor(js.prognm_) )
	return false;

    StringView outtyp = js.pars_.find(
		IOPar::compKey(sKey::Output(),sKey::Type()) );
    return outtyp != sKey::Surface();
}


uiClusterJobDispatcherLauncher::uiClusterJobDispatcherLauncher(
						Batch::JobSpec& js )
    : uiBatchJobDispatcherLauncher(js)
    , jd_(*new Batch::ClusterJobDispatcher)
{}

uiClusterJobDispatcherLauncher::~uiClusterJobDispatcherLauncher()
{ delete &jd_; }

Batch::JobDispatcher& uiClusterJobDispatcherLauncher::gtDsptchr()
{ return jd_; }

bool uiClusterJobDispatcherLauncher::go( uiParent* p, Batch::ID* batchid )
{
    jobspec_.pars_.set( sKey::DataRoot(), GetBaseDataDir() );
    jobspec_.pars_.set( sKey::Survey(), IOM().surveyName() );
    uiClusterJobProv dlg( p, jobspec_.pars_, jobspec_.prognm_,
			  jd_.parfnm_, batchid );
    return dlg.go();
}
