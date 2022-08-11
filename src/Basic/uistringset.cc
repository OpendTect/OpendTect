/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Bert
 Date:		Oct 2018
________________________________________________________________________

-*/

#include "uistringset.h"
#include "uistrings.h"

#include <QList>
#include <QString>

uiStringSet::uiStringSet( const uiString& s )
{
    set( s );
}


uiStringSet::uiStringSet( const uiString& s1, const uiString& s2 )
{
    set( s1 ).add( s2 );
}


uiStringSet::uiStringSet( const uiString& s1, const uiString& s2,
			  const uiString& s3 )
{
    set( s1 ).add( s2 ).add( s3 );
}


uiStringSet::uiStringSet( const QList<QString>& qsl )
{
    for ( auto qstr : qsl )
	add( toUiString(qstr) );
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


uiStringSet::idx_type uiStringSet::indexOf( const uiString& str ) const
{
    const size_type sz = size();
    for ( idx_type idx=0; idx<sz; idx++ )
	if ( *strs_[idx] == str )
	    return idx;
    return -1;
}


uiString& uiStringSet::get( idx_type idx )
{
    return strs_.validIdx(idx) ? *strs_[idx] : uiString::dummy();
}


const uiString& uiStringSet::get( idx_type idx ) const
{
    return strs_.validIdx(idx) ? *strs_[idx] : uiString::empty();
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


uiStringSet& uiStringSet::addKeyValue( const uiWord& ky, const uiString& val )
{
    uiString toadd( ky );
    toadd.addMoreInfo( val );
    return add( toadd );
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


uiStringSet& uiStringSet::insert( idx_type idx, const uiString& str )
{
    strs_.insertAt( new uiString(str), idx );
    return *this;
}


void uiStringSet::removeSingle( idx_type idx, bool kporder )
{
    delete strs_.removeSingle( idx, kporder );
}


void uiStringSet::fill( QList<QString>& qlist ) const
{
    QString qstr;
    for ( idx_type idx=0; idx<size(); idx++ )
    {
	strs_[idx]->fillQString( qstr );
	qlist.append( qstr );
    }
}


uiStringSet uiStringSet::getNonEmpty() const
{
    uiStringSet ret;
    for ( idx_type idx=0; idx<size(); idx++ )
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
	return uiString::empty();

    uiString result( usestrs[0] );
    if ( sz < 2 || maxnr == 1 )
	return result;

    const uiString and_or_or = use_and ? uiStrings::sAnd() : uiStrings::sOr();
    uiString::SeparType septyp =
	separate_lines	? uiString::NoSep : uiString::Comma;
    uiString::AppendType apptyp =
	separate_lines	? uiString::OnNewLine : uiString::OnSameLine;
    if ( maxnr < 1 || maxnr > sz )
	maxnr = sz;


    for ( idx_type idx=1; idx<maxnr; idx++ )
    {
	if ( idx == sz-1 )
	{
	    result.appendPhrase( and_or_or, uiString::Space,
				 uiString::OnSameLine );
	    if ( septyp == uiString::Comma )
		septyp = uiString::Space;
	}
	result.appendPhrase( usestrs.get(idx), septyp, apptyp );
    }

    if ( sz > maxnr )
    {
	result.appendPhrase( and_or_or, uiString::Space, apptyp );
	result.appendPlainText( "...", true );
    }

    return result;
}


uiString uiStringSet::cat( SeparType septyp, AppendType apptyp ) const
{
    uiString result;
    for ( idx_type idx=0; idx<size(); idx++ )
	result.appendPhrase( *strs_[idx], septyp, apptyp );
    return result;
}


uiString uiStringSet::cat( const char* sepstr ) const
{
    const StringView sep( sepstr );
    if ( sep == StringView("\n") )
	return cat( uiString::NoSep, uiString::OnNewLine );
    else if ( sep == StringView("\t") )
	return cat( uiString::Tab, uiString::OnSameLine );
    else if ( sep == StringView(" ") )
	return cat( uiString::Space, uiString::OnSameLine );

    uiString result;
    const uiString uisepstr = toUiString( sepstr );
    for ( idx_type idx=0; idx<size(); idx++ )
    {
	if (idx)
	    result.appendPhrase(uisepstr,uiString::NoSep,uiString::OnSameLine);
	result.appendPhrase( *strs_[idx], uiString::NoSep,uiString::OnSameLine);
    }
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


uiStringSet::idx_type* uiStringSet::getSortIndexes( bool caseinsens,
						    bool ascending ) const
{
    const size_type sz = size();
    if ( sz < 1 )
	return 0;

    mGetIdxArr( size_type, idxs, sz );
    Qt::CaseSensitivity cs(Qt::CaseSensitive);
    if ( caseinsens )
	cs = Qt::CaseInsensitive;

    const uiStringSet* strset = this;
    QString qs1, qs2;
    for ( size_type d=sz/2; d>0; d=d/2 )
    {
	for ( size_type i=d; i<sz; i++ )
	{
	    for ( size_type j=i-d; j>=0; j-=d )
	    {
		strset->get(idxs[j]).fillQString( qs1 );
		strset->get(idxs[j+d]).fillQString( qs2 );
		if ( QString::compare(qs1,qs2,cs) <= 0 )
		    break;
		std::swap( idxs[j+d], idxs[j] );
	    }
	}
    }

    if ( !ascending )
	std::reverse( idxs, idxs+sz );

    return idxs;
}


const uiRetVal uiRetVal::ok_;

uiRetVal::uiRetVal( const uiPhrase& str )
{
    msgs_.add( str );
}


uiRetVal::uiRetVal( const uiPhrase& s1, const uiPhrase& s2 )
{
    msgs_.add( s1 ).add( s2 );
}


uiRetVal::uiRetVal( const uiPhrase& s1, const uiPhrase& s2, const uiPhrase& s3 )
{
    msgs_.add( s1 ).add( s2 ).add( s3 );
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
    return msgs_.isEmpty() ? uiPhrase::empty() : msgs_.cat();
}


uiRetVal::operator uiPhraseSet() const
{
    Threads::Locker locker( lock_ );
    return msgs_;
}


bool uiRetVal::isOK() const
{
    Threads::Locker locker( lock_ );
    return msgs_.isEmpty() || msgs_.get(0).isEmpty();
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
    return msgs_.size() == 1 && msgs_[0] == str;
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

    return toString( uistr );
}


bool isFinished( const uiRetVal& uirv )
{
    return uirv.isSingleWord( uiStrings::sFinished() );
}


bool isNotPresent( const uiRetVal& uirv )
{
    return uirv.isSingleWord( uiStrings::sNotPresent() );
}


bool isCancelled( const uiRetVal& uirv )
{
    return uirv.isSingleWord( uiStrings::sCancelled() );
}
