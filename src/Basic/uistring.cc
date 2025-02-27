/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uistring.h"

#include "bufstringset.h"
#include "envvars.h"
#include "geometry.h"
#include "od_ostream.h"
#include "odmemory.h"
#include "perthreadrepos.h"
#include "ptrman.h"
#include "refcount.h"
#include "survinfo.h"
#include "texttranslator.h"
#include "uistrings.h"

#ifndef OD_NO_QT
# include <QLocale>
# include <QRegularExpression>
# include <QString>
# include <QTranslator>
#endif

#define mForceUpdate (-1)

#ifndef __debug__

# define mSetDBGStr /* nothing */

#else

static char* getNewDebugStr( const BufferString& newstr )
{
    const od_int64 newsz = newstr.size();
    if ( newsz==0 )
	return nullptr;

    auto* strvar = new char [ newsz + 1 ];
    OD::sysMemCopy( strvar, newstr.str(), newsz );
    strvar[newsz] = '\0';
    return strvar;
}

# define mSetDBGStr { \
    delete [] debugstr_; \
    debugstr_ = getNewDebugStr( getString() ); }

#endif


const uiString uiString::emptystring_( toUiString("") );
uiString uiString::dummystring_;
#ifndef OD_NO_QT
Q_GLOBAL_STATIC( QString, emptyqstring );
#endif


class uiStringData : public ReferencedObject
{
  friend class uiString;
public:

    uiStringData( const char* originalstring, const char* context,
		  const char* packagekey,
		  const char* disambiguation, int pluralnr )
	: originalstring_( originalstring )
	, translationcontext_( context )
	, translationpluralnumber_( pluralnr )
	, translationdisambiguation_( disambiguation )
	, packagekey_( packagekey )
	, contentlock_( true )
	, changecount_( mForceUpdate )
    {
    }

    void setFrom( const uiStringData& d )
    {
	Threads::Locker contentlocker( contentlock_ );
	originalstring_ = d.originalstring_;
	translationcontext_ = d.translationcontext_;
	translationpluralnumber_ = d.translationpluralnumber_;
	translationdisambiguation_ = d.translationdisambiguation_;
	arguments_ = d.arguments_;
	packagekey_ = d.packagekey_;
	changecount_ = mForceUpdate;
	tolower_ = d.tolower_;
	totitlecase_ = d.totitlecase_;
    }

    void addAlternateVersion( const uiString& altstr )
    {
	Threads::Locker contentlocker( contentlock_ );
        alternateversions_.add( altstr );
	changecount_ = mForceUpdate;
    }

    void setFrom( const QString& qstr )
    {
#ifndef OD_NO_QT
	Threads::Locker contentlocker( contentlock_ );
	set( 0 );
	qstring_ = qstr;
#endif
    }

    void getFullString(BufferString&) const;

    void set(const char* orig);
    bool fillQString(QString&,const QTranslator* translator,
		     bool nottranslation) const;
    //If no translator is given, default will be used if needed

    mutable Threads::Lock	contentlock_;
    uiStringSet			arguments_;

#ifndef OD_NO_QT
    QString			qstring_;
#endif

    BufferString		originalstring_;
    uiStringSet			alternateversions_;
    BufferString		translationcontext_;
    const char*			packagekey_;
    const char*			translationdisambiguation_;
    int				translationpluralnumber_;
    bool			tolower_		= false;
    bool			totitlecase_		= false;

    int				changecount_;
};


void uiStringData::set( const char* orig )
{
    Threads::Locker contentlocker( contentlock_ );
    originalstring_ = orig;
    arguments_.setEmpty();
    alternateversions_.setEmpty();
    translationcontext_.setEmpty();
    packagekey_ = 0;
    translationdisambiguation_ = 0;
    translationpluralnumber_ = -1;
    changecount_ = mForceUpdate;
    tolower_ = false;
    totitlecase_ = false;
#ifndef OD_NO_QT
    qstring_.clear();
#endif
}


void uiStringData::getFullString( BufferString& ret ) const
{
    Threads::Locker contentlocker( contentlock_ );
#ifndef OD_NO_QT
    if ( !arguments_.size() )
    {
	if ( originalstring_.isEmpty() && qstring_.size() )
	{
	    ret.setEmpty();
	    ret.add( qstring_ );
	}
	else
	    ret = originalstring_;
    }
    else
    {
	QString qres;
	fillQString( qres, 0, true );
	ret = qres;
    }
#else
    ret = originalstring_;
#endif

    if ( tolower_ )
	ret.toLower();

    if ( totitlecase_ )
	ret.toTitleCase();
}



