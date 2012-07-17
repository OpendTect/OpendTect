/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Nageswara
 Date:          Feb 2010
 RCS:           $Id: mantisdatabase.cc,v 1.47 2012-07-17 10:28:55 cvsnageswara Exp $
________________________________________________________________________

-*/

#include "mantisdatabase.h"

#include "bufstring.h"
#include "bufstringset.h"
#include "convert.h"
#include "envvars.h"
#include "errh.h"
#include "mantistables.h"
#include "oddirs.h"
#include "odver.h"
#include "ptrman.h"

const char* SqlDB::MantisDBMgr::sKeyAll()
{ return "All"; }
const char* SqlDB::MantisDBMgr::sKeyUnAssigned()
{ return "Un assigned"; }
const char* SqlDB::MantisDBMgr::sKeyProjectTable()
{ return "mantis_project_table"; }
const char* SqlDB::MantisDBMgr::sKeyBugNoteTable()
{ return "mantis_bugnote_table"; }
const char* SqlDB::MantisDBMgr::sKeyBugNoteTextTable()
{ return "mantis_bugnote_text_table"; }
const char* SqlDB::MantisDBMgr::sKeyProjectCategoryTable()
{ return "mantis_project_category_table"; }
const char* SqlDB::MantisDBMgr::sKeyUserTable()
{ return "mantis_user_table"; }
const char* SqlDB::MantisDBMgr::sKeyProjectVersionTable()
{ return "mantis_project_version_table"; }
const char* SqlDB::MantisDBMgr::sKeyProjectUserListTable()
{ return "mantis_project_user_list_table"; }
const char* SqlDB::MantisDBMgr::sKeyBugFileTable()
{ return "mantis_bug_file_table"; }
const int SqlDB::MantisDBMgr::cOpenDtectProjectID() { return 1; }
const int SqlDB::MantisDBMgr::cAccessLevelDeveloper() { return 50; }
const int SqlDB::MantisDBMgr::cAccessLevelCaseStudy() { return 25; }


SqlDB::MantisQuery::MantisQuery( SqlDB::MantisAccess& acc )
    : SqlDB::Query(acc)
{
}


bool SqlDB::MantisQuery::updateTables( BugTableEntry& bugtable,
				       BugTextTableEntry& tte )
{
    BufferStringSet colnms, values;
    bugtable.getQueryInfo( colnms, values, true );
    bool isexec = false;
    const int bugid = bugtable.id_;
    isexec = update( colnms, values, BugTableEntry::sKeyBugTable(), bugid );
    if ( !isexec )
	return false;

    if ( tte.description_.isEmpty() )
	return isexec;
    
    isexec = false;
    colnms.erase(); values.erase();
    tte.getQueryInfo( colnms, values );
    isexec = update( colnms, values,
		     BugTextTableEntry::sKeyBugTextTable(), bugid );
    if ( !isexec )
	return false;

    return true;
}


//MantisDBMgr
SqlDB::MantisDBMgr::MantisDBMgr( const ConnectionData* cd, const char* usernm )
      :username_( BufferString(usernm) )
{
    if ( cd ) acc_.connectionData() = *cd;
    const bool isopen = acc_.open();
    if ( !isopen )
    {
	errmsg_ = "Unable to open database please check network connection";
       	errmsg_.add ( " or cosult database administrator" );
    }

    if ( username_.isEmpty() )
    {
	BufferString errmsg( "Undefined username" );
	errmsg_ = errmsg;
    }

    query_ = new MantisQuery( acc_ );
    query().isActive();
    bugtable_ = new BugTableEntry();
    bugtexttable_ = new BugTextTableEntry();
}


SqlDB::MantisDBMgr::~MantisDBMgr()
{
    acc_.close();
    if ( query_ )
	{ query().finish(); delete query_; }

    deepErase( bugs_ );
    deepErase( texttables_ );
    deepErase( versionsbyproject_ );
    delete bugtable_;
    delete bugtexttable_;
}


const char* SqlDB::MantisDBMgr::errMsg() const
{
    if ( !errmsg_.isEmpty() )
	return errmsg_.buf();

    errmsg_ = query_ ? query_->errMsg() : acc_.errMsg();
    return errmsg_.buf();
}


#define mErrRet(s) { errmsg_ = s; return false; }
#define mErrMsgRet(s) \
{ \
    BufferString msg("Unable to retrieve data from ", s, "\n" ); \
    msg.add( query_->errMsg() ); \
    mErrRet( msg.buf() ); \
}


bool SqlDB::MantisDBMgr::fillCategories()
{
    errmsg_.setEmpty();
    if ( !acc_.isOpen() )
	mErrRet("No database available")

    BufferString querystr ( "SELECT category FROM " );
    querystr.add( sKeyProjectCategoryTable() ).add( " WHERE project_id=" )
	    .add( cOpenDtectProjectID() );

    if ( !query().execute(querystr) )
	mErrMsgRet( sKeyProjectCategoryTable() );

    while ( query().next() )
	categories_.add( query().data(0) );

    return true;
}


