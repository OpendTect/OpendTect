/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : A.H. Bril
 * DATE     : 25-10-1994
-*/

static const char* rcsID = "$Id: iostrm.cc,v 1.33 2010-10-14 09:58:06 cvsbert Exp $";

#include "iostrm.h"
#include "iolink.h"
#include "ascstream.h"
#include "strmprov.h"
#include "separstr.h"
#include "string2.h"
#include "file.h"
#include "filepath.h"

class IOStreamProducer : public IOObjProducer
{
    bool	canMake( const char* typ ) const
		{ return !strcmp(typ,StreamConn::sType()); }
    IOObj*	make( const char* nm, const MultiID& ky, bool fd ) const
		{ return new IOStream(nm,ky,fd); }
};

int IOStream::prodid = IOObj::addProducer( new IOStreamProducer );


IOStream::IOStream( const char* nm, const char* uid, bool mkdef )
	: IOObject(nm,uid)
	, readcmd(&fname)
	, writecmd(0)
	, blocksize(0)
	, skipfiles(0)
	, rew(false)
	, padzeros(0)
	, fnrs(0,0,1)
	, type_(StreamConn::File)
	, curfnr(0)
{
    if ( mkdef ) genFileName();
}


IOStream::~IOStream()
{
    delete writecmd;
}


const char* IOStream::connType() const
{
    return StreamConn::sType();
}


bool IOStream::bad() const
{
    return type_ != StreamConn::Command && fname.isEmpty();
}


void IOStream::copyFrom( const IOObj* obj )
{
    if ( !obj ) return;
    if ( obj->isLink() ) obj = ((IOLink*)obj)->link();

    IOObj::copyFrom(obj);
    mDynamicCastGet(const IOStream*,iosobj,obj)
    if ( iosobj )
    {
	hostname = iosobj->hostname;
	padzeros = iosobj->padzeros;
	fname = iosobj->fname;
	writecmd = iosobj->writecmd ? new FileNameString(*iosobj->writecmd) : 0;
	type_ = iosobj->type_;
	blocksize = iosobj->blocksize;
	skipfiles = iosobj->skipfiles;
	rew = iosobj->rew;
	fnrs = iosobj->fnrs;
	curfnr = iosobj->curfnr;
    }
}


const char* IOStream::getExpandedName( bool forread, bool fillwc ) const
{
    static BufferString ret;
    StreamProvider* sp = streamProvider( forread, fillwc );
    if ( !sp ) return "<bad>";
    ret = sp->fullName();
    delete sp;
    return ret;
}


const char* IOStream::fullUserExpr( bool forread ) const
{
    return isMulti() ? (const char*)fname : getExpandedName(forread);
}


bool IOStream::implExists( bool fr ) const
{
    StreamProvider* sp = streamProvider( fr );
    bool ret = sp && sp->exists( fr );
    delete sp;
    return ret;
}


bool IOStream::implReadOnly() const
{
    StreamProvider* sp = streamProvider( true );
    bool ret = sp && sp->isReadOnly();
    delete sp;
    return ret;
}


bool IOStream::implShouldRemove() const
{
    return type_ == StreamConn::File;
}


bool IOStream::implRemove() const
{
    return implDo( true, true );
}


bool IOStream::implSetReadOnly( bool yn ) const
{
    return implDo( false, yn );
}


bool IOStream::implDo( bool dorem, bool yn ) const
{
    if ( type_ != StreamConn::File ) return false;

    int curnrfiles = isMulti() ? fnrs.nrSteps() + 1 : 1;
    int kpcurfnr = curfnr;
    int& fnr = const_cast<int&>( curfnr );

    bool ret = true;
    for ( int idx=0; idx<curnrfiles; idx++ )
    {
	fnr = fnrs.start + idx*fnrs.step;
	StreamProvider* sp = streamProvider( true );
	bool thisret = sp && (dorem ? sp->remove(yn) : sp->setReadOnly(yn));
	delete sp;
	if ( !thisret ) ret = false;
    }

    fnr = kpcurfnr;
    return ret;
}