bool uiStringData::fillQString( QString& res,
				const QTranslator* translator,
				bool notranslation ) const
{
#ifndef OD_NO_QT
    Threads::Locker contentlocker( contentlock_ );
    if ( originalstring_.isEmpty() )
        return true;

    bool translationres = false;

    const QTranslator* usedtrans = translator;

    if ( !notranslation && !usedtrans )
	usedtrans = TrMgr().getQTranslator( packagekey_ );

    if ( !notranslation && usedtrans && !translationcontext_.isEmpty() )
    {
	res = usedtrans->translate( translationcontext_, originalstring_,
				     translationdisambiguation_,
				     translationpluralnumber_ );

	if ( res.size() && QString(originalstring_.buf())!=res )
            translationres = true;
	else if ( !alternateversions_.isEmpty() )
	{
	    for ( int idx=0; idx<alternateversions_.size(); idx++ )
	    {
		QString alttrans;
		if ( alternateversions_.get(idx)
					.translate(*usedtrans,alttrans) )
		    { res = alttrans; translationres = true; break; }
	    }
	}

	mDefineStaticLocalObject(bool,dbgtransl,
				 = GetEnvVarYN("OD_DEBUG_TRANSLATION"));
	if ( dbgtransl )
	{
	    BufferString info( translationcontext_, " : " );
	    info.add( originalstring_ ).add( " : " )
		.add( translationdisambiguation_ ).add( " : " )
		.add( translationpluralnumber_ ).add( " : " )
		.add( res );
	    od_cout() << info << od_endl;
	}
    }
    else
    {
	translationres = true;
	res = originalstring_.buf();
    }

    if ( res.isEmpty() )
	res = originalstring_.buf();

    if ( tolower_ )
	res = res.toLower();

    for ( int idx=0; idx<arguments_.size(); idx++ )
    {
	QString thearg;
	if ( notranslation )
	{
	    BufferString str;
	    arguments_.get(idx).getFullString( &str );
	    thearg = str.buf();
	}
	else if ( translator )
	    arguments_.get(idx).translate( *translator, thearg );
	else
	    arguments_.get(idx).fillQString( thearg );

	res = res.arg( thearg );
    }

    if ( totitlecase_ )
    {
	QStringList words = res.split( ' ', Qt::SkipEmptyParts );
	for ( int i=0; i<words.size(); ++i )
	    words[i].replace( 0, 1, words[i][0].toUpper() );

	res = words.join(" ");
    }

    return translationres;
#else
    return true;
#endif
}


#define mEnsureData \
if ( !data_ ) \
{ \
    data_=new uiStringData( 0, 0, 0, 0, -1 ); \
    data_->ref(); \
}

uiString::uiString()
    : data_( 0 )
    , datalock_( true )
    , debugstr_( 0 )
{
}


uiString::uiString( const char* originaltext, const char* context,
		    const char* packagekey,
		    const char* disambiguation, int pluralnr )
    : data_( new uiStringData(originaltext, context, packagekey,
			      disambiguation, pluralnr ))
    , datalock_( true )
    , debugstr_( 0 )
{
    refPtr( data_ );
    mSetDBGStr;
}


uiString::uiString( const uiString& str )
    : data_( str.data_ )
    , datalock_( true )
    , debugstr_( 0 )
{
    refPtr( data_ );
    mSetDBGStr;
}


uiString::~uiString()
{
    unRefAndNullPtr(data_);

    delete [] debugstr_;
}


void uiString::addAlternateVersion( const uiString& altstr )
{
    Threads::Locker datalocker( datalock_ );
    makeIndependent();
    mEnsureData;
    data_->addAlternateVersion( altstr );
}


bool uiString::isEmpty() const
{
    Threads::Locker datalocker( datalock_ );
    if ( !data_ )
	return true;

    Threads::Locker contentlocker( data_->contentlock_ );
    if ( !data_->originalstring_.isEmpty() )
	return false;

#ifndef OD_NO_QT
    return !data_->qstring_.size();
#else
    return true;
#endif
}


