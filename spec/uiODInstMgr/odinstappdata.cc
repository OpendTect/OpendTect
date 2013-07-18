/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          Nov 2011
________________________________________________________________________

-*/
static const char* rcsID mUsedVar = "$Id: odinstappdata.cc 7937 2013-06-03 05:38:18Z ranojay.sen@dgbes.com $";

#include "odinstappdata.h"
#include "odinstrel.h"

#include "file.h"
#include "filepath.h"
#include "strmprov.h"
#include "strmoper.h"
#include "dirlist.h"

const char* ODInst::AppData::sKeyRelInfoSubDir() { return "relinfo"; }
const char* ODInst::AppData::sKeyUpdateDir() { return "update"; };


int ODInst::InstalledPkgSet::getIndexOf( const char* pkgnm,
					 ODInst::Platform plf ) const
{
    for ( int ipkg=0; ipkg<size(); ipkg++ )
    {
	const PkgKey& pk = *(*this)[ipkg];
	if ( (pk.plf_.isIndep() || pk.plf_ == plf) && pk.nm_ == pkgnm )
	    return ipkg;
    }

    return -1;
}


bool ODInst::InstalledPkgSet::readFileNames( const char* appdir, int idx,
					  BufferStringSet& fnms ) const
{
    const FilePath fp( appdir, ODInst::AppData::sKeyRelInfoSubDir(),
	    	       pkgFileName(idx,false) );
    StreamData sd( StreamProvider(fp.fullPath()).makeIStream() );
    if ( !sd.usable() ) return false;

    BufferString* readstr = new BufferString;
    while ( StrmOper::readLine(*sd.istrm,readstr) )
	{ fnms += readstr; readstr = new BufferString; }
    delete readstr;
    sd.close();
    return true;
}


BufferString ODInst::InstalledPkgSet::pkgFileName( int idx, bool version ) const
{
    if ( idx < 0 || idx >= size() ) return BufferString("");

    BufferString ret( version ? "ver." : "files." );
    ret.add( (*this)[idx]->fileNameBase() ).add( ".txt" );
    return ret;
}


ODInst::AppData::AppData( const char* basedirnm, const char* reldirnm )
    : reltype_(ODInst::Stable)
    , platform_(Platform::thisPlatform())
{
    set( basedirnm, reldirnm );
}


#define mErrRet(s1,s2) { \
    exists_ = writable_ = false; \
    errmsg_ = s1; errmsg_ += s2; \
    deepErase( pkgs_ ); \
    return false; }


bool ODInst::AppData::setUpdateMode()
{
    if ( basedir_.isEmpty() )
	return false;

    FilePath fp( basedir_, sKeyUpdateDir() );
    basedir_ = fp.fullPath();
    return true;
}


bool ODInst::AppData::set( const char* basedirnm, const char* reldirnm )
{
    basedir_ = basedirnm; fulldirnm_.setEmpty();
    exists_ = writable_ = false;
    errmsg_.setEmpty();

    if ( basedir_.isEmpty() )
	mErrRet("No base directory"," provided")
    else if ( !reldirnm || !*reldirnm )
	mErrRet("No release directory"," provided")

    fulldirnm_ = FilePath(basedir_,reldirnm).fullPath();
    exists_ = File::isDirectory( fulldirnm_ );
    writable_ = exists_ && File::isWritable( fulldirnm_ );

    if ( !exists_ )
    {
	const BufferString indir( FilePath(basedir_).pathOnly() );
	if ( !File::isDirectory(indir) )
	    mErrRet(indir," does not exist")
	if ( File::exists(fulldirnm_) )
	    mErrRet(fulldirnm_," is not a directory")

        deepErase( pkgs_ );
	return true;
    }

    FilePath relinfofp( fulldirnm_, sKeyRelInfoSubDir() );
    const BufferString relinfodirnm( relinfofp.fullPath() );
    if ( !File::isDirectory(relinfodirnm) )
	mErrRet(fulldirnm_," cannot be upgraded (no 'relinfo' directory)")

    const BufferString reltxtfnm( FilePath(relinfofp,"README.txt").fullPath() );
    if ( !File::exists(reltxtfnm) )
	mErrRet("README.txt missing in relinfo directory:\n",relinfodirnm);
    StreamData sd( StreamProvider(reltxtfnm).makeIStream() );
    if ( !sd.usable() )
	mErrRet("Cannot open ",reltxtfnm)

    char appnm[64], relstr[64];
    appnm[0] = relstr[0] = '\0';
    StrmOper::wordFromLine(*sd.istrm,appnm,64);
    StrmOper::wordFromLine(*sd.istrm,relstr,64);
    sd.close();
    int relstrlen = strlen( relstr );
    if ( appnm[0] != '[' || relstrlen < 4 || relstr[0] != '('
	|| relstr[relstrlen-1] != ']' || relstr[relstrlen-2] != ')' )
	mErrRet(reltxtfnm," is not correct")
    relstr[relstrlen-2] = '\0';

    ODInst::parseEnumRelType( relstr+1, reltype_ );
    getInstalledPkgs();
    return true;
}


