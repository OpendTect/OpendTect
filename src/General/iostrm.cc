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
#include "perthreadrepos.h"

class IOStreamProducer : public IOObjProducer
{
    bool	canMake( const char* typ ) const
		{ return FixedString(typ)==StreamConn::sType(); }
    IOObj*	make( const char* nm, const MultiID& ky, bool fd ) const
		{ return new IOStream(nm,ky,fd); }

    static int	factoryid_;
};

int IOStreamProducer::factoryid_ = IOObj::addProducer( new IOStreamProducer );


IOStream::IOStream( const char* nm, const char* uid, bool mkdef )
	: IOObj(nm,uid)
	, padzeros_(0)
	, fnrs_(0,0,1)
	, iscomm_(false)
	, curfnr_(0)
{
    if ( mkdef ) genFileName();
}


const char* IOStream::connType() const
{
    return StreamConn::sType();
}


bool IOStream::isBad() const
{
    if ( fname_.isEmpty() )
	return true;

    return iscomm_ && writecmd_.isEmpty();
}


void IOStream::copyFrom( const IOObj* obj )
{
    if ( !obj ) return;

    IOObj::copyFrom(obj);
    mDynamicCastGet(const IOStream*,iosobj,obj)
    if ( iosobj )
    {
	padzeros_ = iosobj->padzeros_;
	fname_ = iosobj->fname_;
	writecmd_ = iosobj->writecmd_;
	iscomm_ = iosobj->iscomm_;
	fnrs_ = iosobj->fnrs_;
	curfnr_ = iosobj->curfnr_;
    }
}


const char* IOStream::fullDirName() const
{
    FilePath fp( fname_ );
    if ( fp.isAbsolute() )
	fp.setFileName( 0 );
    else
	{ fp.set( IOM().rootDir() ); fp.add( dirName() ); }

    mDeclStaticString( ret );
    ret = fp.fullPath();
    return ret.buf();
}


const char* IOStream::fullUserExpr( bool forread ) const
{
    return isMulti() ? (const char*)fname_ : getExpandedName(forread);
}


bool IOStream::implExists( bool fr ) const
{
    StreamProvider* sp = getStreamProv( fr );
    const bool ret = sp && sp->exists( fr );
    delete sp;
    return ret;
}


bool IOStream::implReadOnly() const
{
    StreamProvider* sp = getStreamProv( true );
    const bool ret = sp && sp->isReadOnly();
    delete sp;
    return ret;
}


bool IOStream::implShouldRemove() const
{
    return !iscomm_;
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
    if ( iscomm_ ) return false;

    int curnrfiles = isMulti() ? fnrs_.nrSteps() + 1 : 1;
    int kpcurfnr = curfnr_;
    int& fnr = const_cast<int&>( curfnr_ );

    bool ret = true;
    for ( int idx=0; idx<curnrfiles; idx++ )
    {
	fnr = fnrs_.start + idx*fnrs_.step;
	StreamProvider* sp = getStreamProv( true );
	bool thisret = dorem ? sp->remove(yn) : sp->setReadOnly(yn);
	delete sp;
	if ( !thisret ) ret = false;
    }

    fnr = kpcurfnr;
    return ret;
}


bool IOStream::implRename( const char* newnm, const CallBack* cb )
{
    StreamProvider* sp = getStreamProv( true );
    bool rv = !sp || sp->rename( newnm, cb );
    if ( rv )
	setFileName( newnm );
    delete sp;
    return rv;
}


Conn* IOStream::getConn( bool forread ) const
{
    if ( isBad() )
	const_cast<IOStream*>(this)->genFileName();

    const BufferString implnm( getExpandedName(forread) );
    StreamConn*	ret = new StreamConn( implnm, forread );
    if ( !ret || ret->isBad() )
	{ delete ret; ret = 0; }
    else
	ret->ioobj = this;

    return ret;
}


void IOStream::genFileName()
{
    iscomm_ = false;
    writecmd_.setEmpty();

    fname_ = name();
    FilePath fp( fname_ );
    const bool isabs = fp.isAbsolute();
    fname_.clean( isabs ? BufferString::NoSpaces : BufferString::AllowDots );
    const int extsz = extension_.size();
    const int neededsz = fname_.size() + extsz;
    if ( neededsz >= mMaxFilePathLength )
    {
	const BufferString uniqstr( "_",
			FilePath(FilePath::getTempName()).fileName() );
	const int len = uniqstr.size();
	fname_[ mMaxFilePathLength - len - extsz - 1 ] = '\0';
	fname_.add( uniqstr );
    }

    if ( extsz > 0 )
	fname_.add( "." ).add( extension_ );
}