void uiString::setEmpty()
{
    unRefAndNullPtr(data_);
    mSetDBGStr;
}


uiString& uiString::toLower( bool yn )
{
    Threads::Locker datalocker( datalock_ );
    makeIndependent();
    mEnsureData;
    Threads::Locker contentlocker( data_->contentlock_ );
    data_->tolower_ = true;
    data_->changecount_ = mForceUpdate;
    mSetDBGStr;

    return *this;
}


uiString& uiString::toTitleCase()
{
    Threads::Locker datalocker( datalock_ );
    makeIndependent();
    mEnsureData;
    Threads::Locker contentlocker( data_->contentlock_ );
    data_->totitlecase_ = true;
    data_->changecount_ = mForceUpdate;
    mSetDBGStr;
    return *this;
}


const char* uiString::getOriginalString() const
{
    Threads::Locker datalocker( datalock_ );
    if ( !data_ )
	return 0;

    Threads::Locker contentlocker( data_->contentlock_ );
    /* This is safe because if anyone else changes originalstring,
       it should be made independent, and we can live with our
       own copy. */
    return data_->originalstring_;
}


const OD::String& uiString::getFullString( BufferString* res ) const
{
    if ( !res )
    {
	mDeclStaticString( staticres );
	res = &staticres;
    }

    Threads::Locker datalocker( datalock_ );
    if ( !data_ )
	res->setEmpty();
    else
	data_->getFullString( *res );
    return *res;
}


bool uiString::isCacheValid() const
{
#ifndef OD_NO_QT
    const int curchange = TrMgr().changeCount();
    if ( !data_ || data_->changecount_!=curchange || data_->qstring_.isEmpty() )
	return false;

    for ( int idx=0; idx<data_->arguments_.size(); idx++ )
    {
	if ( !data_->arguments_.get(idx).isCacheValid() )
	    return false;
    }
    return true;
#else
    return false;
#endif
}


const QString& uiString::fillQString( QString& res ) const
{
    if ( !data_ )
	res.clear();
    else
    {
	Threads::Locker datalocker( datalock_ );
	Threads::Locker contentlocker( data_->contentlock_ );
	res = getQStringInternal();
    }
    return res;
}


void uiString::fillUTF8String( BufferString& res ) const
{
    QString qres;
    fillQString( qres );
    res.set( qres );
}


const QString& uiString::getQStringInternal() const
{
#ifndef OD_NO_QT
    mEnsureData;
    if ( !isCacheValid() )
    {
	data_->fillQString( data_->qstring_, 0, false );
	data_->changecount_ = TrMgr().changeCount();
    }

    /* This is safe as if anyone else changes any of the inputs to qstring,
       it should be made independent. */

    return data_->qstring_;
#else
    QString* ptr = 0;
    return *ptr;
#endif
}


wchar_t* uiString::createWCharString() const
{
#ifndef OD_NO_QT
    Threads::Locker datalocker( datalock_ );
    if ( !data_ )
	return 0;

    Threads::Locker contentlocker( data_->contentlock_ );

    const QString qstr = getQStringInternal();

    contentlocker.unlockNow();
    datalocker.unlockNow();

    mDeclareAndTryAlloc( wchar_t*, res, wchar_t[qstr.size()+1] );

    if ( !res )
	return 0;

    const int nrchars = qstr.toWCharArray( res );
    res[nrchars] = 0;

    return res;
#else
    return 0;
#endif
}


uiString& uiString::operator=( const uiString& str )
{
    if ( &str == this )
	return *this;

    Threads::Locker datalocker( datalock_ );
    refPtr( str.data_ );
    unRefAndNullPtr( data_ );
    data_ = str.data_;
    mSetDBGStr;
    return *this;
}


void uiString::setFrom( const QString& qstr )
{
    Threads::Locker datalocker( datalock_ );
    makeIndependent();
    mEnsureData;
    Threads::Locker contentlocker( data_->contentlock_ );
    data_->setFrom( qstr );
    mSetDBGStr;
}


uiString& uiString::set( const char* str )
{
    Threads::Locker datalocker( datalock_ );
    if ( str && *str )
    {
	makeIndependent();
	mEnsureData;
	Threads::Locker contentlocker( data_->contentlock_ );
	data_->set( str );
    }
    else
    {
	unRefAndNullPtr( data_ );
    }
    mSetDBGStr;
    return *this;
}


