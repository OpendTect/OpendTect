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

#include <QString>
#include <QTranslator>


class uiStringData
{ mRefCountImplNoDestructor(uiStringData)
public:
    uiStringData( const char* originalstring, const char* context,
		  const char* application,
		  const char* disambiguation, int pluralnr )
	: originalstring_( originalstring )
	, translationcontext_( context )
	, translationpluralnumber_( pluralnr )
	, translationdisambiguation_( disambiguation )
	, application_( application )
    {
    }

    void setFrom( const uiStringData& d )
    {
	originalstring_ = d.originalstring_;
	translationcontext_ = d.translationcontext_;
	translationpluralnumber_ = d.translationpluralnumber_;
	translationdisambiguation_ = d.translationdisambiguation_;
	arguments_ = d.arguments_;
	application_ = d.application_;
    }

    void addLegacyVersion( const uiString& legacy )
    {
        legacyversions_.add( legacy );
    }

    void setFrom( const QString& qstr )
    {
	originalstring_.setEmpty();
	translationcontext_ = 0;
	translationpluralnumber_ = -1;
	translationdisambiguation_ = 0;
	application_ = 0;
	qstring_ = qstr;
    }

    const BufferString& getFullString() const;

    void set(const char* orig);
    bool fillQString(QString&,const QTranslator* translator=0) const;

    TypeSet<uiString>	arguments_;

    QString		qstring_;

    BufferString	originalstring_;
    TypeSet<uiString>	legacyversions_;
    const char*		translationcontext_;
    const char*		application_;
    const char*		translationdisambiguation_;
    int			translationpluralnumber_;

    int			dirtycount_;
};


void uiStringData::set( const char* orig )
{
    originalstring_ = orig;
}


const BufferString& uiStringData::getFullString() const
{
    if ( !arguments_.size() )
	return originalstring_;

    QString qres;
    fillQString( qres, 0 );

    mDeclStaticString( ret );
    ret = qres;
    return ret;
}



bool uiStringData::fillQString( QString& res,
				const QTranslator* translator ) const
{
    if ( !originalstring_ || !*originalstring_ )
	return false;

    bool translationres = false;

    if ( translator )
    {
	res = translator->translate( translationcontext_, originalstring_,
				     translationdisambiguation_,
				     translationpluralnumber_ );

        if ( QString(originalstring_.buf())!=res )
            translationres = true;

        if ( legacyversions_.size() && !translationres )
        {
            for ( int idx=0; idx<legacyversions_.size(); idx++ )
            {
                QString legacytrans;
                if ( legacyversions_[idx].translate( *translator, legacytrans) )
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

    if ( !translator || res.isEmpty() )
    {
	res = originalstring_;
    }

    for ( int idx=0; idx<arguments_.size(); idx++ )
	res = res.arg( arguments_[idx].getQtString() );

    return translationres;
}


uiString::uiString( const char* str )
    : data_( new uiStringData( 0, 0, 0, 0, -1 ) )
{
    data_->ref();
    *this = str;
}


uiString::uiString( const char* originaltext, const char* context,
		    const char* application,
		    const char* disambiguation, int pluralnr )
    : data_( new uiStringData(originaltext, context, application,
			      disambiguation, pluralnr ))
{
    data_->ref();
}



void uiString::addLegacyVersion( const uiString& legacy )
{
    data_->addLegacyVersion( legacy );
}


uiString::uiString( const uiString& str )
    : data_( str.data_ )
{
    data_->ref();

    *this = str;
}


uiString::uiString( const FixedString& str )
    : data_( new uiStringData( 0, 0, 0, 0, -1 ) )
{
    data_->ref();
    *this = str;
}


uiString::uiString( const BufferString& str )
    : data_( new uiStringData( 0, 0, 0, 0, -1 ) )
{
    data_->ref();
    *this = str;
}


uiString::~uiString()
{
    data_->unRef();
}


bool uiString::isEmpty() const
{
    return data_->originalstring_.isEmpty();
}


void uiString::setEmpty()
{
    *this = sKey::EmptyString();
}


static uiString emptystring;


const uiString& uiString::emptyString()
{
    return emptystring;
}



const char* uiString::getOriginalString() const
{
    return data_->originalstring_;
}


const BufferString& uiString::getFullString() const
{
    return data_->getFullString();
}


const QString& uiString::getQtString() const
{
    data_->fillQString( data_->qstring_,
			TrMgr().getQTranslator(data_->application_) );
    return data_->qstring_;
}


wchar_t* uiString::createWCharString() const
{
    QString qstr;
    data_->fillQString( qstr, TrMgr().getQTranslator(data_->application_) );
    if ( !qstr.size() )
	return 0;

    mDeclareAndTryAlloc( wchar_t*, res, wchar_t[qstr.size()+1] );

    if ( !res )
	return 0;

    const int nrchars = qstr.toWCharArray( res );
    res[nrchars] = 0;

    return res;
}


uiString& uiString::operator=( const uiString& str )
{
    str.data_->ref();
    data_->unRef();
    data_ = str.data_;
    return *this;
}


uiString& uiString::setFrom( const uiString& str )
{
    if ( data_==str.data_ )
    {
	data_->unRef();
	data_ = new uiStringData( 0, 0, 0, 0, -1 );
	data_->ref();
    }

    data_->setFrom( *str.data_ );
    return *this;
}


void uiString::setFrom( const QString& qstr )
{
    data_->setFrom( qstr );
}


uiString& uiString::operator=( const FixedString& str )
{
    data_->set( str.str() );
    return *this;
}


uiString& uiString::operator=( const BufferString& str )
{
    data_->set( str.str() );

    return *this;
}


uiString& uiString::operator=( const char* str )
{
    data_->set( str );
    return *this;
}


uiString& uiString::arg( const uiString& newarg )
{
    data_->arguments_ += newarg;
    return *this;;
}


uiString& uiString::arg( const FixedString& a )
{ return arg( a.str() ); }


uiString& uiString::arg( const BufferString& a )
{ return arg( a.str() ); }


uiString& uiString::arg( const char* newarg )
{
    return arg( uiString(newarg) );
}


uiString& uiString::append( const uiString& txt )
{
    uiStringCopy self( *this );
    *this = uiString("%1%2").arg( self ).arg( txt );
    return *this;;
}


uiString& uiString::append( const FixedString& a )
{ return append( a.str() ); }


uiString& uiString::append( const BufferString& a )
{ return append( a.str() ); }


uiString& uiString::append( const char* newarg )
{
    return append( uiString(newarg) );
}


bool uiString::translate( const QTranslator& tr , QString& res ) const
{
    return data_->fillQString( res, &tr );

}

