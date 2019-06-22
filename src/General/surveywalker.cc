/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : A. Huck
 * DATE     : September 2018
-*/

#include "surveywalker.h"

#include "dbman.h"
#include "file.h"
#include "filepath.h"
#include "od_ostream.h"
#include "oddirs.h"
#include "survinfo.h"
#include "uistrings.h"


Survey::Walker::Walker( const char* execnm, const char* dataroot )
    : Survey::Walker(execnm,BufferStringSet(1,dataroot))
{
}


Survey::Walker::Walker( const char* execnm, const BufferStringSet& dataroots )
    : Executor(execnm)
{
    for ( auto dataroot : dataroots )
	addSurveys( dataroot->str() );
}


Survey::Walker::~Walker()
{
}


uiString Survey::Walker::message() const
{
    return errmsgs_.isEmpty() ? uiString::empty()
			      : errmsgs_.messages().cat();
}


uiString Survey::Walker::nrDoneText() const
{
    return tr("%1 %2").arg( uiStrings::sSurveys() ).arg( uiStrings::sDone() );
}


od_int64 Survey::Walker::totalNr() const
{
    return allfullpaths_.size();
}


void Survey::Walker::addSurveys( const char* dataroot )
{
    uiRetVal uirv = DBMan::isValidDataRoot( dataroot ? dataroot
						     : GetBaseDataDir() );
    if ( !uirv.isOK() )
    {
	errmsgs_.add( uirv );
	return;
    }

    BufferStringSet surveydirnms;
    Survey::getDirectoryNames( surveydirnms, true, dataroot );
    if ( surveydirnms.isEmpty() )
	return;

    for ( auto surveydirnm : surveydirnms )
    {
	uirv = DBMan::isValidSurveyDir( surveydirnm->str() );
	if ( !uirv.isOK() )
	    { errmsgs_.add( uirv ); continue; }

	if ( allfullpaths_.isPresent(surveydirnm->str()) )
	{
	    pErrMsg("Duplicate survey path found");
	    continue;
	}

	allfullpaths_.add( surveydirnm->str() );
    }
}


bool Survey::Walker::goImpl( od_ostream* strm, bool first, bool last, int delay)
{
    nrdone_ = 0;
    init();
    if ( !errmsgs_.isOK() )
	return false;

    const BufferString initialsurvdir( GetDataDir() );
    const bool success = Executor::goImpl( strm, first, last, delay ) &&
			 errmsgs_.isEmpty();
    DBM().setDataSource( initialsurvdir );
    return success;
}


int Survey::Walker::nextStep()
{
    uiRetVal uirv = DBM().setDataSource(
		allfullpaths_.get(mCast(BufferString::idx_type,nrdone_++)) );
    if ( !uirv.isOK() )
    {
	errmsgs_.add( uirv );
	return nrdone_ >= totalNr() ? Finished() : MoreToDo();
    }

    const bool success = handleSurvey( uirv );
    if ( !uirv.isOK() )
	errmsgs_.add( uirv );

    return success ? ( nrdone_ >= totalNr() ? Finished()
					    : MoreToDo() )
		   : ErrorOccurred();
}


Survey::SICollector::SICollector( const char* dataroot,
				  CollectorType type )
    : Survey::SICollector(BufferStringSet(1,dataroot),type)
{
}


Survey::SICollector::SICollector( const BufferStringSet& dataroots,
				  CollectorType type )
    : Survey::Walker( "SurveyInfo Collector", dataroots )
    , type_(type)
{
}


Survey::SICollector::~SICollector()
{
    cleanup();
}


void Survey::SICollector::cleanup()
{
    deepErase( pars_ );
}


void Survey::SICollector::setOutputFolder( const char* dirpath, uiString* msg )
{
    if ( !File::isDirectory(dirpath) )
    {
	const File::Path dirbasepath( dirpath );
	const BufferString dirbasenm( dirbasepath.pathOnly() );
	if ( !File::exists(dirbasenm) || !File::isWritable(dirbasenm) )
	    return;

	File::createDir( dirpath );
    }

    if ( !File::isWritable(dirpath) )
    {
	if ( msg )
	    msg->set( uiStrings::phrCannotOpenForWrite(dirpath) );

	return;
    }

    outputpath_.set( dirpath );
    type_ = FileSummary;
}


bool Survey::SICollector::fillPar( ObjectSet<IOPar>& pars ) const
{
    if ( pars_.isEmpty() )
    {
	Survey::SICollector& siexec = const_cast<Survey::SICollector&>( *this );
	if ( !siexec.execute() )
	    return false;
    }

    if ( pars.isManaged() )
	pars.setEmpty();
    else
	deepErase( pars );

    for ( auto par : pars_ )
	pars.add( new IOPar(*par) );

    return true;
}


bool Survey::SICollector::init()
{
    cleanup();
    return true;
}


bool Survey::SICollector::handleSurvey( uiRetVal& uirv )
{
    IOPar par;
    SI().fillPar( par );
    if ( type_ == IOParDump )
    {
	pars_.add( new IOPar(par) );
    }
    else if ( type_ == FileSummary && !outputpath_.isEmpty() )
    {
	File::Path outp( outputpath_,
			 BufferString(DBM().surveyDirectoryName(),"_SI_info") );
	outp.setExtension( "txt" );
	const BufferString outfnm( outp.fullPath() );
	od_ostream strm( outfnm );
	if ( !strm.isOK() )
	{
	    uirv.add( strm.errMsg() );
	    return true;
	}

	par.dumpPretty( strm );
    }

    return true;
}