bool uiString::operator>( const uiString& oth ) const
{
#ifndef OD_NO_QT
    mGetQStr( myqs, *this );
    mGetQStr( othqs, oth );
    return myqs > othqs;
#else
    return true;
#endif
}


bool uiString::operator<( const uiString& oth ) const
{
#ifndef OD_NO_QT
    mGetQStr( myqs, *this );
    mGetQStr( othqs, oth );
    return myqs < othqs;
#else
    return false;
#endif
}


bool uiString::isEqualTo( const uiString& oth ) const
{
    if ( this == &oth )
	return true;

    mGetQStr( myqs, *this );
    mGetQStr( othqs, oth );
    return myqs == othqs;
}


bool uiString::isPlainAscii() const
{
#ifdef OD_NO_QT
    return true;
#else
    mGetQStr( qstr, *this );
    return !qstr.contains( QRegularExpression(
		QStringLiteral( "[^\\x{0000}-\\x{007F}]") ) );
#endif
}


void uiString::encodeStorageString( BufferString& ret ) const
{
    BufferString hexenc; getHexEncoded( hexenc );
    const int sz = hexenc.size();
    ret.set( mStoreduiStringPreamble ).add( sz ).add( ':' ).add( hexenc );
}


int uiString::useEncodedStorageString( const char* inpstr )
{
    BufferString bufstr( inpstr );
    int len = bufstr.size();
    char* str = bufstr.getCStr();
    const char* preamb = mStoreduiStringPreamble;
    if ( len < 4 || *str != *preamb || *(str+1) != *(preamb+1) )
	return -1;

    char* ptrhex = firstOcc( str+2, ':' );
    if ( !ptrhex )
	return -1;
    else
	*ptrhex++ = '\0';

    len = toInt( str+2 );
    if ( len < 1 )
	{ setEmpty(); return ptrhex-str; }

    BufferString hexstr( len+1, true );
    char* hexstrptr = hexstr.getCStr();
    for ( int idx=0; idx<len; idx++ )
    {
	if ( !*ptrhex )
	    return -1;

	*hexstrptr = *ptrhex;
	hexstrptr++; ptrhex++;
    }

    return setFromHexEncoded( hexstr ) ? ptrhex-str : -1;
}


uiString& uiString::arg( const uiString& newarg )
{
    Threads::Locker datalocker( datalock_ );
    makeIndependent();
    mEnsureData;
    Threads::Locker contentlocker( data_->contentlock_ );
    data_->arguments_ += newarg;
    data_->changecount_ = mForceUpdate;
    mSetDBGStr;
    return *this;
}


uiString& uiString::arg( float f, int nrdecimals )
{
    return arg( toUiString(f,0,'f',nrdecimals) );
}


uiString& uiString::arg( double d, int nrdecimals )
{
    return arg( toUiString(d,0,'f',nrdecimals) );
}


uiString& uiString::setArg( int nr, const uiString& newarg )
{
    Threads::Locker datalocker( datalock_ );
    makeIndependent();
    mEnsureData;
    Threads::Locker contentlocker( data_->contentlock_ );
    if ( data_->arguments_.validIdx(nr) )
    {
	data_->arguments_.get(nr) = newarg;
	data_->changecount_ = mForceUpdate;
    }
    mSetDBGStr;
    return *this;
}