void IOStream::setReader( const char* str )
{
    iscomm_ = true;
    fname_ = str;
}


void IOStream::setWriter( const char* str )
{
    iscomm_ = true;
    writecmd_ = str;
}


void IOStream::setFileName( const char* str )
{

    StreamProvider sp = StreamProvider( str );
    if ( sp.isCommand() )
    {
	const char* rwcmd = sp.command();
	setReader( rwcmd ); setWriter( rwcmd );
    }
    else
    {
	iscomm_ = false;
	writecmd_.setEmpty();
	fname_ = sp.fullName();
    }
}


#define mStrmNext() { stream.next(); kw = stream.keyWord() + 1; }


bool IOStream::getFrom( ascistream& stream )
{
    FixedString kw = stream.keyWord() + 1;

    if ( kw == "Extension" )
    {
	extension_ = stream.value();
	mStrmNext()
    }
    if ( kw == "Multi" )
    {
	FileMultiString fms( stream.value() );
	fnrs_.start = fms.getIValue( 0 );
	fnrs_.stop = fms.getIValue( 1 );
	fnrs_.step = fms.getIValue( 2 );
	if ( fnrs_.step == 0 ) fnrs_.step = 1;
	if ( ( fnrs_.start < fnrs_.stop && fnrs_.step < 0 )
	  || ( fnrs_.stop < fnrs_.start && fnrs_.step > 0 ) )
	    Swap( fnrs_.start, fnrs_.stop );
	padzeros_ = fms.getIValue( 3 );
	curfnr_ = fnrs_.start;
	mStrmNext()
    }

    fname_ = stream.value();
    if ( kw == "Name" )
    {
	iscomm_ = false;
	writecmd_.setEmpty();
    }
    else if ( kw == "Reader" )
    {
	mStrmNext()
	setWriter( stream.value() );
    }
    else
	genFileName();

    stream.next();
    return true;
}


bool IOStream::putTo( ascostream& stream ) const
{
    if ( !extension_.isEmpty() )
	stream.put( "$Extension", extension_ );
    if ( isMulti() )
    {
	FileMultiString fms;
	fms += fnrs_.start;
	fms += fnrs_.stop;
	fms += fnrs_.step;
	fms += padzeros_;
	stream.put( "$Multi", fms );
    }

    if ( iscomm_ )
    {
	stream.put( "$Reader", fname_ );
	stream.put( "$Writer", writecmd_ );
    }
    else
    {
	FilePath fp( fname_ );
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
    }

    return true;
}


const char* IOStream::getExpandedName( bool forread, bool fillwc ) const
{
    StreamProvider* sp = getStreamProv( forread, fillwc );
    mDeclStaticString( ret );
    ret = sp ? sp->fullName() : "";
    delete sp;
    return ret.buf();
}


StreamProvider* IOStream::getStreamProv( bool fr, bool fillwc ) const
{
    BufferString nm( iscomm_ && !fr ? writer() : (const char*)fname_ );

    const bool hasast = nm.contains( '*' );
    const bool doins = fillwc && isMulti() && (hasast || nm.contains('%'));
    if ( doins )
    {
	BufferString numbstr( "", curfnr_ );
	if ( padzeros_ )
	{
	    BufferString padded;
	    int len = numbstr.size();
	    for ( int idx=len; idx<padzeros_; idx++ )
		padded.add( "0" );
	    padded.add( numbstr );
	    numbstr = padded;
	}
	nm.replace( hasast ? "*" : "%", numbstr.buf() );
    }

    StreamProvider* sp = iscomm_ ? new StreamProvider( BufferString("@",nm))
				 : new StreamProvider( nm );
    if ( !sp || sp->isBad() )
	{ delete sp; return 0; }

    if ( !iscomm_ )
	sp->addPathIfNecessary( fullDirName() );

    if ( fr && doins && padzeros_ && !iscomm_ && !sp->exists(fr) )
    {
	int kppz = padzeros_;
	const_cast<IOStream*>(this)->padzeros_ = 0;
	StreamProvider* trysp = getStreamProv( fr );
	if ( trysp && trysp->exists(fr) )
	    { delete sp; sp = trysp; trysp = 0; }
	delete trysp;
	const_cast<IOStream*>(this)->padzeros_ = kppz;
    }

    return sp;
}
