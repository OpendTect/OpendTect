/*+
________________________________________________________________________

 Copyright:	(C) 1995-2026 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "bufstring.h"
#include "bufstringset.h"
#include "file.h"
#include "filepath.h"
#include "genc.h"
#include "oddirs.h"
#include "odlockfile.h"
#include "separstr.h"
#include "sets.h"
#include "string2.h"


class SurveyLockEntry
{
public:

			SurveyLockEntry(const char* hostname,int pid,
					const char* app,const char* user);

    bool		belongsToCurrentProcess() const;
    bool		operator ==(const SurveyLockEntry&) const;
    bool		operator !=( const SurveyLockEntry& oth ) const
			    { return !(*this==oth); }

    bool		isAliveLocal() const;
    BufferString	displayText() const;
    BufferString	toStr() const;

    static SurveyLockEntry* fromStr(const char*);

private:

    BufferString	hostname_;
    int			pid_		= 0;
    BufferString	app_;
    BufferString	user_;
};


class SurveyLockMutex
{
public:
			SurveyLockMutex(const char* surveydir);
			~SurveyLockMutex();

    bool		isOK() const		{ return ok_; }

private:

    OD::LockFile	lock_;
    bool		ok_		= false;
};


static BufferString mutexFileName( const char* surveydir )
{
    return FilePath( surveydir, ".od_survey.lock.mutex" ).fullPath();
}


static BufferString& registeredDirStr()
{
    static BufferString* dir = new BufferString;
    return *dir;
}


static bool readRecords( const char* lockfnm,
			 ObjectSet<SurveyLockEntry>& recs )
{
    deepErase( recs );
    if ( !File::exists(lockfnm) )
	return true;

    BufferStringSet lines;
    if ( !File::getContent(lockfnm,lines) )
	return false;

    for ( const auto* line : lines )
    {
	SurveyLockEntry* rec = SurveyLockEntry::fromStr( line->buf() );
	if ( rec )
	    recs += rec;
    }

    return true;
}


static bool writeRecords( const char* lockfnm,
			  const ObjectSet<SurveyLockEntry>& recs )
{
    if ( recs.isEmpty() )
    {
	if ( File::exists(lockfnm) )
	    File::remove( lockfnm );

	return true;
    }

    BufferStringSet lines;
    for ( const auto* rec : recs )
	lines.add( rec->toStr() );

    return File::putContent( lines, lockfnm );
}


static void pruneDead( ObjectSet<SurveyLockEntry>& recs )
{
    for ( int idx=recs.size()-1; idx>=0; idx-- )
    {
	if ( !recs[idx]->isAliveLocal() )
	    delete recs.removeSingle( idx );
    }
}


SurveyLockEntry::SurveyLockEntry( const char* hostname, int pid,
				  const char* app, const char* user )
    : hostname_(hostname)
    , pid_(pid)
    , app_(app)
    , user_(user)
{
}


bool SurveyLockEntry::belongsToCurrentProcess() const
{
    return pid_ == GetPID() &&
	   caseInsensitiveEqual( hostname_.buf(), GetLocalHostName() );
}


bool SurveyLockEntry::operator ==( const SurveyLockEntry& oth ) const
{
    return pid_ == oth.pid_ &&
	   caseInsensitiveEqual( hostname_.buf(), oth.hostname_.buf() );
}


bool SurveyLockEntry::isAliveLocal() const
{
    if ( !caseInsensitiveEqual(hostname_.buf(),GetLocalHostName()) )
	return true; // Cannot probe remote hosts; treat as live

    return pid_ > 0 && isProcessAlive( pid_ );
}


BufferString SurveyLockEntry::displayText() const
{
    BufferString ret;
    if ( !user_.isEmpty() )
	ret.add( user_ ).add( "@" );

    ret.add( hostname_ );
    ret.add( " (PID " ).add( pid_ );
    if ( !app_.isEmpty() )
	ret.add( " " ).add( app_ );

    ret.add( ")" );
    return ret;
}


BufferString SurveyLockEntry::toStr() const
{
    SeparString ss( nullptr, '|' );
    ss.add( hostname_ ).add( pid_ ).add( app_ ).add( user_ );
    return ss.buf();
}


SurveyLockEntry* SurveyLockEntry::fromStr( const char* line )
{
    if ( !line || !*line )
	return nullptr;

    const SeparString ss( line, '|' );
    if ( ss.size() < 2 )
	return nullptr;

    const BufferString hostname( ss[0] );
    const int pid = toInt( ss[1] );
    if ( hostname.isEmpty() || pid <= 0 )
	return nullptr;

    BufferString app, user;
    if ( ss.size() > 2 )
	app = ss[2];

    if ( ss.size() > 3 )
	user = ss[3];

    return new SurveyLockEntry( hostname, pid, app, user );
}


SurveyLockMutex::SurveyLockMutex( const char* surveydir )
    : lock_( mutexFileName(surveydir) )
{
    ok_ = lock_.tryLock( 2000 );
}


SurveyLockMutex::~SurveyLockMutex()
{
    if ( ok_ )
	lock_.unlock();
}


namespace SurveyLockImpl
{

bool		registerInstance(const char* surveydir);
bool		unregisterInstance(const char* surveydir=nullptr);
bool		hasOtherLiveHolders(const char* surveydir);
void		getOtherLiveHolders(const char* surveydir,BufferStringSet&);

static const char* sKeyFileName()
{
    return ".od_survey.lock";
}


static BufferString fileName( const char* surveydir )
{
    return FilePath( surveydir, sKeyFileName() ).fullPath();
}


static const char* registeredDir()
{
    return registeredDirStr().buf();
}


bool registerInstance( const char* surveydir )
{
    if ( !surveydir || !File::isDirectory(surveydir) )
	return false;

    if ( !registeredDirStr().isEmpty() && registeredDirStr() != surveydir )
	unregisterInstance( registeredDir() );

    SurveyLockMutex mtx( surveydir );
    if ( !mtx.isOK() )
	return false;

    const BufferString lockfnm = fileName( surveydir );
    ManagedObjectSet<SurveyLockEntry> recs;
    if ( !readRecords(lockfnm,recs) )
	return false;

    pruneDead( recs );

    const SurveyLockEntry mine( GetLocalHostName(), GetPID(),
				GetExecutableName(), GetUserNm() );

    bool found = false;
    for ( const auto* rec : recs )
    {
	if ( *rec == mine )
	{
	    found = true;
	    break;
	}
    }

    if ( !found )
	recs += new SurveyLockEntry( mine );

    if ( !writeRecords(lockfnm,recs) )
	return false;

    registeredDirStr() = surveydir;
    return true;
}


bool unregisterInstance( const char* surveydir )
{
    BufferString dir( surveydir );
    if ( dir.isEmpty() )
	dir = registeredDir();

    if ( dir.isEmpty() )
	return true;

    const bool wasregisteredhere = registeredDirStr() == dir;
    SurveyLockMutex mtx( dir );
    if ( !mtx.isOK() )
    {
	if ( wasregisteredhere )
	    registeredDirStr().setEmpty();

	return false;
    }

    const BufferString lockfnm = fileName( dir );
    ManagedObjectSet<SurveyLockEntry> recs;
    if ( !readRecords(lockfnm,recs) )
    {
	if ( wasregisteredhere )
	    registeredDirStr().setEmpty();

	return false;
    }

    pruneDead( recs );
    const SurveyLockEntry mine( GetLocalHostName(), GetPID(), nullptr,
				nullptr );
    for ( int idx=recs.size()-1; idx>=0; idx-- )
    {
	if ( *recs[idx] == mine )
	    delete recs.removeSingle( idx );
    }

    const bool ok = writeRecords( lockfnm, recs );
    if ( wasregisteredhere )
	registeredDirStr().setEmpty();

    return ok;
}


bool hasOtherLiveHolders( const char* surveydir )
{
    BufferStringSet infos;
    getOtherLiveHolders( surveydir, infos );
    return !infos.isEmpty();
}


void getOtherLiveHolders( const char* surveydir, BufferStringSet& infos )
{
    infos.setEmpty();
    if ( !surveydir || !*surveydir )
	return;

    SurveyLockMutex mtx( surveydir );
    if ( !mtx.isOK() )
	return;

    const BufferString lockfnm = fileName( surveydir );
    ManagedObjectSet<SurveyLockEntry> recs;
    if ( !readRecords(lockfnm,recs) )
	return;

    const int beforesize = recs.size();
    pruneDead( recs );
    if ( recs.size() != beforesize )
	writeRecords( lockfnm, recs );

    for ( const auto* rec : recs )
    {
	if ( !rec->belongsToCurrentProcess() )
	    infos.add( rec->displayText() );
    }
}

} // namespace SurveyLockImpl