uiString& uiString::appendPhrase( const uiString& txt,
				  SeparType septyp, AppendType apptyp )
{
    Threads::Locker datalocker( datalock_ );
    uiString self( *this );
    self.makeIndependent();
    mEnsureData;

    //To keep it alive if it is replaced in the operator=
    RefMan<uiStringData> tmpptr = data_;
    Threads::Locker contentlocker( tmpptr->contentlock_ );

    if ( isEmpty() )
	{ septyp = NoSep; apptyp = OnSameLine; }

    const char* tplstr = 0;
    if ( apptyp == OnNewLine )
    {
	switch ( septyp )
	{
	    case NoSep:
	    case Space:
	    case Tab:		tplstr = "%1\n%2";	break;
	    case CloseLine:	tplstr = "%1.\n%2";	break;
	    case Comma:		tplstr = "%1,\n%2";	break;
	    case MoreInfo:	tplstr = "%1:\n%2";	break;
	    case SemiColon:	tplstr = "%1;\n%2";	break;
	}
    }
    else if ( apptyp == OnSameLine )
    {
	switch ( septyp )
	{
	    case NoSep:		tplstr = "%1%2";	break;
	    case Space:		tplstr = "%1 %2";	break;
	    case Tab:		tplstr = "%1\t%2";	break;
	    case CloseLine:	tplstr = "%1. %2";	break;
	    case Comma:		tplstr = "%1, %2";	break;
	    case MoreInfo:	tplstr = "%1: %2";	break;
	    case SemiColon:	tplstr = "%1; %2";	break;
	}
    }
    else
    {
	switch ( septyp )
	{
	    case NoSep:
	    case Space:
	    case Tab:		tplstr = "%1\n\n%2";	break;
	    case CloseLine:	tplstr = "%1.\n\n%2";	break;
	    case Comma:		tplstr = "%1,\n\n%2";	break;
	    case MoreInfo:	tplstr = "%1:\n\n%2";	break;
	    case SemiColon:	tplstr = "%1;\n\n%2";	break;
	}
    }

    *this = toUiString( tplstr ).arg( self ).arg( txt );

    mSetDBGStr;
    return *this;
}


uiString& uiString::embed( const char* open,const char* close )
{
    Threads::Locker datalocker( datalock_ );
    uiString self( *this );
    self.makeIndependent();
    mEnsureData;

    RefMan<uiStringData> tmpptr = data_;
    Threads::Locker contentlocker( tmpptr->contentlock_ );

    if ( isEmpty() || self.isEmpty() )
	return *this;

    BufferString fmtstr;

    fmtstr.add(open).add("%1").add(close);

    *this = toUiString( fmtstr ).arg( self );

    mSetDBGStr;
    return *this;
}


uiString& uiString::quote( bool single )
{
    const char* qustr = single ? "'" : "\"";
    return embed(qustr,qustr);
}


uiString& uiString::parenthesize()
{
    return embed("(",")");
}


uiString& uiString::optional()
{
    return embed("[","]");
}


uiString& uiString::embedFinalState()
{
    return embed("<",">");
}


uiString& uiString::appendPhrases( const uiStringSet& strs,
				   SeparType septyp, AppendType apptyp )
{
    return appendPhrase( strs.cat(septyp,apptyp), septyp, apptyp );
}


uiString& uiString::appendPlainText( const OD::String& odstr, bool addspace,
				     bool addquotes )
{
    return appendPlainText( odstr.str(), addspace, addquotes );
}


uiString& uiString::appendPlainText( const char* str, bool addspace,
				     bool addquotes )
{
    if ( !addquotes )
	return constructWordWith( toUiString(str), addspace );

    const BufferString toadd( "'", str, "'" );
    return appendPlainText( toadd, addspace, false );
}


uiString& uiString::withUnit( const uiString& unstr )
{
    if ( !unstr.isEmpty() )
	*this = toUiString("%1 (%2)").arg( *this ).arg( unstr );
    return *this;
}


uiString& uiString::withSurvZUnit()
{
    return withUnit( SI().getUiZUnitString(false) );
}


uiString& uiString::withSurvXYUnit()
{
    return withUnit( SI().getUiXYUnitString(true,false) );
}


uiString& uiString::withSurvDepthUnit()
{
    return withUnit( SI().depthsInFeet() ? "ft" : "m" );
}


bool uiString::translate( const QTranslator& qtr , QString& res ) const
{
#ifndef OD_NO_QT
    Threads::Locker datalocker( datalock_ );
    if ( !data_ )
	return false;

    Threads::Locker contentlocker( data_->contentlock_ );
    return data_->fillQString( res, &qtr, false );
#else
    return true;
#endif
}


uiString uiString::getOrderString( int val )
{
    int nr = val;
    if ( nr < 0 )
	nr = -nr;

    if ( nr > 20 )
	nr = nr % 10;

    uiString rets[] = { tr("th", "zeroth"),
			tr("st", "first"),
			tr("nd", "second"),
			tr("rd", "third" ),
			tr("th", "fourth" ),
			tr("th", "fifth" ),
			tr("th", "sixth" ),
			tr("th", "seventh" ),
			tr("th", "eighth" ),
			tr("th", "ninth"),
			tr("th", "tenth" ),
			tr("th", "eleventh"),
			tr("th", "twelfth"),
			tr("th", "thirteenth"),
			tr("th", "fourteenth"),
			tr("th", "fifteenth"),
			tr("th", "sixteenth"),
			tr("th", "seventeenth"),
			tr("th", "eighteenth"),
			tr("th", "nineteenth"),
			tr("th", "twentieth") };
    return toUiString( "%1%2" ).arg( val ).arg( rets[nr] );
}