const char* ODInst::AppData::dirName() const
{
    return strrchr( fulldirnm_.buf(), *FilePath::dirSep(FilePath::Local) ) + 1;
}


BufferString ODInst::AppData::getReport() const
{
    BufferString ret;
    if ( fulldirnm_.isEmpty() )
	ret = "Empty";
    else
    {
	ret.add( "Directory: " ).add( fulldirnm_ )
	    .add( exists_ ? (writable_ ? " (OK)" : " (read-only)" ) : " (new)" )
	    .add( ".\nType: " ).add( ODInst::toString(reltype_) ).add( "." );
    }
    return ret;
}


void ODInst::AppData::getInstalledPkgs()
{
    deepErase( pkgs_ );

    const FilePath relinfofp( fulldirnm_, sKeyRelInfoSubDir() );
    const BufferString relinfodirnm( relinfofp.fullPath() );
    DirList dl( relinfodirnm, DirList::FilesOnly, "ver.*.txt" );
    for ( int idx=0; idx<dl.size(); idx++ )
    {
	StreamData sd( StreamProvider(dl.fullPath(idx)).makeIStream() );
	if ( !sd.usable() ) continue;
	BufferString verstr; StrmOper::readLine( *sd.istrm, &verstr );
	sd.close();

	if ( !verstr.isEmpty() )
	    pkgs_ += new ODInst::PkgKey( dl.get(idx), verstr );
    }
}


bool ODInst::AppData::needUpdate( const BufferStringSet& pkstrs,
				  ODInst::Platform plf,
				  BufferStringSet* nms ) const
{
    bool rv = false;
    for ( int idx=0; idx<pkstrs.size(); idx++ )
    {
	const PkgKey avpk( pkstrs.get(idx) );
	if ( !(avpk.plf_.isIndep() || avpk.plf_ == plf) )
	    continue;

	const PkgKey* instpk = pkgs_.get( avpk.nm_, plf );
	if ( !instpk )
	    continue;

	if ( avpk.ver_ > instpk->ver_ )
	{
	    rv = true;
	    if ( nms )
		nms->add( avpk.nm_ );
	}
    }

    return rv;
}


BufferString ODInst::AppData::relInfoDirName() const
{
    FilePath fp( fulldirnm_, sKeyRelInfoSubDir() );
    return BufferString( fp.fullPath() );
}


BufferString ODInst::AppData::binPlfDirName() const
{
    FilePath fp( fulldirnm_, "bin", platform_.getPlfSubDir() );
    FilePath relfp( fp.fullPath(), "Release" );
    return File::exists(relfp.fullPath()) ? relfp.fullPath() : fp.fullPath();
}


BufferString ODInst::AppData::binPlfBaseDirName() const
{
    FilePath fp( fulldirnm_, "bin", platform_.getPlfSubDir() );
    return fp.fullPath();
}


bool ODInst::AppData::getRunningProgs( BufferStringSet& list ) const
{
    FilePath binfp( binPlfDirName() ); 
    DirList dl( binfp.fullPath(), DirList::FilesOnly, "od_*.exe" );
    for ( int idx=0; idx<dl.size(); idx++ )
    {
	const BufferString filename =  dl.get( idx );
	if ( filename == "od_instmgr.exe" )
	    continue;
	if ( File::isFileInUse(dl.fullPath(idx)) )
	    list.add( filename );
    }

    return list.size();
}


bool ODInst::AppData::getFileList( const char* pkgnm,
				   BufferString& filelist ) const
{
    filelist.setEmpty();
    const PkgKey* pk = pkgs_.get( pkgnm, platform_ );
    if ( !pk ) return false;
    BufferString listfile(FilePath(relInfoDirName(),pk->listFileName())
	    			  .fullPath() );
    if ( !File::exists(listfile) ) return false;

    StreamData sd = StreamProvider(listfile).makeIStream();
    if ( !sd.usable() )	return false;

    while ( *sd.istrm )
    {
	BufferString line;
	char desc[255];
	*sd.istrm >> desc;
	StrmOper::readLine( *sd.istrm, &line );
	filelist += line; filelist += "\n";
    }
    sd.close();

    return !filelist.isEmpty();
}
