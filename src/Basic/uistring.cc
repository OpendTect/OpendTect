/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	K. Tingdahl
 Date:		Jan 2014
________________________________________________________________________

-*/
static const char* rcsID mUsedVar = "$Id$";

#include "uistring.h"

#include "bufstring.h"
#include "envvars.h"
#include "keystrs.h"
#include "od_iostream.h"
#include "perthreadrepos.h"
#include "ptrman.h"
#include "refcount.h"
#include "texttranslator.h"
#include "typeset.h"

#ifndef OD_NO_QT
# include <QString>
# include <QTranslator>
# include <QLocale>
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
    OD::memCopy( strvar, newstr.str(), newsz );
    strvar[newsz] = '\0';
    return strvar;
}

# define mSetDBGStr str_ = getNewDebugStr( str_, getFullString() )

#endif

const uiString uiString::emptystring_( toUiString(sKey::EmptyString()) );

class uiStringData
{ mRefCountImplNoDestructor(uiStringData)
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
    uiStringSet		arguments_;

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
    arguments_.erase();
    legacyversions_.erase();
    translationcontext_.setEmpty();
    application_ = 0;
    translationdisambiguation_ = 0;
    translationpluralnumber_ = -1;
    changecount_ = mForceUpdate;
    tolower_ = false;
#ifndef OD_NO_QT
    qstring_ = sKey::EmptyString().buf();
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


#ifndef __debug__
# define mInitImpl(var,acts) var->ref(); acts
#else
# define mInitImpl(var,acts) str_ = 0; var->ref(); acts; mSetDBGStr
#endif

uiString::uiString()
    : data_( new uiStringData( 0, 0, 0, 0, -1 ) )
    , datalock_( true )
{
    mInitImpl( data_,  );
}

uiString::uiString( const char* str )
    : data_( new uiStringData( 0, 0, 0, 0, -1 ) )
    , datalock_( true )
{
    mInitImpl( data_, set(str) );
}


uiString::uiString( const char* originaltext, const char* context,
		    const char* application,
		    const char* disambiguation, int pluralnr )
    : data_( new uiStringData(originaltext, context, application,
			      disambiguation, pluralnr ))
    , datalock_( true )
{
    mInitImpl( data_, );
}


uiString::uiString( const uiString& str )
    : data_( str.data_ )
    , datalock_( true )
{
    mInitImpl( data_, );
}


uiString::uiString( const OD::String& str )
    : data_( new uiStringData( 0, 0, 0, 0, -1 ) )
    , datalock_( true )
{
    mInitImpl( data_, set(str) );
}


uiString::~uiString()
{
    data_->unRef();
#ifdef __debug__
    delete [] str_;
#endif
}


void uiString::addLegacyVersion( const uiString& legacy )
{
    Threads::Locker datalocker( datalock_ );
    makeIndependent();
    data_->addLegacyVersion( legacy );
}


bool uiString::isEmpty() const
{
    Threads::Locker datalocker( datalock_ );
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
    set( sKey::EmptyString() );
}


uiString& uiString::toLower(bool yn)
{
    Threads::Locker datalocker( datalock_ );
    makeIndependent();
    Threads::Locker contentlocker( data_->contentlock_ );
    data_->tolower_ = true;
    data_->changecount_ = mForceUpdate;
    mSetDBGStr;

    return *this;
}


const char* uiString::getOriginalString() const
{
    Threads::Locker datalocker( datalock_ );
    Threads::Locker contentlocker( data_->contentlock_ );
    /* This is safe because if anyone else changes originalstring,
       it should be made independent, and we can live with our
       own copy. */
    return data_->originalstring_;
}


const OD::String& uiString::getFullString() const
{
    mDeclStaticString( res );
    Threads::Locker datalocker( datalock_ );
    data_->getFullString( res );
    return res;
}


bool uiString::isCacheValid() const
{
    const int curchange = TrMgr().changeCount();
    if ( data_->changecount_!=curchange || data_->qstring_.isEmpty() )
	return false;

    for ( int idx=0; idx<data_->arguments_.size(); idx++ )
    {
	if ( !data_->arguments_[idx].isCacheValid() )
	    return false;
    }

    return true;
}


const QString& uiString::getQString() const
{
    Threads::Locker datalocker( datalock_ );
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


const QString& uiString::getQStringInternal() const
{
#ifndef OD_NO_QT
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
    Threads::Locker datalocker( datalock_ );
    str.data_->ref();
    data_->unRef();
    data_ = str.data_;
    return *this;
}


void uiString::setFrom( const QString& qstr )
{
    Threads::Locker datalocker( datalock_ );
    makeIndependent();
    Threads::Locker contentlocker( data_->contentlock_ );
    data_->setFrom( qstr );
    mSetDBGStr;
}


uiString& uiString::operator=( const OD::String& str )
{
    return set( str.str() );
}


uiString& uiString::operator=( const char* str )
{
    return set( str );
}


uiString& uiString::set( const char* str )
{
    Threads::Locker datalocker( datalock_ );
    makeIndependent();
    Threads::Locker contentlocker( data_->contentlock_ );
    data_->set( str );
    mSetDBGStr;
    return *this;
}


bool uiString::operator>(const uiString& b ) const
{
#ifndef OD_NO_QT
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
    const QString& aqstr = getQString();
    const QString& bqstr = b.getQString();
    return aqstr < bqstr;
#else
    return true;
#endif
}


uiString& uiString::arg( const uiString& newarg )
{
    Threads::Locker datalocker( datalock_ );
    makeIndependent();
    Threads::Locker contentlocker( data_->contentlock_ );
    data_->arguments_ += newarg;
    data_->changecount_ = mForceUpdate;
    mSetDBGStr;
    return *this;
}


uiString& uiString::append( const uiString& txt, bool withnewline )
{
    Threads::Locker datalocker( datalock_ );
    uiString self( *this );
    self.makeIndependent();

    //To keep it alive if it is replaced in the operator=
    RefMan<uiStringData> tmpptr = data_;
    Threads::Locker contentlocker( tmpptr->contentlock_ );


    if ( isEmpty() )
	withnewline = false;

    *this = toUiString( withnewline ? "%1\n%2" : "%1%2" )
		.arg( self ).arg( txt );

    mSetDBGStr;
    return *this;
}


uiString& uiString::append( const OD::String& a, bool withnewline )
{ return append( a.str(), withnewline ); }


uiString& uiString::append( const char* newarg, bool withnewline )
{
    return append( toUiString(newarg), withnewline );
}


bool uiString::translate( const QTranslator& qtr , QString& res ) const
{
#ifndef OD_NO_QT
    Threads::Locker datalocker( datalock_ );
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
    if ( data_->refcount_.count()==1 )
	return;

    RefMan<uiStringData> olddata = data_;
    Threads::Locker contentlocker( olddata->contentlock_ );
    data_->unRef();

    data_ = new uiStringData( 0, 0, 0, 0, -1 );
    data_->ref();

    data_->setFrom( *olddata );
    mSetDBGStr;
}


bool uiString::operator==( const uiString& b ) const
{
#ifdef __debug__
    DBG::forceCrash( false );
    return true;
#else
    Threads::Locker datalocker( datalock_ );
    if ( data_==b.data_ )
	return true;

    const BufferString myself = getFullString();
    return myself == b.getFullString();
#endif
}


uiString uiStringSet::createOptionString( bool use_and,
					  int maxnr, char space ) const
{
    BufferString glue;

    const char* percentage = "%";
    uiStringSet arguments;
    const char spacestring[] = { space, 0 };

    arguments += toUiString(spacestring);
    bool firsttime = true;

    int nritems = 0;
    for ( int idx=0; idx<size(); idx++ )
    {
	if ( (*this)[idx].isEmpty() )
	    continue;

	nritems++;
	arguments += (*this)[idx];
	if ( firsttime )
	{
	    glue.add( percentage );
	    glue.add( arguments.size() );

            firsttime = false;
	}
	else if ( idx==size()-1 )
	{
	    if ( size()==2 )
		glue.add( use_and ? " and%1%" : " or%1%" );
	    else
		glue.add( use_and ? ", and%1%" : ", or%1%");

	    glue.add( arguments.size() );
	}
	else
	{
	    glue.add(",%1%");
	    glue.add( arguments.size() );

	    if ( maxnr>1 && maxnr<=nritems )
	    {
		glue.add( ",%1...");
		break;
	    }
	}
    }

    if ( glue.isEmpty() )
	return uiString();

    uiString res;
    res.set( glue );

    for ( int idx=0; idx<arguments.size(); idx++ )
	res.arg( arguments[idx] );

    return res;
}


void uiString::getHexEncoded( BufferString& str ) const
{
#ifndef OD_NO_QT
    const QString qstr = getQString();
    const QString hex( qstr.toUtf8().toHex() );

    str = BufferString( hex );
#endif
}


bool uiString::isEqualTo( const uiString& oth ) const
{
    const BufferString othbuf = oth.getFullString();
    return othbuf == getFullString();
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

#ifndef OD_NO_QT
#define mToUiStringWithPrecisionImpl( tp, qtp ) \
uiString toUiString(tp v,int prec) \
{ \
    const QLocale* locale = TrMgr().getQLocale(); \
    if ( locale ) \
    { \
        uiString res; \
        res.setFrom( locale->toString((qtp) v, 'g', prec) ); \
        return res; \
    } \
 \
    return uiString().set( toString(v, prec ) ); \
}

#define mToUiStringImpl( tp, qtp ) \
uiString toUiString(tp v) \
{ \
    const QLocale* locale = TrMgr().getQLocale(); \
    if ( locale ) \
    { \
        uiString res; \
        res.setFrom( locale->toString((qtp) v) ); \
        return res; \
    } \
 \
    return uiString().set( toString(v) ); \
}

#else

#define mToUiStringWithPrecisionImpl( tp, qt ) \
uiString toUiString(tp v, int prec ) \
{ \
    return uiString().set( toString(v,prec) ); \
}
#define mToUiStringImpl( tp, qt ) \
uiString toUiString(tp v) \
{ \
    return uiString().set( toString(v) ); \
}
#endif

mToUiStringImpl(od_int32,int)
mToUiStringImpl(od_uint32,uint)
mToUiStringImpl(od_int64,qlonglong)
mToUiStringImpl(od_uint64,qulonglong)
mToUiStringImpl(float,float)
mToUiStringImpl(double,double)
mToUiStringWithPrecisionImpl(float,float)
mToUiStringWithPrecisionImpl(double,double)



uiString od_static_tr( const char* func, const char* text,
		       const char* disambiguation, int pluralnr )
{
    const BufferString context( "static_func_", func );
    return uiString( text, context.buf(),
		     uiString::sODLocalizationApplication(),
		     disambiguation, pluralnr );
}


