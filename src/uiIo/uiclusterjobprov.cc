/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Raman K Singh
 Date:          May 2009
________________________________________________________________________

-*/
static const char* rcsID = "$Id: uiclusterjobprov.cc,v 1.12 2011-12-14 13:16:41 cvsbert Exp $";

#include "uiclusterjobprov.h"

#include "uifileinput.h"
#include "uilabel.h"
#include "uimsg.h"
#include "uitaskrunner.h"

#include "dirlist.h"
#include "envvars.h"
#include "executor.h"
#include "file.h"
#include "filepath.h"
#include "hostdata.h"
#include "ioman.h"
#include "iostrm.h"
#include "iopar.h"
#include "jobdescprov.h"
#include "keystrs.h"
#include "oddirs.h"
#include "statrand.h"
#include "strmdata.h"
#include "strmprov.h"
#include "transl.h"


const char* uiClusterJobProv::sKeySeisOutIDKey()
{ return "Output Seismics Key"; }

const char* uiClusterJobProv::sKeyOutputID()
{ return "Output.0.Seismic.ID"; }


static BufferString getDefTempStorDir()
{
    BufferString stordir = "Proc_";
    stordir += HostData::localHostName();
    stordir += "_";
    Stats::RandGen::init();
    stordir += Stats::RandGen::getIndex(100000);
    const FilePath fp( GetDataDir(), "Seismics", stordir );
    if ( !File::createDir(fp.fullPath()) )
	return BufferString(File::getTempPath());

    return fp.fullPath();
}


class ClusterJobCreator : public Executor
{
public:
ClusterJobCreator( const InlineSplitJobDescProv& jobprov, const char* dir,
       		   const char* prognm )
    : Executor("Job generator")
    , jobprov_(jobprov),dirnm_(dir),prognm_(prognm)
    , curidx_(0)
{
    FilePath fp( dirnm_.buf() ); fp.add( "X" );
    DirList dl( dirnm_.buf(), DirList::FilesOnly );
    for ( int idx=0; idx<dl.size(); idx++ )
    {
	fp.setFileName( dl.get(idx) );
	File::remove( fp.fullPath() );
    }
}

od_int64 nrDone() const
{ return curidx_; }

const char* nrDoneText() const
{ return "Nr jobs created"; }

od_int64 totalNr() const
{ return jobprov_.nrJobs(); }



#define mSetEnvVar(s) \
    *sd.ostrm << "setenv " << s << " " << GetEnvVar(s) << std::endl;
static bool writeScriptFile( const char* scrfnm, const char* parfnm,
			     const char* logfnm, const char* prognm )
{
    StreamData sd = StreamProvider(scrfnm).makeOStream();
    if ( !sd.usable() )
	return false;

    *sd.ostrm << "#!/bin/csh -f " << std::endl;
    mSetEnvVar("DTECT_APPL")
    mSetEnvVar("DTECT_DATA")
    mSetEnvVar("LD_LIBRARY_PATH")
    *sd.ostrm << GetExecScript(false) << " " << prognm << "\\" << std::endl;
    *sd.ostrm << parfnm << std::endl;
    *sd.ostrm << "echo \"exited with code ${status}\" >>\\" << std::endl;
    *sd.ostrm << logfnm << std::endl;
    *sd.ostrm << "rm -f " << parfnm << std::endl;
    *sd.ostrm << "rm -f $0" << std::endl;
    *sd.ostrm << "echo \"removed script with ${status}\" >>\\" << std::endl;
    *sd.ostrm << logfnm << std::endl;
    sd.close();
    File::setPermissions( scrfnm, "711", 0 );
    return true;
}


int nextStep()
{
    if ( curidx_ >= totalNr() )
	return Finished();

    IOPar iop;
    jobprov_.getJob( curidx_++, iop );
    BufferString filenm( "Job" );
    filenm += curidx_;
    FilePath fp( dirnm_.buf() );
    fp.add( filenm );
    fp.setExtension( "par" );
    BufferString parfnm = fp.fullPath();
    fp.setExtension( "log" );
    BufferString logfnm = fp.fullPath();
    iop.set( sKey::LogFile, logfnm );
    if ( !iop.write(parfnm.buf(),sKey::Pars) )
	return ErrorOccurred();

    fp.setExtension( "scr" );
    BufferString scrfnm = fp.fullPath();
    if ( !writeScriptFile(scrfnm.buf(),parfnm.buf(),logfnm.buf(),prognm_.buf()))
	return ErrorOccurred();

    return MoreToDo();
}

protected:
	
