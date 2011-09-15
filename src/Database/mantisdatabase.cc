/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Nageswara
 Date:          Feb 2010
 RCS:           $Id: mantisdatabase.cc,v 1.16 2011-09-15 06:39:17 cvsnageswara Exp $
________________________________________________________________________

-*/

#include "mantisdatabase.h"

#include "bufstringset.h"
#include "convert.h"
#include "envvars.h"
#include "errh.h"
#include "mantistables.h"
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
SqlDB::MantisDBMgr::MantisDBMgr( const ConnectionData* cd )
{
    if ( cd ) acc_.connectionData() = *cd;
    acc_.open();
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
    if ( !errmsg_.isEmpty() ) return errmsg_.buf();
    if ( query_ ) return query_->errMsg();
    return acc_.errMsg();
}


#define mErrRet(s) { errmsg_ = s; return false; }

bool SqlDB::MantisDBMgr::fillCategories()
{
    errmsg_.setEmpty();
    if ( !acc_.isOpen() )
	mErrRet("No database available")

    BufferString querystr ( "SELECT category FROM " );
    querystr.add( sKeyProjectCategoryTable() ).add( " WHERE project_id=" )
	    .add( cOpenDtectProjectID() );
    if ( !query().execute( querystr ) )
	return false;

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
	    .add( BugTableEntry::sKeyBugTable() ).add( ".reporter_id," )
	    .add( BugTableEntry::sKeyBugTable() ).add( ".handler_id," )
	    .add( BugTableEntry::sKeyBugTable() ).add( ".platform," )
	    .add( BugTableEntry::sKeyBugTable() ).add( ".version," )
	    .add( BugTableEntry::sKeyBugTable() ).add( ".project_id," )
	    .add( BugTextTableEntry::sKeyBugTextTable() ).add( ".description" )
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
	    .add( ".status=" ).add( BugTableEntry::cStatusNew() ).add( ") " )
	    .add( " AND " ).add( BugTableEntry::sKeyBugTable() ).add( ".id=" )
	    .add( BugTextTableEntry::sKeyBugTextTable() )
	    .add( ".id ) ORDER BY " )
	    .add( BugTableEntry::sKeyBugTable() ).add( ".id ASC" );

    if ( !query().execute(querystr) )
	return false;

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
	bugtable->reporterid_ = query().iValue( ++qidx );
	bugtable->handlerid_ = query().iValue( ++qidx );
	bugtable->platform_ = query().data( ++qidx );
	bugtable->version_ = query().data( ++qidx );
	bugtable->projectid_ = query().iValue( ++qidx );
	texttable->description_ = query().data( ++qidx );

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
	return false;

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
	return false;

    usernames_.erase();
    userids_.erase();
    while ( query().next() )
    {
	usernames_.add( query().data(0) );
	userids_.add( toInt(query().data(1).buf()) );
    }

    return true;
}


bool SqlDB::MantisDBMgr::fillProjectsInfo()
{
    errmsg_.setEmpty();
    BufferString querystr( "SELECT id,name FROM " );
    querystr.add( sKeyProjectTable() ).add( " WHERE enabled=1" );
    if ( !query().execute( querystr ) )
	return false;
    projectids_.erase();
    projectnms_.erase();
    while ( query().next() )
    {
	projectids_.add( toInt(query().data(0).buf()) );
	projectnms_.add( query().data(1) );
    }

    return true;

}


bool SqlDB::MantisDBMgr::fillVersions()
{
    const char* vers = GetFullODVersion();
    const float ver = toFloat( vers )-1;
    errmsg_.setEmpty();
    BufferString querystr( "SELECT version FROM " );
    querystr.add( sKeyProjectVersionTable() ).add( " WHERE ( " )
	    .add( "version > ").add( ver ).add(" AND released=1 )" )
    	    .add( " ORDER BY version DESC " );
    if ( !query().execute( querystr ) )
	return false;

    while ( query().next() )
	versions_.add( query().data(0) );

    return true;
}


bool SqlDB::MantisDBMgr::fillVersionsByProject()
{
    const char* vers = GetFullODVersion();
    const float ver = toFloat( vers ) - 1;
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
	    return false;

	while ( query().next() )
	    versbyproj->add( query().data(0) );

	versionsbyproject_ += versbyproj;
    }

    return true;
}


