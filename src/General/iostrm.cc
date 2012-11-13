/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : A.H. Bril
 * DATE     : 25-10-1994
-*/

static const char* rcsID mUsedVar = "$Id$";

#include "iostrm.h"
#include "ioman.h"
#include "ascstream.h"
#include "strmprov.h"
#include "separstr.h"
#include "string2.h"
#include "file.h"
#include "filepath.h"
#include "staticstring.h"

class IOStreamProducer : public IOObjProducer
{
    bool	canMake( const char* typ ) const
		{ return FixedString(typ)==StreamConn::sType(); }
    IOObj*	make( const char* nm, const MultiID& ky, bool fd ) const
		{ return new IOStream(nm,ky,fd); }
};

int IOStream::prodid = IOObj::addProducer( new IOStreamProducer );


IOStream::IOStream( const char* nm, const char* uid, bool mkdef )
	: IOObj(nm,uid)
	, readcmd(&fname)
	, writecmd(0)
	, padzeros(0)
	, fnrs(0,0,1)
	, iscomm(false)
	, curfnr(0)
{
    if ( mkdef ) genFileName();
}


IOStream::~IOStream()
{
    delete writecmd;
}


FixedString IOStream::connType() const
{
    return StreamConn::sType();
}


bool IOStream::bad() const
{
    return !iscomm && fname.isEmpty();
}


void IOStream::copyFrom( const IOObj* obj )
{
    if ( !obj ) return;

    IOObj::copyFrom(obj);
    mDynamicCastGet(const IOStream*,iosobj,obj)
    if ( iosobj )
    {
	hostname = iosobj->hostname;
	padzeros = iosobj->padzeros;
	fname = iosobj->fname;
	writecmd = iosobj->writecmd ? new FileNameString(*iosobj->writecmd) : 0;
	iscomm = iosobj->iscomm;
	fnrs = iosobj->fnrs;
	curfnr = iosobj->curfnr;
    }
}


const char* IOStream::getExpandedName( bool forread, bool fillwc ) const
{
    static StaticStringManager stm;
    BufferString& ret = stm.getString();
    StreamProvider* sp = streamProvider( forread, fillwc );
    if ( !sp ) return "<bad>";
    ret = sp->fullName();
    delete sp;
    return ret;
}


const char* IOStream::fullDirName() const
{
    FilePath fp( fname );
    if ( fp.isAbsolute() )
	fp.setFileName( 0 );
    else
	{ fp.set( IOM().rootDir() ); fp.add( dirName() ); }
    static StaticStringManager stm;
    BufferString& ret = stm.getString();
    ret = fp.fullPath();
    return ret.buf();
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
    return !iscomm;
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
    if ( iscomm ) return false;

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

    if ( !iscomm && fname == "?" )
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
    FilePath fp( fname );
    const bool isabs = fp.isAbsolute();
    cleanupString( fname.buf(), false, isabs, true );
    if ( !extension.isEmpty() )
    {
        fname += ".";
        fname += extension;
    }
}


void IOStream::setReader( const char* str )
{
    if ( !str ) return;
    iscomm = true;
    fname = str;
}


void IOStream::setWriter( const char* str )
{
    if ( !str ) return;
    iscomm = true;
    if ( !writecmd ) writecmd = new FileNameString( str );
    else if ( *writecmd == str ) return;
    *writecmd = str;
}


void IOStream::setFileName( const char* str )
{
    iscomm = false;
    StreamProvider sp = StreamProvider( str );
    if ( !sp.isCommand() )
	fname = sp.fullName();
    else
    {
	setReader( sp.command() );
	setWriter( sp.command() );
    }
}



bool IOStream::getFrom( ascistream& stream )
{
    FixedString kw = stream.keyWord() + 1;
    if ( kw=="Hostname" )
    {
	hostname = stream.value();
	stream.next();
    }
    if ( kw=="Extension" )
    {
	extension = stream.value();
	stream.next();
    }
    if ( kw=="Multi" )
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
    if ( kw=="Name" )
	iscomm = false;
    else if ( kw=="Reader" )
    {
	iscomm = true;
	stream.next();
	if ( !writecmd ) writecmd = new FileNameString( stream.value() );
	else *writecmd = stream.value();
    }
    else
	return false;

    stream.next();
    return true;
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

    if ( iscomm )
    {
	stream.put( "$Reader", fname );
	stream.put( "$Writer", writecmd ? *writecmd : fname );
	return true;
    }
    else
    {
	FilePath fp( fname );
	BufferString cleanfnm( fp.fullPath() );
	int offs = 0;
	if ( fp.isAbsolute() )
	{
	    FilePath fpdir( IOM().rootDir(), dirName() );
	    BufferString head( fp.dirUpTo( fpdir.nrLevels() - 1 ) );
	    if ( head == fpdir.fullPath() )
		offs = head.size()+1;
	}

	stream.put( "$Name", cleanfnm.buf() + offs );
	return true;
    }

    return false;
}


StreamProvider* IOStream::streamProvider( bool fr, bool fillwc ) const
{
    FileNameString nm( iscomm && !fr ? writer() : (const char*)fname );

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

    StreamProvider* sp = new StreamProvider( hostname, nm, iscomm );
    if ( sp->bad() )
	{ delete sp; return 0; }
    if ( hostname.isEmpty() && !iscomm )
	sp->addPathIfNecessary( fullDirName() );

    if ( fr && doins && padzeros && !iscomm && !sp->exists(fr) )
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