bool SqlDB::MantisDBMgr::fillBugTableEntries()
{
    errmsg_.setEmpty();
    BufferString querystr( "SELECT " );
    querystr.add( BugTableEntry::sKeyBugTable() ).add( ".id," )
	    .add( BugTableEntry::sKeyBugTable() ).add( ".category," )
	    .add( BugTableEntry::sKeyBugTable() ).add( ".summary," )
	    .add( BugTableEntry::sKeyBugTable() ).add( ".severity," )
	    .add( BugTableEntry::sKeyBugTable() ).add( ".status," )
	    .add( BugTableEntry::sKeyBugTable() ).add( ".resolution," )
	    .add( BugTableEntry::sKeyBugTable() ).add( ".reporter_id," )
	    .add( BugTableEntry::sKeyBugTable() ).add( ".handler_id," )
	    .add( BugTableEntry::sKeyBugTable() ).add( ".platform," )
	    .add( BugTableEntry::sKeyBugTable() ).add( ".version," )
	    .add( BugTableEntry::sKeyBugTable() ).add( ".project_id," )
	    .add( BugTableEntry::sKeyBugTable() ).add( ".last_updated," )
	    .add( BugTableEntry::sKeyBugTable() ).add( ".fixed_in_version," )
	    .add( BugTextTableEntry::sKeyBugTextTable() ).add( ".description," )
	    .add( BugTextTableEntry::sKeyBugTextTable() )
	    .add( ".steps_to_reproduce," )
	    .add( BugTextTableEntry::sKeyBugTextTable() ).add( ".reporter" )
	    .add( " FROM " )
	    .add( BugTableEntry::sKeyBugTable() ).add( "," )
	    .add( BugTextTableEntry::sKeyBugTextTable() ).add( "," )
	    .add( sKeyProjectTable() )
	    .add( " WHERE ( ")
	    .add( sKeyProjectTable() ).add( ".id" ).add( " = " )
	    .add( BugTableEntry::sKeyBugTable() ).add( ".project_id" )
	    .add ( " AND " ).add( sKeyProjectTable() ).add( ".enabled = 1 " )
	    .add( " AND " ).add( BugTableEntry::sKeyBugTable() ).add( ".id=" )
	    .add( BugTableEntry::sKeyBugTable() )
	    .add( ".bug_text_id" ).add( " AND " )
	    .add( "(" ).add( BugTableEntry::sKeyBugTable() ).add( ".status=" )
	    .add( BugTableEntry::cStatusAssigned() ).add( " OR " )
	    .add( BugTableEntry::sKeyBugTable() )
	    .add( ".status=" ).add( BugTableEntry::cStatusNew() ).add( " OR " )
	    .add( BugTableEntry::sKeyBugTable() )
	    .add( ".status=" ).add( BugTableEntry::cStatusFeedback() )
	    .add( " OR " )
	    .add( "( " ).add( BugTableEntry::sKeyBugTable() )
	    .add( ".status=" ).add( BugTableEntry::cStatusClosed() )
	    .add( " AND " ).add( BugTableEntry::sKeyBugTable() )
	    .add( ".resolution=" ).add( BugTableEntry::cResolutionFixed() )
	    .add( ") " ).add( " OR " )
	    .add( "( " ).add( BugTableEntry::sKeyBugTable() )
	    .add( ".status=" ).add( BugTableEntry::cStatusResolved() )
	    .add( " AND " ).add( BugTableEntry::sKeyBugTable() )
	    .add( ".resolution=" ).add( BugTableEntry::cResolutionOpen() )
	    .add( ") " ).add( " OR " )
	    .add( "( " ).add( BugTableEntry::sKeyBugTable() )
	    .add( ".status=" ).add( BugTableEntry::cStatusResolved() )
	    .add( " AND " ).add( BugTableEntry::sKeyBugTable() )
	    .add( ".resolution=" ).add( BugTableEntry::cResolutionFixed() )
	    .add( "))  AND " ).add( BugTableEntry::sKeyBugTable() )
	    .add( ".id=" ).add( BugTextTableEntry::sKeyBugTextTable() )
	    .add( ".id ) ORDER BY " )
	    .add( BugTableEntry::sKeyBugTable() ).add( ".id ASC" );

    if ( !query().execute(querystr) )
	mErrMsgRet( BugTableEntry::sKeyBugTable() );

    while ( query().next() )
    {
	BugTableEntry* bugtable = new BugTableEntry();
	BugTextTableEntry* texttable = new BugTextTableEntry();

	int qidx = -1;
	bugtable->id_ = query().iValue( ++qidx );
	bugtable->category_ = query().data( ++qidx );
	bugtable->summary_ = query().data( ++qidx );
	bugtable->severity_ = query().iValue( ++qidx );
	bugtable->status_ = query().iValue( ++qidx );
	bugtable->resolution_ = query().iValue( ++qidx );
	bugtable->reporterid_ = query().iValue( ++qidx );
	bugtable->handlerid_ = query().iValue( ++qidx );
	bugtable->platform_ = query().data( ++qidx );
	bugtable->version_ = query().data( ++qidx );
	bugtable->projectid_ = query().iValue( ++qidx );
	bugtable->lastupddate_ = query().data( ++qidx );
	bugtable->fixedinversion_ = query().data( ++qidx );
	texttable->description_ = query().data( ++qidx );
	texttable->stepsreproduce_ = query().data( ++qidx );
	texttable->reporter_ = query().data( ++qidx );

	bugs_ += bugtable;
	texttables_ += texttable;
    }

    if ( bugs_.isEmpty() )
	mErrRet("No issues to fix");

    return true;
}


