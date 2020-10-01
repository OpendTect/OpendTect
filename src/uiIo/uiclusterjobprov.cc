/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Raman K Singh
 Date:          May 2009
________________________________________________________________________

-*/

#include "uiclusterjobprov.h"

#include "uigeninput.h"
#include "uifilesel.h"
#include "uilabel.h"
#include "uimsg.h"
#include "uitaskrunner.h"

#include "dbman.h"
#include "dirlist.h"
#include "envvars.h"
#include "executor.h"
#include "file.h"
#include "filepath.h"
#include "genc.h"
#include "hostdata.h"
#include "iopar.h"
#include "iostrm.h"
#include "jobdescprov.h"
#include "keystrs.h"
#include "oddirs.h"
#include "od_helpids.h"
#include "od_ostream.h"
#include "oscommand.h"
#include "settings.h"
#include "statrand.h"
#include "transl.h"


const char* uiClusterJobProv::sKeySeisOutIDKey()
{ return "Output Seismics Key"; }

const char* uiClusterJobProv::sKeyOutputID()
{ return "Output.0.Seismic.ID"; }


static BufferString getDefTempStorDir()
{
    BufferString stordir = "Proc_";
    stordir += GetLocalHostName();
    stordir += "_";
    stordir += Stats::randGen().getIndex(100000);
    const File::Path fp( GetDataDir(), sSeismicSubDir(), stordir );
    if ( !File::createDir(fp.fullPath()) )
	return BufferString(File::getTempPath());

    return fp.fullPath();
}


class ClusterJobCreator : public Executor
{ mODTextTranslationClass(ClusterJobCreator);
public:
ClusterJobCreator( const InlineSplitJobDescProv& jobprov, const char* dir,
		   const char* prognm )
    : Executor("Job generator")
    , jobprov_(jobprov),dirnm_(dir),prognm_(prognm)
    , curidx_(0)
{
    File::Path fp( dirnm_.buf() ); fp.add( "X" );
    DirList dl( dirnm_.buf(), File::FilesInDir );
    for ( int idx=0; idx<dl.size(); idx++ )
    {
	fp.setFileName( dl.get(idx) );
	File::remove( fp.fullPath() );
    }
}


uiString message() const
{ return tr("Creating jobs"); }

od_int64 nrDone() const
{ return curidx_; }

uiString nrDoneText() const
{ return tr("Nr jobs created"); }

od_int64 totalNr() const
{ return jobprov_.nrJobs(); }



#define mSetEnvVar(s) \
{ \
    const char* envval = GetEnvVar( s ); \
    strm << "setenv " << s << " " << (envval ? envval : "") << od_endl; \
}

static bool writeScriptFile( const char* scrfnm, const char* prognm,
			     const char* desc )
{
    od_ostream strm( scrfnm );
    if ( !strm.isOK() )
	return false;

    strm << "#!/bin/csh -f " << od_endl;

    strm << "setenv DTECT_DATA " << GetBaseDataDir() << od_endl;
    strm << GetUnixExecScript() << " " << prognm << " \\" << od_endl;
    File::Path fp( scrfnm );
    fp.setExtension( ".par" );
    strm << fp.fullPath().buf() << od_endl;
    strm << "set exitcode = $status" << od_endl;
    strm << "echo \""; strm << desc;
    strm << " finished with code ${exitcode}\" >> \\" << od_endl;
    fp.setExtension( ".log" );
    strm << fp.fullPath().buf() << od_endl;
    strm << "exit ${exitcode}" << od_endl;
    strm.close();
    File::setPermissions( scrfnm, "711", 0 );
    return true;
}


int nextStep()
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
    File::Path fp( dirnm_.buf() );
    fp.add( filenm );
    fp.setExtension( sParFileExtension() );
    BufferString parfnm = fp.fullPath();
    fp.setExtension( "log" );
    BufferString logfnm = fp.fullPath();
    iop.set( sKey::LogFile(), logfnm );
    iop.set( sKey::DataRoot(), GetBaseDataDir() );
    iop.set( sKey::Survey(), DBM().surveyName() );
    if ( !iop.write(parfnm.buf(),sKey::Pars()) )
	return ErrorOccurred();

