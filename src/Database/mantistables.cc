/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Nageswara
 Date:          April 2010
 RCS:           $Id$
________________________________________________________________________

-*/

#include "mantistables.h"

#include "bufstringset.h"
#include "odplatform.h"
#include "mantisdatabase.h"
#include "string2.h"

//BugTextTableEntry
const char* SqlDB::BugTextTableEntry::sKeyBugTextTable()
{ return "mantis_bug_text_table"; }


SqlDB::BugTextTableEntry::BugTextTableEntry()
{
    init();
}


void SqlDB::BugTextTableEntry::deleteHistory()
{
    deepErase( btthistoryset_ );
}


void SqlDB::BugTextTableEntry::getQueryInfo( BufferStringSet& colnms,
					     BufferStringSet& values )
{
    colnms.add( "description" ).add( "steps_to_reproduce" ).add( "reporter" );
    values.add( description_ ).add( stepsreproduce_ ).add( reporter_ );
}


void SqlDB::BugTextTableEntry::init()
{
    description_ = "";
    stepsreproduce_ = "";
    reporter_ = "";
}


void SqlDB::BugTextTableEntry::setDescription( BufferString& desc )
{
    removeTrailingBlanks( desc.buf() );
    if ( description_.isEqual( desc, true ) )
	return;

    addToHistory( "description" );
    description_ = desc;
}


void SqlDB::BugTextTableEntry::setStepsReproduce( BufferString& reproduce )
{
    removeTrailingBlanks( reproduce.buf() );
    if ( stepsreproduce_.isEqual( reproduce, true ) )
	return;

    addToHistory( "steps_to_reproduce" );
    stepsreproduce_ = reproduce;
}


void SqlDB::BugTextTableEntry::setReporter( BufferString& reporter )
{
    removeTrailingBlanks( reporter.buf() );
    if ( reporter_.isEqual( reporter, true ) )
	return;

    //TODO
//    addToHistory( "reporter" );
    reporter_ = reporter;
}


void SqlDB::BugTextTableEntry::addToHistory( const char* fldnmptr )
{
    FixedString fldnm( fldnmptr );
    for ( int idx=0; idx<btthistoryset_.size(); idx++ )
    {
	BufferString fieldnm = btthistoryset_[idx]->fieldnm_;
	if ( fieldnm.isEqual( fldnm ) )
	    return;
    }

    BugHistoryTableEntry* history = new BugHistoryTableEntry();
    if ( fldnm=="description" )
	history->type_ = 6;
    else if ( fldnm=="steps_to_reproduce" )
	history->type_ = 8;
    else 
    {
	history=0;
	delete history;
    }

    if ( !history )
	return;

    history->fieldnm_ = fldnm;
    btthistoryset_ += history;
}


//BugTableEntry
const char* SqlDB::BugTableEntry::sKeyBugTable() { return "mantis_bug_table"; }
const char* SqlDB::BugTableEntry::sKeyFixedInVersion()
{ return "fixed_in_version"; }
const char* SqlDB::BugTableEntry::sKeySevere()
{ return "Severe"; }
const char* SqlDB::BugTableEntry::sKeyMinor()
{ return "Minor"; }
int	    SqlDB::BugTableEntry::cStatusNew() { return 10; }
int 	    SqlDB::BugTableEntry::cStatusAssigned() { return 50; }
int 	    SqlDB::BugTableEntry::cStatusFeedback() { return 20; }
int 	    SqlDB::BugTableEntry::cStatusResolved() { return 80; }
int	    SqlDB::BugTableEntry::cStatusClosed() { return 90; }
int	    SqlDB::BugTableEntry::cResolutionOpen() { return 10; }
int	    SqlDB::BugTableEntry::cResolutionFixed() { return 20; }
int	    SqlDB::BugTableEntry::cResolutionWillNotFixed() { return 90; }
int	    SqlDB::BugTableEntry::cSeverityFeature() { return 10; }
int	    SqlDB::BugTableEntry::cSeverityTrivial() { return 20; }
int	    SqlDB::BugTableEntry::cSeverityText() { return 30; }
int	    SqlDB::BugTableEntry::cSeverityTweak() { return 40; }
int	    SqlDB::BugTableEntry::cSeverityMinor() { return 50; }
int	    SqlDB::BugTableEntry::cSeverityMajor() { return 60; }
int	    SqlDB::BugTableEntry::cSeverityCrash() { return 70; }
int	    SqlDB::BugTableEntry::cSeverityBlock() { return 80; }


SqlDB::BugTableEntry::BugTableEntry()
{
    init();
}


void SqlDB::BugTableEntry::init()
{
    id_ = 0;
    projectid_ = MantisDBMgr::cOpenDtectProjectID();
    reporterid_ = 0;
    handlerid_ = 0;
    severity_ = cSeverityMinor();
    status_ = cStatusNew();
    resolution_ = cResolutionOpen();
    category_ = "";
    lastupddate_ = "";
    platform_ = OD::Platform().longName();
    version_ = "";
    fixedinversion_ = "";
    summary_ = "";
}