bool SqlDB::MantisDBMgr::fillUsersInfo()
{
    errmsg_.setEmpty();
    BufferString querystr( "SELECT " );
    querystr.add( sKeyUserTable() ).add( ".username," )
	    .add( sKeyProjectUserListTable() ).add( ".user_id" )
	    .add( " FROM " ).add( sKeyUserTable() ).add( "," )
	    .add( sKeyProjectUserListTable() ).add( " WHERE (" )
	    .add( sKeyProjectUserListTable() ).add( ".project_id=1 AND " )
	    .add( sKeyUserTable() ).add( ".id=" )
	    .add( sKeyProjectUserListTable() ).add( ".user_id AND " )
	    .add( sKeyUserTable() ).add( ".enabled=1 AND " )
	    .add( "( " ).add( sKeyProjectUserListTable() )
	    .add( ".access_level" ).add( ">" )
	    .add( cAccessLevelDeveloper() );
    BufferString querystrdvprs( querystr );
    querystrdvprs.add( ") ) ORDER BY " ).add( sKeyUserTable() )
		 .add( ".username ASC" );

    if ( !query().execute(querystrdvprs) )
	mErrMsgRet( sKeyUserTable() );

    developers_.erase();
    while ( query().next() )
	developers_.add( query().data(0) );

    BufferString querystrusers( querystr );
    querystrusers.add( " OR " ).add( sKeyProjectUserListTable() )
		 .add( ".access_level" ).add( "=" )
		 .add( cAccessLevelCaseStudy() )
	    	 .add( " )) ORDER BY " ).add( sKeyUserTable() )
		 .add( ".username ASC" );
    if ( !query().execute( querystrusers ) )
	mErrMsgRet( sKeyProjectUserListTable() );

    usernames_.erase();
    userids_.erase();
    while ( query().next() )
    {
	usernames_.add( query().data(0) );
	userids_.add( toInt(query().data(1).buf()) );
    }

    if ( username_.isEmpty() )
    {
	errmsg_ = "Unable to retrieve data from mantis_user_table";
	return false;
    }

    bool isexist = false;
    for ( int idx=0; idx<usernames_.size(); idx++ )
    {
	BufferString unm = usernames_.get( idx );
	if ( username_.isEqual( unm, true ) )
	{
	    isexist = true;
	    break;
	}
    }

    if ( !isexist )
    {
	developers_.erase();
	usernames_.erase();
	userids_.erase();

	BufferString msg( "User '",username_,"'" );
	msg.add( " is not existed in database. OR \n" )
	   .add( "He/She did not have permission " )
	   .add( "to work on mantis issues" );
	errmsg_ = msg;
	return false;
    }

    return true;
}


bool SqlDB::MantisDBMgr::fillProjectsInfo()
{
    errmsg_.setEmpty();
    BufferString querystr( "SELECT id,name FROM " );
    querystr.add( sKeyProjectTable() ).add( " WHERE enabled=1" );
    if ( !query().execute( querystr ) )
	mErrMsgRet( sKeyProjectTable() );

    projectids_.erase();
    projectnms_.erase();
    while ( query().next() )
    {
	projectids_.add( toInt(query().data(0).buf()) );
	projectnms_.add( query().data(1) );
    }

    return true;

}


void SqlDB::MantisDBMgr::getProjnm( int projid, BufferString& projnm ) const
{
    projnm.setEmpty();
    const int pidx = projectids_.indexOf( projid );
    if ( projid < 0 || !projectIDs().validIdx( pidx ) )
	return;

    if ( projectnms_.size() != projectids_.size() )
	return;

    projnm = projectnms_.get( pidx );
}


bool SqlDB::MantisDBMgr::fillVersionsByProject()
{
//    const char* vers = GetFullODVersion();
//    const float ver = toFloat( vers ) - 1;
    const float ver = 4.5 - 1;
    errmsg_.setEmpty();
    for ( int pidx=0; pidx<projectIDs().size(); pidx++ )
    {
	BufferStringSet* versbyproj = new BufferStringSet();
	BufferString querystr( "SELECT version FROM " );
	querystr.add( sKeyProjectVersionTable() ).add( " WHERE ( " )
		.add( "version > ").add( ver ).add(" AND released=1 )" )
		.add( " AND project_id=" ).add( projectIDs()[pidx] )
		.add( " ORDER BY version DESC " );
	if ( !query().execute( querystr ) )
	    mErrMsgRet( sKeyProjectVersionTable() );

	while ( query().next() )
	    versbyproj->add( query().data(0) );

	versionsbyproject_ += versbyproj;
    }

    return true;
}


