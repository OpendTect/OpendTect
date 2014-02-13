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
#include "refcount.h"
#include "ptrman.h"
#include "typeset.h"
#include "perthreadrepos.h"
#include "uitexttranslator.h"
#include "uistrings.h"


#include <QString>
#include <QCoreApplication>


class uiStringData
{ mRefCountImplNoDestructor(uiStringData)
public:
    uiStringData( const char* originalstring, const char* context,
		  const char* disambiguation, int pluralnr )
	: originalstring_( originalstring )
	, translationcontext_( context )
	, translationpluralnumber_( pluralnr )
	, translationdisambiguation_( disambiguation )
    {
    }

    void setFrom( const uiStringData& d )
    {
	originalstring_ = d.originalstring_;
	translationcontext_ = d.translationcontext_;
	translationpluralnumber_ = d.translationpluralnumber_;
	translationdisambiguation_ = d.translationdisambiguation_;
	arguments_ = d.arguments_;
    }

    const char* getFullString() const;

    void set(const char* orig);
    void fillQString(QString&, bool translate) const;

    TypeSet<uiString>		arguments_;

    QString			qstring_;

    BufferString		originalstring_;
    const char* 		translationcontext_;
    const char* 		translationdisambiguation_;
    int 			translationpluralnumber_;

    int 			dirtycount_;
};


void uiStringData::set( const char* orig )
{
    originalstring_ = orig;
}


const char* uiStringData::getFullString() const
{
    QString qres;
    fillQString( qres, false )

    mDeclStaticString( ret );
    ret = qres;
    return ret.buf();
}



void uiStringData::fillQString( QString& res, bool translate ) const
{
    if ( !originalstring_ || !*originalstring_ )
	return;

    res = originalstring_;

    if ( translate )
	res = QCoreApplication::translate( translationcontext_,
					    originalstring_,
					    translationdisambiguation_,
					    QCoreApplication::CodecForTr,
					    translationpluralnumber_ );

    for ( int idx=0; idx<arguments_.size(); idx++ )
    {
	res = res.arg( arguments_[idx].getQtString() );
    }
}



uiString::uiString( const char* str )
    : data_( new uiStringData( 0, 0, 0, -1 ) )
{
    data_->ref();
    *this = str;
}


uiString::uiString( const char* originaltext, const char* context,
		    const char* disambiguation, int pluralnr )
    : data_( new uiStringData(originaltext, context, disambiguation, pluralnr ))
{
    data_->ref();
}


uiString::uiString( const uiString& str )
    : data_( str.data_ )
{
    data_->ref();

    *this = str;
}


uiString::uiString( const FixedString& str )
    : data_( new uiStringData( 0, 0, 0, -1 ) )
{
    data_->ref();
    *this = str;
}


uiString::uiString( const BufferString& str )
    : data_( new uiStringData( 0, 0, 0, -1 ) )
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


const char* uiString::getOriginalString() const
{
    return data_->originalstring_;
}


const char* uiString::getFullString() const
{
    return data_->getFullString();
}


const QString& uiString::getQtString() const
{
    data_->fillQString( data_->qstring_, true );
    return data_->qstring_;
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
	data_ = new uiStringData( 0, 0, 0, -1 );
	data_->ref();
    }

    data_->setFrom( *str.data_ );
    return *this;
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


uiString uiString::arg( const uiString& newarg ) const
{
    uiString ret;
    ret.data_->setFrom( *data_ );
    ret.data_->arguments_ += newarg;

    return ret;
}


uiString uiString::arg( const char* newarg ) const
{
    return arg( uiString(newarg) );
}


mGlobal( uiBase )  TextTranslateMgr& TrMgr()
{
    mDefineStaticLocalObject( PtrMan<TextTranslateMgr>, trmgr, = 0 );
    if ( !trmgr ) trmgr = new TextTranslateMgr();
    return *trmgr;
}