void uiString::makeIndependent()
{
    Threads::Locker datalocker( datalock_ );
    if ( !data_ || data_->nrRefs()==1 )
	return;

    RefMan<uiStringData> olddata = data_;
    Threads::Locker contentlocker( olddata->contentlock_ );
    data_->unRef();

    data_ = new uiStringData( 0, 0, 0, 0, -1 );
    data_->ref();

    data_->setFrom( *olddata );
    mSetDBGStr;
}


void uiString::getHexEncoded( BufferString& str ) const
{
#ifndef OD_NO_QT
    mGetQStr( qstr, *this );
    const QString hex( qstr.toUtf8().toHex() );
    str = BufferString( hex );
#endif
}


bool uiString::setFromHexEncoded( const char* str )
{
#ifndef OD_NO_QT
    const QByteArray qba = QByteArray::fromHex( str );
    setFrom( QString::fromUtf8(qba.data(),qba.size()) );
    return true;
#else
    return false;
#endif
}


BufferString uiString::getString() const
{
    BufferString ret;
    getFullString( &ret );
    return ret;
}


uiString toUiString( const char* var ) { return uiString().set( var ); }
uiString toUiString( const uiString& var ) { return var; }

uiString toUiString( const OD::String& str ) { return toUiString( str.str() ); }


static bool useNumberLocale()
{
    static bool ret = GetEnvVarYN( "OD_USE_LOCALE_NUMBERS" );
    return ret;
}

static const char* getNumberLocArg( int idx=1 )
{
    BufferString tmpstr;
    tmpstr.set( "%" );
    if ( useNumberLocale() )
	tmpstr.add( "L" );

    tmpstr.add( idx );
    mDeclStaticString( retstr );
    retstr = tmpstr.str();
    return retstr.str();
}


template <class ODT,class QT> inline
static uiString toUiStringImpl( ODT v, od_uint16 width )
{
#ifdef OD_NO_QT
    return uiString().set( toString(v) );
#else
    const QString ret = QString( getNumberLocArg() ).arg( v, width );
    uiString res;
    res.setFrom( ret );
    return res;
#endif
}


template <class ODT,class QT> inline
static uiString toUiStringWithPrecisionImpl( ODT v, int width, char format,
					     int precision, char fillchar )
{
#ifdef OD_NO_QT
    return uiString().set( toString(v, precision ) );
#else
    const QString ret = QString( getNumberLocArg() ).arg( v, width, format,
							  precision, fillchar );
    uiString res;
    res.setFrom( ret );
    return res;
#endif
}


#ifdef OD_NO_QT
typedef short qint16;
typedef unsigned short quint16;
typedef od_uint32 uint;
typedef od_uint32 qulonglong;
typedef od_int64 ulonglong;
typedef od_uint64 qulonglong;
#endif


uiString toUiString( short v, od_uint16 width )
{ return toUiStringImpl<short,qint16>(v,width); }


uiString toUiString( unsigned short v, od_uint16 width )
{ return toUiStringImpl<unsigned short,quint16>(v,width); }


uiString toUiString( od_int32 v, od_uint16 width )
{ return toUiStringImpl<od_int32,int>(v,width); }


uiString toUiString( od_uint32 v, od_uint16 width )
{ return toUiStringImpl<od_uint32,uint>(v,width); }


uiString toUiString( od_int64 v, od_uint16 width )
{ return toUiStringImpl<od_int64,qlonglong>(v,width); }


uiString toUiString( od_uint64 v, od_uint16 width )
{ return toUiStringImpl<od_uint64,qulonglong>(v,width); }


uiString toUiString( float f, int width, char format, int prec, char fillchar )
{
    return toUiStringWithPrecisionImpl<float,float>( f, width, format,
						     prec, fillchar );
}