bool SqlDB::MantisDBMgr::fillAttachedFilesInfo()
{
    attachids_.erase();
    attachfilenms_.erase();
    errmsg_.setEmpty();
    BufferString querystr( "SELECT " );
    querystr.add( sKeyBugFileTable() ).add ( ".bug_id, " )
	    .add( sKeyBugFileTable() ).add( ".filename " )
	    .add(" FROM " ).add( SqlDB::BugTableEntry::sKeyBugTable() )
	    .add( ", " ).add( sKeyBugFileTable() ).add( " WHERE ( (" )
	    .add( SqlDB::BugTableEntry::sKeyBugTable() )
	    .add( ".status=" ).add( SqlDB::BugTableEntry::cStatusNew() )
	    .add( " OR " )
	    .add( SqlDB::BugTableEntry::sKeyBugTable() )
	    .add( ".status=" ).add( SqlDB::BugTableEntry::cStatusAssigned() )
	    .add( " OR " )
	    .add( SqlDB::BugTableEntry::sKeyBugTable() )
	    .add( ".status=" ).add( SqlDB::BugTableEntry::cStatusFeedback() )
	    .add( " OR " )
	    .add( SqlDB::BugTableEntry::sKeyBugTable() )
	    .add( ".status=" ).add( SqlDB::BugTableEntry::cStatusResolved() )
	    .add( " ) AND ( " )
	    .add( SqlDB::BugTableEntry::sKeyBugTable() )
	    .add( ".resolution=" ).add(SqlDB::BugTableEntry::cResolutionOpen())
	    .add( " OR " )
	    .add( SqlDB::BugTableEntry::sKeyBugTable() )
	    .add( ".resolution=" )
	    .add( SqlDB::BugTableEntry::cResolutionFixed() ).add( " )" )
	    .add( " AND " ).add( SqlDB::BugTableEntry::sKeyBugTable() )
	    .add( ".id" ).add( "=" )
	    .add( sKeyBugFileTable() ).add( ".bug_id )" )
	    .add( " ORDER BY " )
	    .add( SqlDB::BugTableEntry::sKeyBugTable() ).add( ".id" );

	if ( !query().execute( querystr ) )
	    mErrMsgRet( sKeyBugFileTable() );

	while ( query().next() )
	{
	    attachids_.add( toInt(query().data(0).buf()) );
	    attachfilenms_.add( query().data(1) );
	}
	
	return true;
}


void SqlDB::MantisDBMgr::getAllVersions( BufferStringSet& versions ) const
{
    versions.erase();
    if ( !versionsbyproject_.size() )
	return;

    for ( int vidx=0; vidx<versionsbyproject_.size(); vidx++ )
    {
	const BufferStringSet* vers = versionsbyproject_[vidx];
	if ( !vers )
	    continue;

	for ( int idx=0; idx<vers->size(); idx++ )
	    versions.addIfNew( vers->get( idx ) );
    }
}


const BufferStringSet* SqlDB::MantisDBMgr::getVersions(const char* projnm) const
{
    const BufferStringSet& projectnms = projects();
    if ( projectnms.size() != versionsbyproject_.size() )
	return 0;

    const int pidx = projectnms.indexOf( projnm );
    if ( !versionsbyproject_.validIdx(pidx) )
	return 0;

    return versionsbyproject_[pidx];
}


const BufferStringSet* SqlDB::MantisDBMgr::getVersions( int projid ) const
{
    const TypeSet<int>& pids = projectIDs();
    const int pidx = pids.indexOf( projid );
    if ( !projects().validIdx(pidx) )
	return 0;

    BufferString projnm( projects().get(pidx) );
    return getVersions( projnm );
}


void SqlDB::MantisDBMgr::fillSeverity()
{
    sevirities_.erase();
    severityvals_.erase();
    sevirities_.add( "Feature" ).add( "Trivial" ).add( "Text" ).add( "Tweak" )
	       .add( "Minor" ).add( "Major" ).add( "Crash" ).add( "Block" );
    severityvals_.add( SqlDB::BugTableEntry::cSeverityFeature() );
    severityvals_.add( SqlDB::BugTableEntry::cSeverityTrivial() );
    severityvals_.add( SqlDB::BugTableEntry::cSeverityText() );
    severityvals_.add( SqlDB::BugTableEntry::cSeverityTweak() );
    severityvals_.add( SqlDB::BugTableEntry::cSeverityMinor() );
    severityvals_.add( SqlDB::BugTableEntry::cSeverityMajor() );
    severityvals_.add( SqlDB::BugTableEntry::cSeverityCrash() );
    severityvals_.add( SqlDB::BugTableEntry::cSeverityBlock() );
}


bool SqlDB::MantisDBMgr::getInfoFromTables()
{
    if ( !fillUsersInfo() || !fillCategories() || !fillProjectsInfo() ||
	 !fillBugTableEntries() || !fillVersionsByProject() ||
	 !fillAttachedFilesInfo() )
	return false;

    fillSeverity();
    return true;
}


