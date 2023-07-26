#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uistring.h"
#include "objectset.h"

class QString;
template <class T> class QList;
class uiRetVal;


/*\brief Set of uiStrings */

mExpClass(Basic) uiStringSet
{	mODTextTranslationClass(uiStringSet)
	mIsContainer( uiStringSet, ObjectSet<uiString>, strs_ )
public:


    typedef uiString::AppendType		AppendType;
    typedef uiString::SeparType			SeparType;

			uiStringSet()				{}
			uiStringSet(const uiString&);
			uiStringSet(const uiString&,const uiString&);
			uiStringSet(const uiString&,const uiString&,
				    const uiString&);
			uiStringSet( const uiStringSet& oth )	{ *this = oth; }
			uiStringSet(const QList<QString>&);
			~uiStringSet();
    uiStringSet&	operator =(const uiStringSet&);

    inline size_type	size() const		{ return strs_.size(); }
    inline bool		validIdx( idx_type i ) const
						{ return strs_.validIdx(i); }
    bool		isEmpty() const		{ return strs_.isEmpty(); }
    bool		isPresent(const uiString&) const;
    idx_type		indexOf(const uiString&) const;
    uiString&		get(idx_type);
    const uiString&	get(idx_type) const;
    uiString&		operator []( idx_type i )	{ return get(i); }
    const uiString&	operator []( idx_type i ) const	{ return get(i); }

    void		setEmpty();
    uiStringSet&	set(const uiString&);
    uiStringSet&	set( const uiStringSet& oth )	{ return (*this=oth); }
    uiStringSet&	set(const uiRetVal&);
    uiStringSet&	add(const uiString&);
    uiStringSet&	add(const uiStringSet&);
    uiStringSet&	add(const uiRetVal&);
    uiStringSet&	append( const uiStringSet& ss )	{ return add(ss); }
    uiStringSet&	insert(idx_type,const uiString&);
    uiStringSet&	operator +=( const uiString& s ) { return add(s); }
    void		removeSingle(idx_type,bool keep_order=true);
    void		removeRange(idx_type,idx_type);
    void		swap( idx_type i1, idx_type i2 )
			{ strs_.swap( i1, i2 ); }

    uiString		cat(SeparType septyp=uiString::CloseLine,
			    AppendType apptyp=uiString::OnNewLine) const;
    uiStringSet		getNonEmpty() const;
    uiString		createOptionString(bool use_and=true,int maxnritems=-1,
				   bool separate_lines=false) const;
				//!< example: "option1, option2 and option3"
    uiStringSet&	addKeyValue(const uiWord& ky,const uiString& val);
    template <class T>
    uiStringSet&	addKeyValue( const uiWord& ky,const T& val )
			{ return addKeyValue( ky, toUiString(val) ); }

    void		fill(QList<QString>&) const;
    void		sort(const bool caseinsens=true,bool asc=true);
    void		useIndexes( const idx_type* idxs );
    idx_type*		getSortIndexes(bool caseinsens,bool asc) const;

public:

    uiString		cat(const char* sepstr) const;
    void		erase()	{ setEmpty(); }

};

mDefContainerSwapFunction( Basic, uiStringSet )


typedef uiStringSet uiPhraseSet;
typedef uiStringSet uiWordSet;


/*\brief allows returning status and accompanying user info.

  This class helps us make sure there is always user info on errors. Therefore,
  you will find a 'setIsOK' but no equivalent like 'setNotOK'. You will simply
  have to set a non-empty message.

*/

mExpClass(Basic) uiRetVal
{ mIsContainer( uiRetVal, uiPhraseSet, msgs_ )
public:

			uiRetVal()		{}
			uiRetVal(const uiPhrase&);
			uiRetVal(const uiPhrase&,const uiPhrase&);
			uiRetVal(const uiPhrase&,const uiPhrase&,
				 const uiPhrase&);
			uiRetVal(const uiPhraseSet&);
			uiRetVal(const uiRetVal&);
    static uiRetVal	OK()			{ return ok_; }
    static uiRetVal	Empty()			{ return ok_; }
    uiRetVal&		operator =(const uiRetVal&);
    uiRetVal&		operator =(const uiPhrase&);
    uiRetVal&		operator =(const uiPhraseSet&);
			operator uiPhrase() const;
			operator uiPhraseSet() const;

    bool		isOK() const;
    inline bool		isEmpty() const		{ return isOK(); }
    inline bool		isError() const		{ return !isOK(); }
    bool		isMultiMessage() const;
    uiPhraseSet		messages() const;
    bool		isSingleWord(const uiWord&) const;

    uiRetVal&		setEmpty();
    inline uiRetVal&	setOK()			{ return setEmpty(); }
    uiRetVal&		insert(const uiPhrase&);
    uiRetVal&		set(const uiRetVal&);
    uiRetVal&		set(const uiPhrase&);
    uiRetVal&		set(const uiPhraseSet&);
    uiRetVal&		add(const uiRetVal&);
    uiRetVal&		add(const uiPhrase&);
    uiRetVal&		add(const uiPhraseSet&);
    uiRetVal&		setAsStatus(const uiWord&);
    void                resetError() { setEmpty(); }

    BufferString	getText() const;

private:

    mutable Threads::Lock lock_;

    static const uiRetVal ok_;

};

mDefContainerSwapFunction( Basic, uiRetVal )

mGlobal(Basic) bool isFinished(const uiRetVal&);
mGlobal(Basic) bool isNotPresent(const uiRetVal&);
mGlobal(Basic) bool isCancelled(const uiRetVal&);
