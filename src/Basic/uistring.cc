/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	K. Tingdahl
 Date:		Jan 2014
________________________________________________________________________

-*/

#include "usernamestring.h"

#include "bufstring.h"
#include "envvars.h"
#include "geometry.h"
#include "keystrs.h"
#include "od_iostream.h"
#include "odmemory.h"
#include "staticstring.h"
#include "ptrman.h"
#include "refcount.h"
#include "texttranslator.h"
#include "typeset.h"
#include "uistrings.h"

#ifndef OD_NO_QT
# include <QString>
# include <QStringList>
# include <QTranslator>
# include <QLocale>
# include <QRegularExpression>
#endif

#define mForceUpdate (-1)

#ifndef __debug__

# define mSetDBGStr /* nothing */

#else

static char* getNewDebugStr( char* strvar, const OD::String& newstr )
{
    delete [] strvar;
    const od_int64 newsz = newstr.size();
    strvar = new char [ newsz + 1 ];
    OD::sysMemCopy( strvar, newstr.str(), newsz );
    strvar[newsz] = '\0';
    return strvar;
}

# define mSetDBGStr debugstr_ = getNewDebugStr( debugstr_, getFullString() )

#endif

const uiString uiString::emptystring_( toUiString("") );

#ifndef OD_NO_QT
static const QString emptyqstring;
#endif

class uiStringData : public RefCount::Referenced
{
  friend class uiString;
public:
    uiStringData( const char* originalstring, const char* context,
		  const char* application,
		  const char* disambiguation, int pluralnr )
	: originalstring_( originalstring )
	, translationcontext_( context )
	, translationpluralnumber_( pluralnr )
	, translationdisambiguation_( disambiguation )
	, application_( application )
	, contentlock_( true )
	, changecount_( mForceUpdate )
	, tolower_( false )
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
	application_ = d.application_;
	changecount_ = mForceUpdate;
	tolower_ = d.tolower_;
    }

    void addLegacyVersion( const uiString& legacy )
    {
	Threads::Locker contentlocker( contentlock_ );
        legacyversions_.add( legacy );
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

    void getFullString( BufferString& ) const;

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
    uiStringSet			legacyversions_;
    BufferString		translationcontext_;
    const char*			application_;
    const char*			translationdisambiguation_;
    int				translationpluralnumber_;
    bool			tolower_;

    int				changecount_;
};


void uiStringData::set( const char* orig )
{
    Threads::Locker contentlocker( contentlock_ );
    originalstring_ = orig;
    arguments_.setEmpty();
    legacyversions_.setEmpty();
    translationcontext_.setEmpty();
    application_ = 0;
    translationdisambiguation_ = 0;
    translationpluralnumber_ = -1;
    changecount_ = mForceUpdate;
    tolower_ = false;
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
}