bool SqlDB::MantisDBMgr::fillBugsIdx( const char* projectnm, const char* usernm,
				      TypeSet<int>& bugstofix )
{
    bugsindex_.erase();
    if ( !usernm ) return false;

    const int usridx = userNames().indexOf( usernm );
    const bool isall = caseInsensitiveEqual( usernm, sKeyAll() );
    if ( !userIDs().validIdx( usridx ) && !isall )
    {
	UsrMsg( BufferString("User ",usernm," does not exist in Mantis") );
	return false;
    }

    const bool isallprojs = caseInsensitiveEqual( projectnm, sKeyAll() );
    const int projidx = projects().indexOf( projectnm );
    if ( !isallprojs && projidx < 0 )
    {
	UsrMsg( BufferString( "Project ", projectnm,
		    	      " does not exist in Mantis") );
	return false;
    }

    if ( !isallprojs && !projectIDs().validIdx(projidx ) )
	return false;

    const int projid = isallprojs ? -1 : projectIDs()[projidx];
    const int usrid = isall ? -1 : userIDs()[usridx];
    const int nrbugs = nrBugs();
    for ( int idx=0; idx<nrbugs; idx++ )
    {
	BugTableEntry* bugtable = getBugTableEntry( idx );
	if ( !bugtable )
	    continue;

	const bool isprojequal = projid < 0 ? true
	    				    : bugtable->projectid_ == projid;
	if ( isprojequal && usrid < 0 )
	    bugsindex_.add( idx );
	else if ( isprojequal && usrid == bugtable->handlerid_ )
	    bugsindex_.add( idx );
    }

    bugstofix = bugsindex_;
    return bugstofix.isEmpty() ? false : true;
}


void SqlDB::MantisDBMgr::addBugTableEntryToSet( BugTableEntry& table )
{
    bugs_ += &table;
}


void SqlDB::MantisDBMgr::addBugTextTableEntryToSet( BugTextTableEntry& tt )
{
    texttables_ += &tt;
}


void SqlDB::MantisDBMgr::removeBugTableEntryFromSet( int tableidx )
{ 
    bugs_.remove( tableidx, true );
}


void SqlDB::MantisDBMgr::removeBugTextTableEntryFromSet( int tableidx )
{
    texttables_.remove( tableidx, true );
}


const SqlDB::BugTableEntry* SqlDB::MantisDBMgr::getBugTableForRead( int idx ) const
{
    if ( idx == -1 ) return bugtable_;
    return bugs_.validIdx( idx ) ? bugs_[idx] : 0;
}


SqlDB::BugTableEntry* SqlDB::MantisDBMgr::getBugTableEntry( int idx )
{
    if ( idx == -1 ) return bugtable_;
    return bugs_.validIdx( idx ) ? bugs_[idx] : 0;
}


SqlDB::BugTextTableEntry* SqlDB::MantisDBMgr::getBugTextTableEntry( int idx )
{ 
    if ( idx == -1 ) return bugtexttable_;
    return texttables_.validIdx( idx ) ? texttables_[idx] : 0;
}


int SqlDB::MantisDBMgr::getBugTableIdx( int bugid )
{
    BugTableEntry* table = 0;
    for( int idx=0; idx<nrBugs(); idx++ )
    {
	table = getBugTableEntry( idx );
	if ( !table )
	    continue;

	if ( table->id_ == bugid )
	    return idx;
    }

    return -1;
}


TypeSet<int>& SqlDB::MantisDBMgr::getBugsIndex()
{ return bugsindex_; }


bool SqlDB::MantisDBMgr::updateBugTableEntryHistory( int bidx, bool isadded,
						     bool isnoteempty )
{
    errmsg_.setEmpty();
    BugTableEntry* bugtable = 0;
    bugtable = bidx < 0 ? bugtable_ : getBugTableEntry( bidx );
    if ( !bugtable )
    {
	errmsg_ = "Bug not existed";
	return false;
    }

    ObjectSet<BugHistoryTableEntry>& history = bugtable->getHistory();
    if ( isadded )
    {
	BugHistoryTableEntry* historynewentry = new BugHistoryTableEntry;
	historynewentry->type_ = 1;
	history.insertAt( historynewentry, 0 );
    }

    if ( !isnoteempty )
    {
	BugHistoryTableEntry* notehistory = new BugHistoryTableEntry;
	notehistory->bugid_ = bugtable->id_;
	notehistory->type_ = 2;
	const int maxnote = getMaxNoteIDFromBugNoteTable();
	notehistory->oldvalue_ = maxnote;
	const int insert = history.size()==0 ? 0 : 1;
	history.insertAt( notehistory, insert );
    }

    if ( !history.size() )
	return true;

    const int userid = getUserID();
    if ( userid < 0 )
    {
	errmsg_ = "User not existed";
	return false;
    }

    BufferString date = bugtable->lastupddate_;
    for ( int ihist=0; ihist<history.size(); ihist++ )
    {
	if ( isadded )
	    history[ihist]->bugid_ = bugtable->id_;

	history[ihist]->userid_ = userid;
	history[ihist]->date_ = date;
    }

    const bool isok = updateBugHistoryTable( history, isadded );
    bugtable->deleteHistory();
    return isok;
}


void SqlDB::MantisDBMgr::updateBugTextTableEntryHistory( int bidx )
{
    if ( bidx < 0 )
	return;

    BugTableEntry* bugtable = 0;
    BugTextTableEntry* texttable = 0;
    bugtable = getBugTableEntry( bidx );
    texttable = getBugTextTableEntry( bidx );
    if ( !texttable || !bugtable )
	return;

    ObjectSet<BugHistoryTableEntry>& history = texttable->getHistory();
    for ( int idx=0; idx<history.size(); idx++ )
    {
	BugHistoryTableEntry* bhte = history[idx];
	if ( !bhte )
	    continue;

	bhte->bugid_ = bugtable->id_;
	const int usrid = getUserID();
	if ( usrid < 0 )
	    return;

	bhte->userid_ = usrid;
	bhte->date_ = bugtable->lastupddate_;
    }

    updateBugHistoryTable( history, false );
    texttable->deleteHistory();
}


