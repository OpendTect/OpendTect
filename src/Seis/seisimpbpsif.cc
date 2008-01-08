/*+
 * COPYRIGHT: (C) dGB Beheer B.V.
 * AUTHOR   : A.H. Bril
 * DATE     : Oct 2003
-*/

static const char* rcsID = "$Id: seisimpbpsif.cc,v 1.1 2008-01-08 12:59:14 cvsbert Exp $";

#include "seisimpbpsif.h"
#include "seispswrite.h"
#include "seisbuf.h"
#include "seistrc.h"
#include "strmprov.h"
#include "strmoper.h"
#include "dirlist.h"
#include "filepath.h"
#include "ioman.h"
#include "ioobj.h"
#include "oddirs.h"
#include <iostream>



SeisImpBPSIF::SeisImpBPSIF( const char* filenm, const MultiID& )
    	: pswrr_(0)
    	, curfileidx_(-1)
{
    if ( !filenm || !*filenm )
	return;

    FilePath fp( filenm );
    if ( !fp.isAbsolute() )
	fp.setPath( GetBaseDataDir() );

    const char* ptr = strchr( filenm, '*' );
    if ( !ptr )
    {
	fnames_.add( fp.fullPath() );
	return;
    }

    DirList dl( fp.pathOnly(), DirList::FilesOnly, fp.fileName() );
    for ( int idx=0; idx<dl.size(); idx++ )
	fnames_.add( dl.fullPath(idx) );
}


SeisImpBPSIF::~SeisImpBPSIF()
{
    cursd_.close();
    delete pswrr_;
}


bool SeisImpBPSIF::open( const char* fnm )
{
    cursd_.close();
    cursd_ = StreamProvider(fnm).makeIStream();
    return cursd_.usable();
}


bool SeisImpBPSIF::openNext()
{
    curfileidx_++;
    if ( curfileidx_ > fnames_.size() )
	return false;

    if ( !readFileHeader() || !open( fnames_.get(curfileidx_) ) )
	return openNext();

    return true;
}


#define mErrRet(s) \
	{ errmsg_ = fnames_.get(curfileidx_); errmsg_ += s; return false; }


bool SeisImpBPSIF::readFileHeader()
{
    char buf[10];
    cursd_.istrm->getline( buf, 10 );
    if ( strcmp(buf,"#BPSIF#") )
	mErrRet(" is not a BPSIF file")
    if ( cursd_.istrm->peek() == '\n' ) cursd_.istrm->ignore( 1 );

    while ( true )
    {
	BufferString* lineread = new BufferString;
	if ( !StrmOper::readLine(*cursd_.istrm,lineread) )
	    return false;
	if ( lineread->isEmpty() )
	    { delete lineread; continue; }

	if ( matchString("#----",lineread->buf()) )
	    { delete lineread; break; }

	hdrlines_ += lineread;
    }

    if ( !(srcattrs_.isEmpty() && rcvattrs_.isEmpty()) )
	return true;

    static const char* valstr = "#Value.";
    for ( int idx=0; idx<hdrlines_.size(); idx++ )
    {
	const BufferString& ln = hdrlines_.get( idx );
	if ( matchString(valstr,ln.buf()) )
	{
	    const char* nrstr = ln.buf() + 7;
	    const char* attrstr = strchr( ln.buf(), ':' );
	    if ( !attrstr ) continue;
	    addAttr( *nrstr == '1' ? srcattrs_ : rcvattrs_, attrstr+1 );
	}
    }

    if ( srcattrs_.isEmpty() && rcvattrs_.isEmpty() )
	mErrRet(" does not contain any attribute data")
    return true;
}


void SeisImpBPSIF::addAttr( BufferStringSet& attrs, const char* attrstr )
{
    skipLeadingBlanks(attrstr);
    const char* ptr = strchr( attrstr, '=' );
    if ( !ptr ) ptr = attrstr;

    if ( *ptr )
	attrs.add( ptr );
}