bool uiStringData::fillQString( QString& res,
				const QTranslator* translator,
				bool notranslation) const
{
#ifndef OD_NO_QT
    Threads::Locker contentlocker( contentlock_ );
    if ( !originalstring_ || !*originalstring_ )
    {
        //res = qstring_;
        return false;
    }

    bool translationres = false;

    const QTranslator* usedtrans = translator;

    if ( !notranslation && !usedtrans )
	usedtrans = TrMgr().getQTranslator( application_ );

    if ( !notranslation && usedtrans && !translationcontext_.isEmpty() )
    {
	res = usedtrans->translate( translationcontext_, originalstring_,
				     translationdisambiguation_,
				     translationpluralnumber_ );

	if ( res.size() && QString(originalstring_.buf())!=res )
            translationres = true;

        if ( legacyversions_.size() && !translationres )
        {
            for ( int idx=0; idx<legacyversions_.size(); idx++ )
            {
                QString legacytrans;
		if ( legacyversions_[idx].translate( *usedtrans, legacytrans) )
                {
                    res = legacytrans;
                    translationres = true;
                    break;
                }
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
	res = originalstring_;
    }

    if ( res.isEmpty() )
    {
	res = originalstring_;
    }

    if ( tolower_ )
	res = res.toLower();

    for ( int idx=0; idx<arguments_.size(); idx++ )
    {
	QString thearg;
	if ( notranslation )
	    thearg = arguments_[idx].getFullString().buf();
	else if ( translator )
	    arguments_[idx].translate( *translator, thearg );
	else
	    thearg = arguments_[idx].getQString();

	res = res.arg( thearg );
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
{}


uiString::uiString( const char* originaltext, const char* context,
		    const char* application,
		    const char* disambiguation, int pluralnr )
    : data_( new uiStringData(originaltext, context, application,
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
    unRefAndZeroPtr(data_);

    delete [] debugstr_;
}


void uiString::addLegacyVersion( const uiString& legacy )
{
    Threads::Locker datalocker( datalock_ );
    makeIndependent();
    mEnsureData;
    data_->addLegacyVersion( legacy );
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
    unRefAndZeroPtr(data_);
    mSetDBGStr;
}


uiString& uiString::toLower(bool yn)
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
	if ( !data_->arguments_[idx].isCacheValid() )
	    return false;
    }
    return true;
#else
    return false;
#endif
}


const QString& uiString::getQString() const
{
    Threads::Locker datalocker( datalock_ );
    if ( !data_ )
    {
#ifndef OD_NO_QT
	return emptyqstring;
#else
	QString* ptr = 0;
	return *ptr;
#endif
    }

    Threads::Locker contentlocker( data_->contentlock_ );

    return getQStringInternal();
}


const QString& uiString::fillQString( QString& res ) const
{
    Threads::Locker datalocker( datalock_ );
    Threads::Locker contentlocker( data_->contentlock_ );

    res = getQStringInternal();
    return res;
}


const BufferString& uiString::fillUTF8String( BufferString& res ) const
{
    QString qres;
    fillQString( qres );
    res.set( qres );
    return res;
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
    unRefAndZeroPtr( data_ );
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
	unRefAndZeroPtr( data_ );
    }
    mSetDBGStr;
    return *this;
}


bool uiString::operator>(const uiString& b ) const
{
#ifndef OD_NO_QT
    Threads::Locker datalocker( datalock_ );
    if ( !data_ )
	return false;

    const QString& aqstr = getQString();
    const QString& bqstr = b.getQString();
    return aqstr > bqstr;
#else
    return true;
#endif
}


bool uiString::operator<(const uiString& b ) const
{
#ifndef OD_NO_QT
    Threads::Locker datalocker( datalock_ );
    if ( !data_ )
	return true;

    const QString& aqstr = getQString();
    const QString& bqstr = b.getQString();
    return aqstr < bqstr;
#else
    return true;
#endif
}


bool uiString::isEqualTo( const uiString& oth ) const
{
    Threads::Locker datalocker( datalock_ );
    if ( data_ == oth.data_ )
	return true;

    const BufferString myfullstring = getFullString();
    return myfullstring == oth.getFullString();
}


bool uiString::isPlainAscii() const
{
#ifdef OD_NO_QT
    return true;
#else
    const QString qstr = getQString();
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


uiString& uiString::appendPhrase( const uiString& txt, AppendType apptyp )
{
    Threads::Locker datalocker( datalock_ );
    uiString self( *this );
    self.makeIndependent();
    mEnsureData;

    //To keep it alive if it is replaced in the operator=
    RefMan<uiStringData> tmpptr = data_;
    Threads::Locker contentlocker( tmpptr->contentlock_ );

    if ( isEmpty() || txt.isEmpty() )
	apptyp = BluntGlue;

    const char* tplstr = 0;
    switch ( apptyp )
    {
	case BluntGlue:		tplstr = "%1%2";	break;
	case WithSpace:		tplstr = "%1 %2";	break;
	case NewLine:		tplstr = "%1\n%2";	break;
	case CloseLine:		tplstr = "%1. %2";	break;
	case CloseAndNewLine:	tplstr = "%1.\n%2";	break;
    }
    *this = toUiString( tplstr ).arg( self ).arg( txt );

    mSetDBGStr;
    return *this;
}


uiString& uiString::appendPhrases( const uiStringSet& strs, AppendType apptyp )
{
    return appendPhrase( strs.cat(apptyp), apptyp );
}


uiString& uiString::appendPlainText( const OD::String& a, AppendType apptyp )
{
    return appendPlainText( a.str(), apptyp );
}


uiString& uiString::appendPlainText( const char* newarg, AppendType apptyp )
{
    return appendPhrase( toUiString(newarg), apptyp );
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
    if ( nr < 0 ) nr = -nr;

    if ( nr > 20 )
	nr = nr % 10;

    uiString rets[] = { tr("th", "zeroth"), //0
			tr("st", "first"), //1
			tr("nd", "second"), //2
			tr("rd", "third" ), //3
			tr("th", "fourth" ), //4
			tr("th", "fifth" ), //5
			tr("th", "sixth" ), //6
			tr("th", "seventh" ), //7
			tr("th", "eighth" ), //8
			tr("th", "ninth"), //9
			tr("th", "tenth" ), //10
			tr("th", "eleventh"), //11
			tr("th", "twelfth"), //12
			tr("th", "thirteenth"), //13
			tr("th", "fourteenth"), //14
			tr("th", "fifteenth"), //15
			tr("th", "sixteenth"), //16
			tr("th", "seventeenth"), //17
			tr("th", "eighteenth"), //18
			tr("th", "nineteenth"), //19
			tr("th", "twentieth") }; //20
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
    const QString qstr = getQString();
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


uiString toUiString( const char* var ) { return uiString().set( var ); }
uiString toUiString( const uiString& var ) { return var; }

uiString toUiString( const OD::String& str ) { return toUiString( str.str() ); }


template <class ODT,class QT> inline
static uiString toUiStringWithPrecisionImpl( ODT v, int prec )
{
#ifndef OD_NO_QT
    const QLocale* locale = TrMgr().getQLocale();
    if ( !locale )
	{ pFreeFnErrMsg("No locale available"); }
    else
    {
        uiString res;
        res.setFrom( locale->toString( (QT)v, 'g', prec ) );
        return res;
    }
#endif
    return uiString().set( toString(v) );
}


template <class ODT,class QT> inline
static uiString toUiStringImpl( ODT v )
{
#ifndef OD_NO_QT
    const QLocale* locale = TrMgr().getQLocale();
    if ( !locale )
	{ pFreeFnErrMsg("No locale available"); }
    else
    {
        uiString res;
        res.setFrom( locale->toString( (QT)v ) );
        return res;
    }
#endif

    return uiString().set( toString(v) );
}

#ifdef OD_NO_QT
typedef od_uint32 uint;
typedef od_int64 ulonglong;
typedef od_uint32 qulonglong;
#endif


uiString toUiString( od_int32 v )
{ return toUiStringImpl<od_int32,int>( v ); }


uiString toUiString( od_uint32 v )
{ return toUiStringImpl<od_int32,uint>( v ); }


uiString toUiString( od_int64 v )
{ return toUiStringImpl<od_int32,qlonglong>( v ); }


uiString toUiString( od_uint64 v )
{ return toUiStringImpl<od_uint64,qulonglong>( v ); }


uiString toUiString( float v )
{ return toUiStringImpl<float,float>( v ); }


uiString toUiString( double v )
{ return toUiStringImpl<double,double>( v ); }


uiString toUiString( float v, int prec )
{ return toUiStringWithPrecisionImpl<float,float>( v, prec ); }


uiString toUiString( double v, int prec )
{ return toUiStringWithPrecisionImpl<double,double>( v, prec ); }

uiString toUiString( const Coord& c )
{
    return toUiString( "(%1,%2)" ).arg( mRounded(od_int64,c.x_) )
				  .arg( mRounded(od_int64,c.y_) );
}

const char* toString( const uiString& uistr )
{
    mDeclStaticString( ret );
    ret.set( uistr.getFullString() );
    return ret.str();
}


uiString od_static_tr( const char* func, const char* text,
		       const char* disambiguation, int pluralnr )
{
    const BufferString context( "static_func_", func );
    return uiString( text, context.buf(),
		     uiString::sODLocalizationApplication(),
		     disambiguation, pluralnr );
}


uiWord getUiYesNoWord( bool res )
{
    return res ? uiStrings::sYes() : uiStrings::sNo();
}


int uiString::size() const
{
    return getQString().size();
}


//-- uiStringSet


uiStringSet::uiStringSet( const uiString strs[] )
{
    for ( IdxType idx=0; ; idx++ )
    {
	const uiString& str = strs[idx];
	if ( str.isEmpty() )
	    break;
	add( str );
    }
}


uiStringSet::~uiStringSet()
{
    setEmpty();
}


uiStringSet& uiStringSet::operator =( const uiStringSet& oth )
{
    deepCopy( strs_, oth.strs_ );
    return *this;
}


bool uiStringSet::isPresent( const uiString& str ) const
{
    return indexOf( str ) >= 0;
}


uiStringSet::IdxType uiStringSet::indexOf( const uiString& str ) const
{
    const size_type sz = size();
    for ( IdxType idx=0; idx<sz; idx++ )
	if ( strs_[idx]->isEqualTo(str) )
	    return idx;
    return -1;
}


uiString uiStringSet::get( IdxType idx ) const
{
    return strs_.validIdx(idx) ? *strs_[idx] : uiString::emptyString();
}


void uiStringSet::setEmpty()
{
    deepErase( strs_ );
}


uiStringSet& uiStringSet::set( const uiString& str )
{
    setEmpty();
    return add( str );
}


uiStringSet& uiStringSet::set( const uiRetVal& uirv )
{
    return set( (const uiStringSet&)uirv );
}


uiStringSet& uiStringSet::add( const uiString& str )
{
    strs_ += new uiString( str );
    return *this;
}


uiStringSet& uiStringSet::add( const uiStringSet& oth )
{
    deepAppend( strs_, oth.strs_ );
    return *this;
}


uiStringSet& uiStringSet::add( const uiRetVal& uirv )
{
    return add( (const uiStringSet&)uirv );
}


uiStringSet& uiStringSet::insert( IdxType idx, const uiString& str )
{
    strs_.insertAt( new uiString(str), idx );
    return *this;
}


void uiStringSet::removeSingle( IdxType idx, bool kporder )
{
    delete strs_.removeSingle( idx, kporder );
}


void uiStringSet::fill( QStringList& qlist ) const
{
    for ( IdxType idx=0; idx<size(); idx++ )
	qlist.append( strs_[idx]->getQString() );
}


uiStringSet uiStringSet::getNonEmpty() const
{
    uiStringSet ret;
    for ( IdxType idx=0; idx<size(); idx++ )
    {
	const uiString& str = *strs_[idx];
	if ( !str.isEmpty() )
	    ret.add( str );
    }
    return ret;
}



uiString uiStringSet::createOptionString( bool use_and, size_type maxnr,
					  bool separate_lines ) const
{
    const uiStringSet usestrs( getNonEmpty() );
    const size_type sz = usestrs.size();
    if ( sz < 1 )
	return uiString::emptyString();

    uiString result( usestrs[0] );
    if ( sz < 2 || maxnr == 1 )
	return result;

    const uiString and_or_or = use_and ? uiStrings::sAnd() : uiStrings::sOr();
    const uiString::AppendType apptyp = separate_lines	? uiString::NewLine
							: uiString::WithSpace;

    if ( maxnr < 1 || maxnr > sz )
	maxnr = sz;

    const uiString possiblecomma = separate_lines ? uiString::emptyString()
						  : toUiString( "," );
    for ( IdxType idx=1; idx<maxnr; idx++ )
    {
	if ( idx < sz-1 )
	    result.appendPhrase( possiblecomma, uiString::BluntGlue );
	else
	    result.appendPhrase( and_or_or, uiString::WithSpace );
	result.appendPhrase( usestrs.get(idx), apptyp );
    }

    if ( sz > maxnr )
	result.appendPhrase( and_or_or, apptyp )
	      .appendPhrase( toUiString( "..." ), uiString::WithSpace );

    return result;
}


uiString uiStringSet::cat( uiString::AppendType apptyp ) const
{
    uiString result;
    for ( IdxType idx=0; idx<size(); idx++ )
	result.appendPhrase( *strs_[idx], apptyp );
    return result;
}


void uiStringSet::sort( const bool caseinsens, bool asc )
{
    size_type* idxs = getSortIndexes( caseinsens, asc );
    useIndexes( idxs );
    delete [] idxs;
}


void uiStringSet::useIndexes( const size_type* idxs )
{
    const size_type sz = size();
    if ( !idxs || sz < 2 )
	return;

    ObjectSet<uiString> tmp;
    for ( size_type idx=0; idx<sz; idx++ )
	tmp.add( strs_[idx] );

    strs_.plainErase();

    for ( size_type idx=0; idx<sz; idx++ )
	strs_.add( tmp[ idxs[idx] ] );
}


uiStringSet::IdxType* uiStringSet::getSortIndexes( bool caseinsens,
							    bool asc ) const
{
    const size_type sz = size();
    if ( sz < 1 )
	return 0;
    mGetIdxArr( size_type, idxs, sz );
    Qt::CaseSensitivity cs(Qt::CaseSensitive);
    if ( caseinsens )
	cs = Qt::CaseInsensitive;

    const uiStringSet* strset = this;
    for ( size_type d=sz/2; d>0; d=d/2 )
	for ( size_type i=d; i<sz; i++ )
	    for ( size_type j=i-d; j>=0 &&
		  (QString::compare( strset->get(idxs[j]).getQString(),
		  strset->get(idxs[j+d]).getQString(), cs ) > 0 ); j-=d )
		Swap( idxs[j+d], idxs[j] );

    if ( !asc )
    {
	const size_type hsz = sz/2;
	for ( size_type idx=0; idx<hsz; idx++ )
	    Swap( idxs[idx], idxs[sz-idx-1] );
    }

    return idxs;
}


static const UserNameString emptyusrnmstring( uiString::emptyString() );
const UserNameString& UserNameString::empty() { return emptyusrnmstring; }

UserNameString& UserNameString::join( const uiString& s, bool before )
{
    uiString res = toUiString( "%1 %2" );
    if ( before )
	res.arg( s ).arg( impl_ );
    else
	res.arg( impl_ ).arg( s );
    return *this = res;
}


const uiRetVal uiRetVal::ok_;

uiRetVal::uiRetVal( const uiPhrase& str )
{
    msgs_.add( str );
}


uiRetVal::uiRetVal( const uiPhraseSet& strs )
    : msgs_(strs)
{
}


uiRetVal::uiRetVal( const uiRetVal& oth )
    : msgs_(oth.msgs_)
{
}

uiRetVal& uiRetVal::operator =( const uiRetVal& oth )
{
    return set( oth );
}


uiRetVal& uiRetVal::operator =( const uiPhrase& str )
{
    return set( str );
}


uiRetVal& uiRetVal::operator =( const uiPhraseSet& strs )
{
    return set( strs );
}


uiRetVal::operator uiPhrase() const
{
    Threads::Locker locker( lock_ );
    return msgs_.isEmpty() ? uiPhrase::emptyString() : msgs_.cat();
}


uiRetVal::operator uiPhraseSet() const
{
    Threads::Locker locker( lock_ );
    return msgs_;
}


bool uiRetVal::isOK() const
{
    Threads::Locker locker( lock_ );
    return msgs_.isEmpty() || msgs_[0].isEmpty();
}


bool uiRetVal::isMultiMessage() const
{
    Threads::Locker locker( lock_ );
    return msgs_.size() > 1;
}


uiPhraseSet uiRetVal::messages() const
{
    Threads::Locker locker( lock_ );
    return msgs_;
}


bool uiRetVal::isSingleWord( const uiWord& str ) const
{
    Threads::Locker locker( lock_ );
    return msgs_.size() == 1 && msgs_[0].isEqualTo( str );
}


uiRetVal& uiRetVal::set( const uiRetVal& oth )
{
    if ( this != &oth )
    {
	Threads::Locker locker( lock_ );
	msgs_ = oth.msgs_;
    }
    return *this;
}


uiRetVal& uiRetVal::set( const uiPhrase& str )
{
    Threads::Locker locker( lock_ );
    msgs_.setEmpty();
    msgs_.add( str );
    return *this;
}


uiRetVal& uiRetVal::set( const uiPhraseSet& strs )
{
    Threads::Locker locker( lock_ );
    msgs_ = strs;
    return *this;
}


uiRetVal& uiRetVal::setEmpty()
{
    Threads::Locker locker( lock_ );
    msgs_.setEmpty();
    return *this;
}


uiRetVal& uiRetVal::insert( const uiPhrase& str )
{
    Threads::Locker locker( lock_ );
    msgs_.insert( 0, str );
    return *this;
}


uiRetVal& uiRetVal::add( const uiRetVal& oth )
{
    if ( this != &oth )
    {
	Threads::Locker locker( lock_ );
	msgs_.append( oth.msgs_ );
    }
    return *this;
}


uiRetVal& uiRetVal::add( const uiPhrase& str )
{
    if ( !str.isEmpty() )
    {
	Threads::Locker locker( lock_ );
	msgs_.add( str );
    }
    return *this;
}


uiRetVal& uiRetVal::setAsStatus( const uiWord& word )
{
    if ( !word.isEmpty() )
    {
	Threads::Locker locker( lock_ );
	msgs_.add( word );
    }
    return *this;
}


uiRetVal& uiRetVal::add( const uiPhraseSet& strs )
{
    for ( int idx=0; idx<strs.size(); idx++ )
	add( strs[idx] );
    return *this;
}


BufferString uiRetVal::getText() const
{
    uiString uistr;
    if ( !isMultiMessage() )
	uistr = *this;
    else
    {
	Threads::Locker locker( lock_ );
	uistr = msgs_.cat();
    }

    return BufferString( uistr.getFullString() );
}


bool isFinished( const uiRetVal& uirv )
{
    return uirv.isSingleWord( uiStrings::sFinished() );
}


bool isCancelled( const uiRetVal& uirv )
{
    return uirv.isSingleWord( uiStrings::sCancelled() );
}