	const InlineSplitJobDescProv&	jobprov_;
	BufferString			dirnm_;
	BufferString			prognm_;
	int				curidx_;	

};


uiClusterJobProv::uiClusterJobProv( uiParent* p, const IOPar& iop,
       				    const char* prognm, const char* parfnm )
    : uiDialog(p,uiDialog::Setup("Cluster job generator","","101.2.2")
	    		   .oktext("Continue"))
    , prognm_(prognm)
    , tempstordir_(getDefTempStorDir())
    , iopar_(*new IOPar(iop))
{
    jobprov_ = new InlineSplitJobDescProv( iop );

    nrinlfld_ = new uiGenInput( this, "No. of inlines per job",
	    			IntInpSpec(1) );
    nrinlfld_->valuechanging.notify( mCB(this,uiClusterJobProv,nrJobsCB) );
	
    nrjobsfld_ = new uiLabel( this, "Total no. of jobs: 0000" );
    nrjobsfld_->attach( alignedBelow, nrinlfld_ );

    parfilefld_ = new uiFileInput( this, "Par file",
	    	uiFileInput::Setup(uiFileDialog::Gen,parfnm)
		.forread(false).filter("*.par;;").confirmoverwrite(false) );
    parfilefld_->attach( alignedBelow, nrjobsfld_ );

    tmpstordirfld_ = new uiFileInput( this, "Temporary storage directory",
	   			      tempstordir_.buf() );
    tmpstordirfld_->setSelectMode( uiFileDialog::DirectoryOnly );
    tmpstordirfld_->attach( alignedBelow, parfilefld_ );

    FilePath fp( GetProcFileName(0) );
    fp.add( "scriptdir" );
    if ( !File::isDirectory(fp.fullPath()) )
	File::createDir( fp.fullPath() );
    scriptdirfld_ = new uiFileInput( this, "Storage directory for scripts",
	   			     fp.fullPath() );
    scriptdirfld_->setSelectMode( uiFileDialog::DirectoryOnly );
    scriptdirfld_->attach( alignedBelow, tmpstordirfld_ );

    fp.setFileName( "clusterprocscript" );
    masterscriptfld_ = new uiFileInput( this, "Main script file",
	    		uiFileInput::Setup(fp.fullPath()).forread(false) );
    masterscriptfld_->attach( alignedBelow, scriptdirfld_ );

    cmdfld_ = new uiGenInput( this, "Cluster Processing command",
	   		      StringInpSpec("srun") );
    cmdfld_->attach( alignedBelow, masterscriptfld_ );

    postFinalise().notify( mCB(this,uiClusterJobProv,nrJobsCB) );
}


uiClusterJobProv::~uiClusterJobProv()
{
    delete jobprov_; delete &iopar_;
}


void uiClusterJobProv::nrJobsCB( CallBacker* )
{
    jobprov_->setNrInlsPerJob( nrinlfld_->getIntValue() );
    BufferString lbltxt( "Total no. of jobs: " );
    lbltxt += jobprov_->nrJobs();
    nrjobsfld_->setText( lbltxt.buf() );
    return;
}