void SqlDB::BugTableEntry::getQueryInfo( BufferStringSet& colnms,
					 BufferStringSet& values, bool isedit )
{
    if ( !isedit )
    {
	colnms.add( "project_id" ).add( "reporter_id" ).add( "date_submitted" )
	      .add( "category" );

	values.add( toString(projectid_) );
	values.add( toString(reporterid_) );
	values.add( lastupddate_ ).add( category_ );
    }

    colnms.add( "summary" ).add( "handler_id" ).add( "severity" )
	  .add ( "status" ).add( "resolution" ).add( "last_updated" )
	  .add( "platform" ).add( "version" ).add( "fixed_in_version" );

    values.add( summary_ );
    values.add( toString(handlerid_) );
    values.add( toString(severity_) );
    values.add( toString(status_) );
    values.add( toString(resolution_) );
    values.add( lastupddate_ ).add( platform_ ).add( version_ )
	  .add( fixedinversion_ );
}


void SqlDB::BugTableEntry::addToHistory( const char* fldnm, const char* oldval,
					 const char* newval )
{
    bool isexisted = false;
    for ( int idx=0; idx<historyset_.size(); idx++ )
    {
	const BufferString& fieldnm = historyset_[idx]->fieldnm_;
	if ( fieldnm==fldnm )
	{
	    historyset_[idx]->newvalue_ = newval;
	    isexisted = true;
	    break;
	}
    }

    if ( !isexisted )
    {
	BugHistoryTableEntry* historyentry = new BugHistoryTableEntry;
	historyentry->bugid_ = id_;
	historyentry->fieldnm_ = fldnm;
	historyentry->oldvalue_ = oldval;
	historyentry->newvalue_ = newval;
	historyset_ += historyentry;
    }
}


void SqlDB::BugTableEntry::deleteHistory()
{
    deepErase( historyset_ );
}


void SqlDB::BugTableEntry::setSeverity( int val )
{
    if ( val == severity_ ) return;

    BufferString oldvalue( toString(severity_) );
    addToHistory( "severity", oldvalue, toString(val) );
    severity_ = val;
}


void SqlDB::BugTableEntry::setHandlerID( int hid )
{
    if ( hid == handlerid_ )
	return;

    BufferString oldhid( toString(handlerid_) );
    addToHistory( "handler_id", oldhid, toString(hid) );
    handlerid_ = hid;
}


void SqlDB::BugTableEntry::setStatus( int status )
{
    if ( status_ == status )
	return;

    BufferString oldstatus( toString(status_) );
    addToHistory( "status", oldstatus, toString(status) );
    status_ = status;
}


void SqlDB::BugTableEntry::setResolution( int resolution )
{
    if ( resolution_ == resolution )
	return;

    BufferString oldres( toString(resolution_) );
    addToHistory( "resolution", oldres, toString(resolution) );
    resolution_ = resolution;
}


void SqlDB::BugTableEntry::setPlatform( const char* plf )
{
    if ( platform_.isEqual( plf, true ) )
	return;

    addToHistory( "platform", platform_, plf );
    platform_ = plf;
}


void SqlDB::BugTableEntry::setSummary( BufferString& summary )
{
    removeTrailingBlanks( summary.buf() );
    if ( summary_.isEqual( summary ) )
	return;

    BufferString newsummary( summary );
    SqlDB::MantisDBMgr::prepareForQuery( summary_ );
    SqlDB::MantisDBMgr::prepareForQuery( newsummary );
    addToHistory( "summary", summary_, newsummary );
    summary_ = summary;
}


void SqlDB::BugTableEntry::setVersion( const char* version )
{
    if ( version_ == version )
	return;

    addToHistory( "version", version_, version );
    version_ = version;
}


bool SqlDB::BugTableEntry::isSevere( int severity )
{
    return severity == cSeverityFeature() || severity == cSeverityTrivial() ||
	   severity == cSeverityText() || severity == cSeverityTweak() ||
	   severity == cSeverityMinor() ? false : true;
}


//BugHistoryTableEntry
const char* SqlDB::BugHistoryTableEntry::sKeyBugHistoryTable()
{ return "mantis_bug_history_table"; }


SqlDB::BugHistoryTableEntry::BugHistoryTableEntry()
{
    init();
}


void SqlDB::BugHistoryTableEntry::init()
{
    userid_ = 0;
    bugid_ = 0;
    date_ = "";
    fieldnm_ = "";
    oldvalue_ = "";
    newvalue_ = "";
    type_ = 0;
}


void SqlDB::BugHistoryTableEntry::getQueryInfo( BufferStringSet& colnms,
						BufferStringSet& values )
{
    colnms.add( "user_id" ).add( "bug_id" ).add( "date_modified" )
	  .add( "field_name" ).add( "old_value" ).add( "new_value" )
	  .add( "type" );

    values.add( toString(userid_) );
    values.add( toString(bugid_) );
    values.add( date_ ).add( fieldnm_ ).add( oldvalue_ ).add( newvalue_ );
    values.add( toString(type_) );
}