    fp.setExtension( "scr" );
    BufferString scrfnm = fp.fullPath();
    if ( !writeScriptFile(scrfnm.buf(),prognm_.buf(),desc.buf()))
	return ErrorOccurred();

    return MoreToDo();
}

protected:

	const InlineSplitJobDescProv&	jobprov_;
	BufferString			dirnm_;
	BufferString			prognm_;
	int				curidx_;

};


static const char* sKeyClusterProcCommand()
{ return "dTect.Cluster Proc Command"; }


uiClusterJobProv::uiClusterJobProv( uiParent* p, const IOPar& iop,
				    const char* prognm, const char* parfnm,
				    Batch::ID* batchid )
    : uiDialog(p,uiDialog::Setup(tr("Cluster job generator"),
				 uiString::empty(),
                                  mODHelpKey(mClusterJobProvHelpID) )
			   .oktext(uiStrings::sContinue()))
    , prognm_(prognm)
    , tempstordir_(getDefTempStorDir())
    , iopar_(*new IOPar(iop))
    , batchid_( batchid ? *batchid : Batch::JobDispatcher::getInvalid() )
{
    jobprov_ = new InlineSplitJobDescProv( iop );

    const int nrinl = InlineSplitJobDescProv::defaultNrInlPerJob();
    nrinlfld_ = new uiGenInput( this, tr("Nr of inlines per job"),
				IntInpSpec(nrinl) );
    nrinlfld_->valuechanging.notify( mCB(this,uiClusterJobProv,nrJobsCB) );

    nrjobsfld_ = new uiLabel( this, tr("Total no. of jobs: 0000") );
    nrjobsfld_->attach( alignedBelow, nrinlfld_ );

    uiFileSel::Setup fssu( parfnm );
    fssu.setForWrite().confirmoverwrite( false )
	.setFormat( File::Format::parFiles() );
    parfilefld_ = new uiFileSel( this, uiStrings::sParFile(), fssu );
    parfilefld_->attach( alignedBelow, nrjobsfld_ );

    uiFileSel::Setup dirfssu( tempstordir_ );
    dirfssu.selectDirectory();
    tmpstordirfld_ = new uiFileSel( this, tr("Temporary Storage Directory"),
				    dirfssu );
    tmpstordirfld_->attach( alignedBelow, parfilefld_ );

    File::Path fp( parfnm );
    fp.setExtension( 0 );
    BufferString filenm = fp.fileName();
    filenm += "_scriptdir";
    fp.setFileName( filenm.buf() );
    const BufferString scriptsdirnm( fp.fullPath() );
    if ( !File::isDirectory(scriptsdirnm) )
	File::createDir( scriptsdirnm );
    dirfssu.setFileName( scriptsdirnm );
    scriptdirfld_ = new uiFileSel( this, uiStrings::phrStorageDir(
				     tr("for scripts")), dirfssu );
    scriptdirfld_->attach( alignedBelow, tmpstordirfld_ );

    const Settings& setts = Settings::common();
    BufferString cmd = "srun";
    setts.get( sKeyClusterProcCommand(), cmd );
    cmdfld_ = new uiGenInput( this, tr("Cluster Processing command"),
			      StringInpSpec(cmd) );
    cmdfld_->attach( alignedBelow, scriptdirfld_ );

    postFinalise().notify( mCB(this,uiClusterJobProv,nrJobsCB) );
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


#define mErrRet(s) { uiMSG().error(s); return false; }
bool uiClusterJobProv::acceptOK()
{
    const int nrinlperjob = nrinlfld_->getIntValue();
    if ( mIsUdf(nrinlperjob) || nrinlperjob < 1 )
	mErrRet( uiStrings::phrSpecify(tr("number of inlines per job")) )

    BufferString parfnm = parfilefld_->fileName();
    if ( parfnm.isEmpty() )
	mErrRet( uiStrings::phrEnter(tr("a valid par file name")) )

    BufferString tmpdir = tmpstordirfld_->fileName();
    if ( tmpdir.isEmpty() || !File::isDirectory(tmpdir) )
	mErrRet(tr("Please make a valid entry for temporary storage directory"))

    BufferString scriptdir = scriptdirfld_->fileName();
    if ( scriptdir.isEmpty() || !File::isDirectory(scriptdir) )
	mErrRet( tr("Please make a valid entry for script storage directory"))

    if ( tempstordir_ != tmpdir )
	File::remove( tempstordir_.buf() );

    const DBKey tmpid = getTmpID( tmpdir.buf() );
    DBKey outseisid;
    iopar_.get( getOutPutIDKey(), outseisid );
    iopar_.set( getOutPutIDKey(), tmpid );
    delete jobprov_;
    jobprov_ = new InlineSplitJobDescProv( iopar_ );
    jobprov_->setNrInlsPerJob( nrinlperjob );
    iopar_.set( "Output.ID", outseisid );
    iopar_.set( "Script dir", scriptdir.buf() );
    iopar_.set( sKey::TmpStor(), tmpdir.buf() );
    const FixedString cmd = cmdfld_->text();
    if ( cmd.isEmpty() )
	mErrRet(uiStrings::phrEnter(tr("a valid command for submitting jobs")))

    Settings& setts = Settings::common();
    setts.set( sKeyClusterProcCommand(), cmd );
    setts.write();

    uiUserShowWait usw( this, tr("Writing job parameters") );
    iopar_.set( "Command", cmd );
    if ( !iopar_.write(parfnm.buf(),sKey::Pars()) )
	mErrRet(tr("Failed to write parameter file"))
    usw.readyNow();

    if ( !createJobScripts(scriptdir.buf()) )
	mErrRet(tr("Failed to split jobs"))

    uiString msg = tr("Job scripts "
		      "have been created successfully. Execute now?");
    if ( uiMSG().askGoOn(msg) )
    {
	OS::MachineCommand machcomm( "od_ClusterProc" );
	machcomm.addKeyedArg( "dosubmit", parfnm );
	Batch::JobDispatcher::addIDTo( batchid_, machcomm );
	OS::CommandLauncher cl( machcomm );
	if ( !cl.execute(OS::RunInBG) )
	{
	    uiMSG().error( cl.errorMsg() );
	    return false;
	}
    }

    return true;
}


bool uiClusterJobProv::createJobScripts( const char* scriptdir )
{
    if ( !jobprov_ || !jobprov_->nrJobs() )
	mErrRet(tr("No jobs to generate"))

    ClusterJobCreator exec( *jobprov_, scriptdir, prognm_ );
    uiTaskRunner dlg( this );
    return TaskRunner::execute( &dlg, exec);
}


const char* uiClusterJobProv::getOutPutIDKey() const
{
    FixedString res = iopar_.find( sKeySeisOutIDKey() );
    if ( !res )
	res = sKeyOutputID();

    return res;
}


DBKey uiClusterJobProv::getTmpID( const char* tmpdir ) const
{
    CtxtIOObj ctio( IOObjContext(&TranslatorGroup::getGroup("Seismic Data")) );
    ctio.ctxt_.stdseltype_ = IOObjContext::Seis;
    File::Path fp( tmpdir );
    BufferString objnm( "~" );
    objnm += fp.fileName();
    ctio.setName( objnm );
    DBM().getEntry( ctio );
    if ( !ctio.ioobj_ )
	return DBKey::getInvalid();

    fp.add( "i.*");
    DBKey ret = ctio.ioobj_->key();
    mDynamicCastGet(IOStream*,iostrm,ctio.ioobj_)
    if ( !iostrm ) return DBKey::getInvalid();

    StepInterval<int> fnrs;
    jobprov_->getRange( fnrs );
    iostrm->fileSpec().setFileName( fp.fullPath() );
    iostrm->fileSpec().nrs_ = fnrs;
    iostrm->commitChanges();
    return ret;
}


bool Batch::SimpleClusterProgDef::isSuitedFor( const char* pnm ) const
{
    FixedString prognm = pnm;
    return prognm == Batch::JobSpec::progNameFor( Batch::JobSpec::Attrib )
	|| prognm == Batch::JobSpec::progNameFor( Batch::JobSpec::AttribEM );
}


bool Batch::SimpleClusterProgDef::canHandle( const JobSpec& js ) const
{
    if ( !isSuitedFor(js.prognm_) )
	return false;

    FixedString outtyp = js.pars_.find(
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
    jobspec_.pars_.set( sKey::Survey(), DBM().surveyName() );
    uiClusterJobProv dlg( p, jobspec_.pars_, jobspec_.prognm_,
			  jd_.parfnm_, batchid );
    return dlg.go();
}