bool IOStream::implRename( const char* newnm, const CallBack* cb )
{
    StreamProvider* sp = streamProvider( true );
    if ( !sp ) return true;

    bool rv = sp->rename( newnm, cb );
    delete sp;

    if ( rv )
	setFileName( newnm );
    return true;
}


Conn* IOStream::getConn( Conn::State rw ) const
{
    bool fr = rw == Conn::Read;
    StreamProvider* sp = streamProvider( fr );
    if ( !sp ) return 0;

    if ( type_ == StreamConn::Device )
    {
	sp->setBlockSize( blocksize );
	if ( rew ) sp->rewind();
	if ( skipfiles ) sp->skipFiles( skipfiles );
    }
    else if ( type_ == StreamConn::File && fname == "?" )
    {
	((IOStream*)this)->genFileName();
	delete sp;
	sp = streamProvider( fr );
    }

    StreamConn*	s = 0;
    StreamData sd( fr ? sp->makeIStream() : sp->makeOStream() );
    if ( !sd.usable() && !File::isDirectory(sp->fileName()) )
	{ delete sp; return 0; }

    s = new StreamConn( sd );
    s->ioobj = (IOObj*)this;
    delete sp;
    return s;
}


void IOStream::genFileName()
{
    fname = name();
    cleanupString( fname.buf(), mC_False, mC_True, mC_True );
    if ( !extension.isEmpty() )
    {
        fname += ".";
        fname += extension;
    }
}


void IOStream::setReader( const char* str )
{
    if ( !str ) return;
    type_ = StreamConn::Command;
    fname = str;
}


void IOStream::setWriter( const char* str )
{
    if ( !str ) return;
    type_ = StreamConn::Command;
    if ( !writecmd ) writecmd = new FileNameString( str );
    else if ( *writecmd == str ) return;
    *writecmd = str;
}


void IOStream::setFileName( const char* str )
{
    type_ = StreamConn::File;
    StreamProvider sp = StreamProvider( str );
    type_ = sp.type();
    if ( type_ != StreamConn::Command )
	fname = sp.fullName();
    else
    {
	setReader( sp.command() );
	setWriter( sp.command() );
    }
}


const char* IOStream::devName() const
{
    const char* retptr = fname;
    return matchString( "/dev/", retptr ) ? retptr + 5 : retptr;
}


void IOStream::setDevName( const char* str )
{
    if ( !str ) return;
    type_ = StreamConn::Device;
    fname = "/dev/";
    if ( matchString("/dev/",str) ) fname = str;
    else			    fname += str;
}


bool IOStream::getFrom( ascistream& stream )
{
    const char* kw = stream.keyWord() + 1;
    if ( !strcmp(kw,"Hostname") )
    {
	hostname = stream.value();
	stream.next();
    }
    if ( !strcmp(kw,"Extension") )
    {
	extension = stream.value();
	stream.next();
    }
    if ( !strcmp(kw,"Multi") )
    {
	FileMultiString fms( stream.value() );
	fnrs.start = toInt(fms[0]);
	fnrs.stop = toInt(fms[1]);
	fnrs.step = toInt(fms[2]); if ( fnrs.step == 0 ) fnrs.step = 1;
	if ( ( fnrs.start < fnrs.stop && fnrs.step < 0 )
	  || ( fnrs.stop < fnrs.start && fnrs.step > 0 ) )
	    Swap( fnrs.start, fnrs.stop );
	padzeros = toInt(fms[3]);
	curfnr = fnrs.start;
	stream.next();
    }

    fname = stream.value();
    if ( !strcmp(kw,"Name") )
	type_ = StreamConn::File;
    else if ( !strcmp(kw,"Reader") )
    {
	type_ = StreamConn::Command;
	stream.next();
	if ( !writecmd ) writecmd = new FileNameString( stream.value() );
	else *writecmd = stream.value();
    }
    else if ( !strcmp("Device", kw) )
	{ getDev(stream); return true; }
    else
	return false;

    stream.next();
    return true;
}