#define mErrRet(s) { uiMSG().error(s); return false; }
bool uiClusterJobProv::acceptOK( CallBacker* )
{
    MouseCursorChanger cursorchanger( MouseCursor::Wait );
    const int nrinlperjob = nrinlfld_->getIntValue();
    if ( mIsUdf(nrinlperjob) || nrinlperjob < 1 )
	mErrRet( "Please specify number of inlines per job")

    BufferString parfnm = parfilefld_->fileName();
    if ( parfnm.isEmpty() )
	mErrRet( "Please enter a valid par file name")

    BufferString tmpdir = tmpstordirfld_->fileName();
    if ( tmpdir.isEmpty() || !File::isDirectory(tmpdir) )
	mErrRet( "Please make a valid entry for temporary storage directory")

    BufferString scriptdir = scriptdirfld_->fileName();
    if ( scriptdir.isEmpty() || !File::isDirectory(scriptdir) )
	mErrRet( "Please make a valid entry for script storage directory")

    BufferString mainscrnm = masterscriptfld_->fileName();
    if ( mainscrnm.isEmpty() )
	mErrRet( "Please make a valid entry for Main script file")

    if ( tempstordir_ != tmpdir )
	File::remove( tempstordir_.buf() );

    const MultiID tmpid = getTmpID( tmpdir.buf() );
    MultiID outseisid;
    iopar_.get( getOutPutIDKey(), outseisid );
    iopar_.set( getOutPutIDKey(), tmpid );
    delete jobprov_;
    jobprov_ = new InlineSplitJobDescProv( iopar_ );
    jobprov_->setNrInlsPerJob( nrinlperjob );
    iopar_.set( "Output.ID", outseisid );
    iopar_.set( "Script dir", scriptdir.buf() );
    iopar_.set( sKey::TmpStor, tmpdir.buf() );
    if ( !iopar_.write(parfnm.buf(),sKey::Pars) )
	mErrRet("Failed to write parameter file")

    return createJobScripts( scriptdir.buf() )
    	&& createMasterScript( parfnm.buf(), scriptdir.buf() );
}


bool uiClusterJobProv::createJobScripts( const char* scriptdir )
{
    if ( !jobprov_ || !jobprov_->nrJobs() )
	mErrRet("No jobs to generate")

    ClusterJobCreator exec( *jobprov_, scriptdir, prognm_ );
    uiTaskRunner dlg( this );
    return dlg.execute( exec );
}


const char* uiClusterJobProv::getOutPutIDKey() const
{
    FixedString res = iopar_.find( sKeySeisOutIDKey() );
    if ( !res )
	res = sKeyOutputID();

    return res;
}


MultiID uiClusterJobProv::getTmpID( const char* tmpdir ) const
{
    CtxtIOObj ctio( IOObjContext(&TranslatorGroup::getGroup("Seismic Data",
		    					     true)) );
    ctio.ctxt.stdseltype = IOObjContext::Seis;
    FilePath fp( tmpdir );
    BufferString objnm( "~" );
    objnm += fp.fileName();
    ctio.setName( objnm );
    IOM().to( ctio.ctxt.getSelKey() );
    IOM().getEntry( ctio );
    if ( !ctio.ioobj )
	return MultiID(-1);

    fp.add( "i.*");
    MultiID ret = ctio.ioobj->key();
    mDynamicCastGet(IOStream*,iostrm,ctio.ioobj)
    if ( !iostrm ) return MultiID(-1);

    StepInterval<int> fnrs;
    jobprov_->getRange( fnrs );
    iostrm->fileNumbers() = fnrs;
    iostrm->setFileName( fp.fullPath() );
    IOM().commitChanges( *iostrm );
    return ret;
}


bool uiClusterJobProv::createMasterScript( const char* parfnm,
					   const char* scriptdir ) const
{
    BufferString masterscript = masterscriptfld_->fileName();
    StreamData sd = StreamProvider(masterscript.buf()).makeOStream();

    FilePath fp( scriptdir );
    fp.add( "*.scr" );
    BufferString cmscmd = cmdfld_->text();
    if ( cmscmd.isEmpty() )
	mErrRet("Please enter a valid command to run the job")

    *sd.ostrm << "#!/bin/csh -f " << std::endl;
    *sd.ostrm << GetExecScript(false) << " ";
    *sd.ostrm << "od_ClusterProc" << "\\" << std::endl;
    *sd.ostrm << parfnm << " &" << std::endl;
    *sd.ostrm << "foreach file ( " << fp.fullPath() << " )" << std::endl;
    *sd.ostrm << "    " << cmscmd.buf() << " $file &" << std::endl;
    *sd.ostrm << "end" << std::endl;
    sd.close();
    File::setPermissions( masterscript.buf(), "744", 0 );
    BufferString msg( "The script file " );
    msg += masterscript;
    msg += " has been created successfully. Execute now?";
    if ( uiMSG().askGoOn(msg.buf()) )
	return !system(masterscript.buf());
    return true;
}