bool SqlDB::MantisDBMgr::updateBugHistoryTable(
	ObjectSet<BugHistoryTableEntry>& bhte, bool isadded )
{
    errmsg_.setEmpty();
    BufferStringSet colnms, values;
    for ( int htidx=0; htidx<bhte.size(); htidx++ )
    {
	BugHistoryTableEntry* entry = bhte[htidx];
	BufferString fldnm;
	if ( isadded )
	{
	    fldnm = entry->fieldnm_;
	    if ( fldnm=="severity" || fldnm=="platform" || fldnm=="version" )
		continue;
	}

	entry->getQueryInfo( colnms, values );
	if ( !query().insert( colnms, values,
		    	      BugHistoryTableEntry::sKeyBugHistoryTable() ) )
	{
	    errmsg_ = query().errMsg();
	    colnms.erase(); values.erase();
	    return false;
	}

	colnms.erase(); values.erase();
    }

    return true;
}


void SqlDB::MantisDBMgr::eraseCurrentEntries()
{
    bugtable_ = new BugTableEntry();
    bugtexttable_ = new BugTextTableEntry();
}


bool SqlDB::MantisDBMgr::addToBugTextTable( BugTextTableEntry& bugtt )
{
    errmsg_.setEmpty();
    BufferStringSet colnms, values;
    bugtt.getQueryInfo( colnms, values );
    return query().insert(colnms,values,BugTextTableEntry::sKeyBugTextTable());
}


bool SqlDB::MantisDBMgr::addToBugTable( BugTableEntry& bugtable )
{
    errmsg_.setEmpty();
    BufferStringSet colnms, values;
    bugtable.getQueryInfo( colnms, values, false );
    if ( !query().insert(colnms, values, BugTableEntry::sKeyBugTable()) )
	return false;

//Update bug_text_id
    const int btid = getMaxBugIDFromBugTextTable();
    const int bid = getMaxBugIDFromBugTable();
    if ( btid < 0 || bid < 0 )
	return false;

    BufferString querystr = "UPDATE ";
    querystr.add( BugTableEntry::sKeyBugTable() )
	    .add( " SET bug_text_id=").add( btid )
	    .add( " WHERE id=").add( bid );
    return query().execute( querystr );
}


bool SqlDB::MantisDBMgr::addToBugNoteTextTable( const char* note )
{
    errmsg_.setEmpty();
    BufferStringSet colmns,values;
    colmns.add( "id" ).add( "note" );
    values.add( "" ).add( note );
    return query().insert( colmns, values, sKeyBugNoteTextTable() );
}


bool SqlDB::MantisDBMgr::addToBugNoteTable( const char* note, int bugid )
{
    errmsg_.setEmpty();
    if ( !addToBugNoteTextTable( note ) )
	mErrRet( "Problem while updating bug note text table");

    int reporterid = getUserID();
    if ( reporterid < 0 )
	return false;

    BufferString datetime = query().getCurrentDateTime();
    BufferStringSet colnms,values;
    colnms.add( "id" ).add( "bug_id" ).add( "reporter_id" )
	  .add( "date_submitted" ).add( "last_modified" );
    values.add( "" ).add( toString( bugid ) );
    values.add( toString(reporterid ) ).add( datetime )
	  .add( datetime );
    if ( !query().insert( colnms, values, sKeyBugNoteTable() ) )
	mErrRet( "Problem while updating bug note table");

//Update bugnote_text_id
    const int maxntid = getMaxNoteIDFromBugNoteTextTable();
    const int maxnid = getMaxNoteIDFromBugNoteTable();
    BufferString querystr( "UPDATE " );
    querystr.add( sKeyBugNoteTable() )
	    .add( " SET bugnote_text_id=" ).add( maxntid )
	    .add (" WHERE id=" ).add( maxnid );
    bool isupd = query().execute( querystr );
    if ( !isupd )
	mErrRet( "Problem while updating bug note textid table");

    return true;

}


bool SqlDB::MantisDBMgr::addBug( BugTableEntry& bugtable,
				 BugTextTableEntry& tt, const char* note )
{
    if ( !addToBugTextTable( tt ) )
	return false;

    if ( !addToBugTable( bugtable ) )
	return false;

    bugtable.id_ = getMaxBugIDFromBugTable();

    if ( !*note )
	return true;

    return addToBugNoteTable( note, bugtable.id_ );
}


bool SqlDB::MantisDBMgr::editBug( BugTableEntry& bugtable,
				  BugTextTableEntry& tte, const char* note )
{
    const bool isedit = query().updateTables( bugtable, tte );
    if ( !isedit )
	return false;

    if ( !note || !*note )
	return true;

    return addToBugNoteTable( note, bugtable.id_ );
}


bool SqlDB::MantisDBMgr::deleteBug( int bugid )
{
    return deleteBugHistory( bugid ) && deleteBugNotesInfo( bugid ) &&
	   deleteBugTableInfo( bugid );
}