void IOStream::getDev( ascistream& stream )
{
    const char* kw = stream.keyWord() + 1;
    type_ = StreamConn::Device;
    while ( !atEndOfSection(stream.next()) )
    {
	if ( *stream.keyWord() == '#' ) break;
	if ( !strcmp(kw,"Blocksize") )
	    blocksize = toInt(stream.value());
	else if ( !strcmp(kw,"Fileskips") )
	    skipfiles = toInt(stream.value());
	else if ( !strcmp(kw,"Rewind") )
	    rew = yesNoFromString(stream.value());
    }
}


bool IOStream::putTo( ascostream& stream ) const
{
    if ( !hostname.isEmpty() )
	stream.put( "$Hostname", hostname );
    if ( !extension.isEmpty() )
	stream.put( "$Extension", extension );
    if ( isMulti() )
    {
	FileMultiString fms;
	fms += fnrs.start;
	fms += fnrs.stop;
	fms += fnrs.step;
	fms += padzeros;
	stream.put( "$Multi", fms );
    }

    switch( type_ )
    {
    case StreamConn::File:
    {
	FilePath fp( fname );
	BufferString cleanfnm( fp.fullPath() );
	int offs = 0;
	if ( fp.isAbsolute() )
	{
	    FilePath fpdir( dirName() );
	    BufferString head( fp.dirUpTo( fpdir.nrLevels() - 1 ) );
	    if ( head == fpdir.fullPath() )
		offs = head.size()+1;
	}

	stream.put( "$Name", cleanfnm.buf() + offs );
	return true;
    }
    case StreamConn::Command:
	stream.put( "$Reader", fname );
	stream.put( "$Writer", writecmd ? *writecmd : fname );
	return true;
    case StreamConn::Device:
	stream.put( "$Device", fname );
	if ( blocksize )
	    stream.put( "$Blocksize", blocksize );
	if ( skipfiles )
	    stream.put( "$Fileskips", skipfiles );
	if ( rew )
	    stream.put( "$Rewind", getYesNoString(rew) );
	return true;
    }

    return false;
}


StreamProvider* IOStream::streamProvider( bool fr, bool fillwc ) const
{
    FileNameString nm( type_ == StreamConn::Command && !fr
			? writer() : (const char*)fname );

    const bool hasast = strchr( nm, '*' );
    const bool doins = fillwc && isMulti() && (hasast || strchr(nm,'%'));
    if ( doins )
    {
	char numb[80], numbstr[80]; numbstr[0] = '\0';
	sprintf( numb, "%d", curfnr );
	if ( padzeros )
	{
	    int len = strlen( numb );
	    for ( int idx=len; idx<padzeros; idx++ )
		strcat( numbstr, "0" );
	}
	strcat( numbstr, numb );
	replaceString( nm.buf(), hasast ? "*" : "%", numbstr );
    }

    StreamProvider* sp = new StreamProvider( hostname, nm, type_ );
    if ( sp->bad() )
	{ delete sp; return 0; }
    if ( hostname.isEmpty() && type_ == StreamConn::File )
	sp->addPathIfNecessary( dirName() );
    if ( blocksize ) sp->setBlockSize( blocksize );

    if ( fr && doins && padzeros && type_ == StreamConn::File
      && !sp->exists(fr) )
    {
	int kppz = padzeros;
	const_cast<IOStream*>(this)->padzeros = 0;
	StreamProvider* trysp = streamProvider( fr );
	if ( trysp && trysp->exists(fr) )
	    { delete sp; sp = trysp; trysp = 0; }
	delete trysp;
	const_cast<IOStream*>(this)->padzeros = kppz;
    }

    return sp;
}
