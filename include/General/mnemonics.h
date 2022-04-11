#pragma once
/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Khushnood Qadir
 Date:		Aug 2020
 RCS:		$Id$
________________________________________________________________________

-*/

#include "generalmod.h"
#include "enums.h"
#include "bufstringset.h"
#include "namedobj.h"
#include "objectset.h"
#include "propertyref.h"
#include "ranges.h"
#include "uistring.h"
#include "unitofmeasure.h"

class MathProperty;

mExpClass(General) Mnemonic : public NamedObject
{ mODTextTranslationClass(Mnemonic);
public:

    enum Scale		{ Linear, Logarithmic };
			mDeclareEnumUtils(Scale)

			Mnemonic( const char* nm=nullptr )
			    : NamedObject(nm)
			    , mathdef_(nullptr)		{}
			Mnemonic( const Mnemonic& mnc )
			    : NamedObject(mnc.name())
			    , mathdef_(nullptr)		{ *this = mnc; }
    virtual		~Mnemonic();

    Mnemonic&		operator =(const Mnemonic&);
    bool		operator ==( const Mnemonic& mnc ) const;
    bool		operator !=( const Mnemonic& mnc ) const;
    bool		isKnownAs(const char*) const;
    bool		hasFixedDef() const		{ return mathdef_; }


    mExpStruct(General) DispDefs
    {
			DispDefs()
			: range_(mUdf(float),mUdf(float))
			, typicalrange_(mUdf(float),mUdf(float))	{}

	Color		color_;
	Interval<float> range_;
	Interval<float> typicalrange_;
	BufferString	unit_;
	Mnemonic::Scale scale_;
    };

    DispDefs		disp_;

    inline BufferStringSet&		aliases()	{ return aliases_; }
    inline const BufferStringSet&	aliases() const { return aliases_; }
    inline PropertyRef::StdType		stdType() const { return stdtype_; }
    const MathProperty&			fixedDef() const{ return *mathdef_; }
    const BufferString&			logTypeName() const
					{ return logtypename_; }

    inline void		setStdType( PropertyRef::StdType t )
			{ stdtype_ = t; }
    inline bool		hasType( PropertyRef::StdType t ) const
			{ return stdtype_ == t; }
    inline bool		isCompatibleWith( const PropertyRef& pr ) const
			{ return hasType(pr.stdType()); }
    void		setFixedDef(const MathProperty*);

protected:

    BufferStringSet		aliases_;
    BufferString		logtypename_;
    PropertyRef::StdType	stdtype_;
    MathProperty*		mathdef_;

    friend class		MnemonicSet;
    void			usePar(const IOPar&);
    void			fillPar(IOPar&) const;
};


mExpClass(General) MnemonicSet : public ObjectSet<Mnemonic>
{
public:
				MnemonicSet()	{}
				MnemonicSet( const MnemonicSet& mns )
				    : ObjectSet<Mnemonic>(){ *this = mns; }
				~MnemonicSet();

    MnemonicSet&		operator =(const MnemonicSet&);
    inline Mnemonic*		find(const char* nm)	{ return fnd(nm); }
    inline const Mnemonic*	find(const char* nm) const { return fnd(nm); }
    MnemonicSet*		getSet(const PropertyRef*);
    Mnemonic*			getGuessed(const UnitOfMeasure*);
				//first match only
    Mnemonic*			getGuessed(PropertyRef::StdType);
				//first match only
    const Mnemonic*		getGuessed(const UnitOfMeasure*) const;
				//first match only
    const Mnemonic*		getGuessed(PropertyRef::StdType) const;
				//first match only
    void			getNames(BufferStringSet&) const;

    int			add(Mnemonic*);
    int			indexOf(const char*) const;
    bool		save() const;
    void		readFrom(ascistream&);
    bool		writeTo(ascostream&) const;
    int			indexOf( const Mnemonic* mn ) const
			{ return ObjectSet<Mnemonic>::indexOf(mn); }
    inline bool		isPresent( const Mnemonic* mn ) const
			{ return ObjectSet<Mnemonic>::isPresent(mn); }
    inline bool		isPresent( const char* nm ) const
			{ return indexOf(nm) >= 0; }

protected:

    Mnemonic*		fnd(const char*) const;
    virtual MnemonicSet&	doAdd( Mnemonic* mn )
				{ add(mn); return *this; }
};


mGlobal( General ) const MnemonicSet& MNC();
inline MnemonicSet& eMNC()	{ return const_cast<MnemonicSet&>(MNC()); }


mExpClass(General) MnemonicSelection : public ObjectSet<const Mnemonic>
{
public:

			MnemonicSelection()	{}
    bool		operator ==(const MnemonicSelection&) const;

    int			indexOf(const char*) const;
    int			find(const char*) const;

    inline bool		isPresent( const char* mnnm ) const
			{ return indexOf( mnnm ) >= 0; }
    inline int		indexOf( const Mnemonic* mn ) const
			{ return ObjectSet<const Mnemonic>::indexOf(mn); }
    inline bool		isPresent( const Mnemonic* mn ) const
			{ return ObjectSet<const Mnemonic>::isPresent(mn); }

    inline const Mnemonic* getSingle( const char* nm ) const
			{ const int idx = indexOf(nm);
			  return idx < 0 ? 0 : (*this)[idx]; }

    static MnemonicSelection getAll(const Mnemonic* exclude=nullptr);
};