bool SqlDB::MantisDBMgr::deleteBugHistory( int id )
{
    const char* tblnm = SqlDB::BugHistoryTableEntry::sKeyBugHistoryTable();
    const bool isok = query().deleteInfo( tblnm, "bug_id", id );

    if ( !isok )
	mErrRet( "Unable to delete history");

    return true;
}


bool SqlDB::MantisDBMgr::getNotesInfo( int bugid, TypeSet<int>& noteids,
				       BufferStringSet& notes )
{
    const char* bntt = sKeyBugNoteTextTable();
    const char* bnt = sKeyBugNoteTable();
    BufferString qstr( "SELECT " );
    qstr.add( bnt ).add( ".id, " ).add( bntt ).add( ".note" )
	.add( " FROM " ).add( bnt ).add( "," ).add( bntt )
	.add( " WHERE " ).add( bnt ).add( ".bugnote_text_id=" )
	.add( bntt ).add( ".id" ).add( " AND " )
	.add( bnt ).add( ".bug_id=" ).add( bugid );
    const bool isok = query().execute( qstr );
    if ( !isok )
	mErrMsgRet( sKeyBugNoteTable() );

    while ( query().next() )
    {
	noteids.add( query().iValue(0) );
	notes.add( query().data(1) );
    }

    if ( noteids.size() != notes.size() )
	return false;

    if ( !noteids.size() )
    {
	BufferString msg( "There are no notes attached to selected Bug:",
			  bugid );
	errmsg_ = msg;
    }

    return true;
}


int SqlDB::MantisDBMgr::getBugNoteTextID( int noteid )
{
    const char* bnt = sKeyBugNoteTable();
    BufferString qstr( "SELECT bugnote_text_id FROM " );
    qstr.add( bnt ).add( " WHERE id=" ).add( noteid );
    int notetextid = -1;
    const bool isok = query().execute( qstr );
    if ( !isok )
	return notetextid;

    while ( query().next() )
	notetextid = query().iValue( 0 );

    return notetextid;
}


bool SqlDB::MantisDBMgr::deleteSelBugNote( int noteid )
{
    int notetextid = getBugNoteTextID( noteid );
    const char* bntt = sKeyBugNoteTextTable();
    const char* bnt = sKeyBugNoteTable();
    if ( notetextid < 0 )
	return false;

    bool isntiddel = query().deleteInfo( bnt, "id", noteid );
    bool isnttiddel = query().deleteInfo( bntt, "id", notetextid );
    return isnttiddel && isntiddel;
}



bool SqlDB::MantisDBMgr::updateBugNote( int noteid, const BufferString& note )
{
    int notetextid = getBugNoteTextID( noteid );
    if ( notetextid < 0 )
	return false;

    BufferStringSet cols,vals;
    cols.add( "note" );
    vals.add( note );
    const char* bntt = sKeyBugNoteTextTable();
    bool isupd = query().update( cols, vals, bntt, notetextid );
    return isupd;
}


bool SqlDB::MantisDBMgr::deleteBugNotesInfo( int bugid )
{
    const char* bntt = sKeyBugNoteTextTable();
    const char* bnt = sKeyBugNoteTable();
    BufferString qstr( "SELECT id,bugnote_text_id FROM " );
    qstr.add( bnt ).add ( " WHERE " ).add( " bug_id=" ).add( bugid );
    bool isok = false;
    isok = query().execute( qstr );
    if ( !isok )
	mErrRet( "Unable to get bug note ids" );

    TypeSet<int> notesid;
    TypeSet<int> notetextids;
    while ( query().next() )
    {
	notesid.add( query().iValue( 0 ) );
	notetextids.add( query().iValue( 1 ) );
    }

    if ( notesid.isEmpty() )
	return true;

    if ( notetextids.size() != notesid.size() )
	return false;

    for ( int nidx=0; nidx<notesid.size(); nidx++ )
    {
	if ( !notetextids[nidx] )
	    continue;

	bool isnttdel = query().deleteInfo( bntt, "id", notetextids[nidx] );
	if ( !isnttdel )
	{
	    BufferString msg( "Unable to delete notes from" );
	    msg.add( sKeyBugNoteTextTable() );
	    mErrRet( msg.buf() );
	}
    }

    isok = query().deleteInfo( bnt, "bug_id", bugid );
    if ( !isok )
	mErrRet( "Unable to delete note info from BugNoteTable" );

    return true;
}


bool SqlDB::MantisDBMgr::deleteBugTableInfo( int id )
{
    const char* btt = SqlDB::BugTextTableEntry::sKeyBugTextTable();
    const char* bt = SqlDB::BugTableEntry::sKeyBugTable();

    BufferString qstr( "SELECT bug_text_id FROM " );
    qstr.add( bt ).add ( " WHERE " ).add( " id=" ).add( id );
    bool isexec = query().execute( qstr );
    if ( !isexec )
	mErrRet( "Unable to delete bug" );

    TypeSet<int> textids;
    while ( query().next() )
	textids.add( query().iValue( 0 ) );

    if ( textids.isEmpty() )
	mErrRet( "Unable to delete bug" );

    if ( !textids.validIdx(0) && textids.size() > 1 )
	mErrRet( "Unable to delete bug" );

    bool isbtisdel = query().deleteInfo( btt, "id", textids[0] );
    if ( !isbtisdel )
	mErrRet( "Unable to delete bug info from BugTextTable" );

    bool isbdel = query().deleteInfo( bt, "id", id );
    if ( !isbdel )
	mErrRet( "Unable to delete bug info from BugTable" );

    return true;
}