uiString toUiStringDec( float f, int nrdec )
{
    const float absval = Math::Abs( f );
    const bool needgeneric = absval < 1e-4f || absval >= 1e6f;
    if ( nrdec <0 && !needgeneric )
        return toUiString( f, 0, 'd', 0 );

    const char specifier = needgeneric ? 'g' : 'f';
    const int precision = needgeneric ? nrdec <=0 ? 1 : nrdec+1 : nrdec;
    return toUiString( f, 0, specifier, precision );
}


uiString toUiString( double d, int width, char format, int prec, char fillchar )
{
    return toUiStringWithPrecisionImpl<double,double>( d, width, format,
						       prec, fillchar );
}


uiString toUiStringDec( double d, int nrdec )
{
    const float absval = Math::Abs( d );
    const bool needgeneric = absval < 1e-4f || absval >= 1e6f;
    if ( nrdec <0 && !needgeneric )
        return toUiString( d, 0, 'd', 0 );

    const char specifier = needgeneric ? 'g' : 'f';
    const int precision = needgeneric ? nrdec <=0 ? 1 : nrdec+1 : nrdec;
    return toUiString( d, 0, specifier, precision );
}


uiString toUiString( const Coord& crd, int precision, int width, char format,
		     bool withparenthesis )
{
    const char fmt = precision <= 0 ? 'd' : format;
    const int prec = precision <= 0 ? 0 : precision;
    BufferString qfmt( getNumberLocArg(1) );
    qfmt.add( ", " ).add( getNumberLocArg(2) );
    if ( withparenthesis )
	qfmt.embed('(',')');

    QString qret = QString( qfmt.str() ).arg( crd.x_, width, fmt, prec )
					.arg( crd.y_, width, fmt, prec );

    uiString res;
    res.setFrom( qret );
    return res;
}


uiString toUiString( const Coord3& crd, int xyprecision, int zprecision,
		     int xywidth, int zwidth, char format, bool withparenthesis)
{
    const char xyfmt = xyprecision <= 0 ? 'd' : format;
    const char zfmt = zprecision <= 0 ? 'd' : format;
    const int xyprec = xyprecision <= 0 ? 0 : xyprecision;
    const int zprec = zprecision <= 0 ? 0 : zprecision;
    BufferString qfmt( getNumberLocArg(1) );
    qfmt.add( ", " ).add( getNumberLocArg(2) )
        .add( ", " ).add( getNumberLocArg(3) );
    if ( withparenthesis )
	qfmt.embed('(',')');

    QString qret = QString( qfmt.str() ).arg( crd.x_, xywidth, xyfmt, xyprec )
					.arg( crd.y_, xywidth, xyfmt, xyprec )
					.arg( crd.z_, zwidth, zfmt, zprec );

    uiString res;
    res.setFrom( qret );
    return res;
}

uiString toUiString( const BufferStringSet& bss )
{
    return toUiString( bss.getDispString(-1,false) );
}


uiString toUiString( const MultiID& key )
{
    return toUiString( key.toString() );
}


uiString od_static_tr( const char* func, const char* text,
		       const char* disambiguation, int pluralnr )
{
    const BufferString context( "static_func_", func );
    return uiString( text, context.buf(),
		     uiString::sODLocalizationApplication(),
		     disambiguation, pluralnr );
}


int uiString::size() const
{
    mGetQStr( qstr, *this );
    return qstr.size();
}


uiString toUiString( const QString& qs )
{
    uiString ret;
    ret.setFrom( qs );
    return ret;
}


uiString toUiString( float f, char format, int precision )
{
    return toUiString( f, 0, format, precision );
}


uiString toUiString( double d, char format, int precision )
{
    return toUiString( d, 0, format, precision );
}


const char* toString( const uiString& uis )
{
    mDeclStaticString( retstr );
    retstr = uis.getString();
    return retstr.getCStr();
}


uiString getUiYesNoString( bool res )
{
    return res ? uiStrings::sYes() : uiStrings::sNo();
}


const QString& uiString::getQString() const
{
    Threads::Locker datalocker( datalock_ );
    if ( !data_ )
    {
#ifndef OD_NO_QT
	return *emptyqstring;
#else
	QString* ptr = 0;
	return *ptr;
#endif
    }

    Threads::Locker contentlocker( data_->contentlock_ );

    return getQStringInternal();
}


bool hasUnicodeCharacters( const QString& qstr )
{
    for ( auto qchar : qstr )
    {
	if ( qchar.unicode() > 127 )
	    return true;
    }

    return false;
}