const BufferStringSet* SqlDB::MantisDBMgr::getVersions(const char* projnm) const
{
    const BufferStringSet& projectnms = projects();
    if ( projectnms.size() != versionsbyproject_.size() )
	return 0;

    const int pidx = projectnms.indexOf( projnm );
    if ( ! pidx < 0 || !versionsbyproject_.validIdx( pidx ) )
	return 0;

    return versionsbyproject_[pidx];
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
    if ( !fillCategories() || !fillProjectsInfo() || !fillBugTableEntries()
	 || !fillUsersInfo() || !fillVersions() )
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
    const bool isunassign = caseInsensitiveEqual( usernm, sKeyUnAssigned() );
    if ( !isunassign && !userIDs().validIdx( usridx ) && !isall )
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
    const int usrid = isall || isunassign ? -1 : userIDs()[usridx];
    const int nrbugs = nrBugs();
    for ( int idx=0; idx<nrbugs; idx++ )
    {
	BugTableEntry* bugtable = getBugTableEntry( idx );
	if ( !bugtable )
	    continue;

	if ( isunassign )
	{
	    bool isstatusequal =
	    		bugtable->status_ == SqlDB::BugTableEntry::cStatusNew();
	    if ( !isstatusequal ) continue;
	}

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


void SqlDB::MantisDBMgr::updateBugTableEntryHistory( int bidx, bool isadded,
						     bool isnoteempty )
{
    BugTableEntry* bugtable = 0;
    bugtable = bidx < 0 ? bugtable_ : getBugTableEntry( bidx );
    if ( !bugtable )
	return;

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
	notehistory->type_ = 2;
	const int maxnote = getMaxNoteIDFromBugNoteTable();
	notehistory->oldvalue_ = maxnote;
	const int insert = history.size()==0 ? 0 : 1;
	history.insertAt( notehistory, insert );
    }

    if ( history.size() == 0 )
	return;

    const int userid = getUserID( false );
    if ( userid < 0 )
	return;

    BufferString date = bugtable->date_;
    for ( int ihist=0; ihist<history.size(); ihist++ )
    {
	if ( isadded )
	    history[ihist]->bugid_ = bugtable->id_;

	history[ihist]->userid_ = userid;
	history[ihist]->date_ = date;
    }

    updateBugHistoryTable( history, isadded );
    bugtable->deleteHistory();
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

    BugHistoryTableEntry* texttabhistory = texttable->getHistory();
    if ( !texttabhistory )
	return;

    texttabhistory->bugid_ = bugtable->id_;
    const int usrid = getUserID( false );
    if ( usrid < 0 )
	return;

    texttabhistory->userid_ = usrid;
    texttabhistory->date_ = bugtable->date_;

    ObjectSet<BugHistoryTableEntry> history;
    history += texttabhistory;
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
	if ( !query().insert(colnms, values,
		    		BugHistoryTableEntry::sKeyBugHistoryTable()) )
	    return false;

	colnms.erase(); values.erase();
    }

    return true;
}


void SqlDB::MantisDBMgr::eraseCurrentEntries( bool isfix )
{
    if ( !isfix )
    {
	bugtable_ = new BugTableEntry();
	bugtexttable_ = new BugTextTableEntry();
	return;
    }

    bugtable_->init();
    bugtexttable_->init();
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
    BufferString querystr = "UPDATE ";
    querystr.add( BugTableEntry::sKeyBugTable() )
	    .add( " SET bug_text_id=id WHERE id= (SELECT MAX(id) FROM " )
	    .add( BugTextTableEntry::sKeyBugTextTable() ).add( ")" );
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
	return false;

    int reporterid = getUserID( false );
    if ( reporterid < 0 )
	return false;

    BufferString datetime = query().getCurrentDateTime();
    BufferStringSet colnms,values;
    colnms.add( "id" ).add( "bug_id" ).add( "reporter_id" )
	  .add( "date_submitted" ).add( "last_modified" );
    values.add( "" ).add( toString(bugid ) );
    values.add( toString(reporterid ) ).add( datetime )
	  .add( datetime );
    if ( !query().insert( colnms, values, sKeyBugNoteTable() ) )
	return false;

//Update bugnote_text_id
    BufferString querystr( "UPDATE " );
    querystr.add( sKeyBugNoteTable() )
	    .add( " SET bugnote_text_id=id WHERE id= (SELECT MAX(id) FROM " )
	    .add( sKeyBugNoteTextTable() ).add( ")" );
    return query().execute( querystr );
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

    if ( !*note )
	return true;

    return addToBugNoteTable( note, bugtable.id_ );
}


int SqlDB::MantisDBMgr::getUserID( bool isdeveloper ) const
{
    const char* username = 0;
    username = GetEnvVar( "USER" );
    if ( !username || !*username )
	return -1;

    const bool ispresent = developers().isPresent( username );
    if ( isdeveloper && !ispresent )
    {
	BufferString msg( "'", username );
	msg.add ( "' does not existed in mantis developers list" );
	UsrMsg( msg );
	return -1;
    }

    if ( !usernames_.isPresent(username) )
    {
	UsrMsg( BufferString("User ",username," does not exist in Mantis") );
	return -1;
    }

    const TypeSet<int>& userids = userIDs();
    const int uid = usernames_.indexOf( username );
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


void SqlDB::MantisDBMgr::getMajorVersions( BufferStringSet& majorvers )
{
    majorvers.erase();
    majorvers.add( sKeyAll() );
    const BufferStringSet& vers = versions();
    if ( vers.isEmpty() ) return;

    for ( int idx=0; idx<vers.size(); idx++ )
    {
	BufferString str( vers.get( idx ) );
	char* ver = str.buf();
	if ( !ver || !*ver ) continue;

	int found = 0;
	while ( *ver )
	{
	    if ( '.' == *ver )
	    {
		found++;
		if ( found > 1 )
		    *ver = '\0';
	    }

	    ver++;
	}

	if ( found > 1 )
	    majorvers.addIfNew( str.buf() );
    }
}