int SqlDB::MantisDBMgr::getUserID() const
{
    if ( username_.isEmpty() )
	return -1;

    if ( !usernames_.isPresent( username_ ) )
    {
	UsrMsg( BufferString("User ",username_," does not exist in Mantis") );
	return -1;
    }

    const TypeSet<int>& userids = userIDs();
    const int uid = usernames_.indexOf( username_ );
    if ( !userids.validIdx( uid ) )
	return -1;

    return userids[uid];
}


int SqlDB::MantisDBMgr::getUserID( bool isdeveloper ) const
{
    if ( username_.isEmpty() )
	return -1;

    const bool ispresent = developers().isPresent( username_ );
    if ( isdeveloper && !ispresent )
    {
	BufferString msg( "'", username_ );
	msg.add ( "' does not existed in mantis developers list" );
	UsrMsg( msg );
	return -1;
    }

    if ( !usernames_.isPresent(username_) )
    {
	UsrMsg( BufferString("User ",username_," does not exist in Mantis") );
	return -1;
    }

    const TypeSet<int>& userids = userIDs();
    const int uid = usernames_.indexOf( username_ );
    if ( !userids.validIdx( uid ) )
	return -1;

    return userids[uid];
}


int SqlDB::MantisDBMgr::getMaxBugIDFromBugTable() const
{
    errmsg_.setEmpty();
    BufferString querystr( "SELECT  MAX(id) FROM " );
    querystr.add( BugTableEntry::sKeyBugTable() );
    if ( !query_->execute(querystr) )
	return -1;

    while ( query().next() )
	querystr = query().data(0);

    return toInt( querystr );
}


int SqlDB::MantisDBMgr::getMaxBugIDFromBugTextTable() const
{
    errmsg_.setEmpty();
    BufferString querystr( "SELECT  MAX(id) FROM " );
    querystr.add( BugTextTableEntry::sKeyBugTextTable() );
    if ( !query_->execute(querystr) )
	return -1;

    while ( query().next() )
	querystr = query().data(0);

    return toInt( querystr );
}


int SqlDB::MantisDBMgr::getMaxNoteIDFromBugNoteTextTable() const
{
    errmsg_.setEmpty();
    BufferString querystr( "SELECT  MAX(id) FROM " );
    querystr.add( sKeyBugNoteTextTable() );
    if ( !query_->execute( querystr ) )
	return -1;

    while ( query().next() )
	querystr = query().data(0);

    return toInt( querystr );
}


int SqlDB::MantisDBMgr::getMaxNoteIDFromBugNoteTable() const
{
    errmsg_.setEmpty();
    BufferString querystr( "SELECT  MAX(id) FROM " );
    querystr.add( sKeyBugNoteTable() );
    if ( !query_->execute( querystr ) )
	return -1;

    while ( query().next() )
	querystr = query().data(0);

    return toInt( querystr );
}


void SqlDB::MantisDBMgr::prepareForQuery( BufferString& str )
{
    if ( str.isEmpty() ) return;
    const int nrsrepl = countCharacter( str, '\'' );
    const int nrdrepl = countCharacter( str, '"' );
    if ( nrsrepl + nrdrepl < 1 ) return;

    str.setBufSize( str.size() + nrsrepl + nrdrepl + 1 );
    if ( nrsrepl > 0 )
	replaceString( str.buf(), "'", "''" ); // ANSI standard
    if ( nrdrepl > 0 )
	replaceString( str.buf(), "\"", "\\\"" ); // MySql specific
}


void SqlDB::MantisDBMgr::parseVersion( const BufferString& fullver,
				       BufferString& numver,
				       BufferString& patchver )
{
    const char* buf = fullver.buf();
    char* numbuf = numver.buf();
    char* patchbuf = patchver.buf();
    bool ispatch = false;
    while ( *buf )
    {
	char c = *buf;
	if ( !ispatch && c != '.' && ( c < '0' || c > '9' ) )
	    ispatch = true;

	if ( ispatch )
	    *patchbuf++ = c;
	else
	    *numbuf++ = c;

	buf++;
    }

    *patchbuf = '\0';
    *numbuf = '\0';
}


void SqlDB::MantisDBMgr::editVersions( const BufferStringSet& versions,
				       BufferStringSet& editedvers,
				       bool ismajor, bool isalladd )
{
    if ( versions.isEmpty() ) return;

    editedvers.erase();
    if ( isalladd )
	editedvers.add( sKeyAll() );

    for ( int idx=0; idx<versions.size(); idx++ )
    {
	BufferString str( versions.get( idx ) );
	char* ver = str.buf();
	if ( !ver || !*ver ) continue;

	int found = 0;
	while ( *ver )
	{
	    if ( '.' == *ver )
	    {
		found++;
		if ( found > 1 && ismajor )
			*ver = '\0';
	    }

	    if ( *ver != '.' && found > 1 && (*ver < '0' || *ver > '9') )
		    *ver = '\0';

	    ver++;
	}

	if ( found > 1 )
	    editedvers.addIfNew( str.buf() );
    }
}
