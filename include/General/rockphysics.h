#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Bert Bril
 Date:		Feb 2012
________________________________________________________________________


-*/

#include "mnemonics.h"
#include "repos.h"

class ascistream;
class ascostream;
class MathProperty;
class PropertyRef;
class RockPhysicsFormulaMgr;



namespace RockPhysics
{

/*!\brief A Mathematics formula based on Mnemonics */

mExpClass(General) Formula : public NamedObject
{
public:
			Formula( const Mnemonic& mn, const char* nm=nullptr )
			    : NamedObject(nm)
			    , mn_(&mn)			{}
			~Formula();

    inline bool		isCompatibleWith( const Mnemonic* mn ) const
						{ return mn_ == mn; }

    mExpClass(General) ConstDef : public NamedObject
    {
    public:
			ConstDef( const char* nm )
			    : NamedObject(nm)
			    , typicalrg_(mUdf(float),mUdf(float))
			    , defaultval_(mUdf(float))	{}
	BufferString	desc_;
	Interval<float>	typicalrg_;
	float		defaultval_;
    };
    mExpClass(General) VarDef : public NamedObject
    {
    public:
			VarDef( const char* nm, const Mnemonic& mn )
			    : NamedObject(nm)
			    , mn_(&mn)			{}
	BufferString	desc_;
	const Mnemonic* mn_;
	BufferString	unit_;
    };

    BufferString	def_;
    BufferString	desc_;
    const Mnemonic*	mn_;
    BufferString	unit_;
    ObjectSet<ConstDef>	constdefs_;
    ObjectSet<VarDef>	vardefs_;

    void		setSource( Repos::Source src )	{ src_ = src; }

    bool		setDef(const char*); // Will add var- and constdefs
    MathProperty*	getProperty(const PropertyRef* pr=nullptr) const;

private:

			Formula( const Formula& f ) { *this = f; }

    static Formula*	get(const IOPar&);	//!< returns null if bad IOPar

    Formula&		operator =(const Formula&);
    inline bool		operator ==( const Formula& pr ) const
			{ return name() == pr.name(); }
    inline bool		operator !=( const Formula& pr ) const
			{ return name() != pr.name(); }

    bool		usePar(const IOPar&);
    void		fillPar(IOPar&) const;

    Repos::Source	src_;

    friend class FormulaSet;

public:
			mDeprecated("Use Mnemonic")
    bool		hasPropType(Mnemonic::StdType) const;

};


/*!\brief A repository of mathematics formulaes based on Mnemonics */

mExpClass(General) FormulaSet : public ObjectSet<const Formula>
{
public:
    typedef Mnemonic::StdType PropType;

			~FormulaSet();

    int			getIndexOf(const char*) const;
    bool		hasType(PropType) const;
    void		getRelevant(const Mnemonic&,
				    BufferStringSet&) const;
    void		getRelevant(PropType,MnemonicSelection&) const;

    const Formula*	getByName( const char* nm ) const
			{
			    const int idxof = getIndexOf( nm );
			    return validIdx(idxof) ? get( idxof ) : nullptr;
			}

private:

    bool		save(Repos::Source) const;

    void		readFrom(ascistream&);
    bool		writeTo(ascostream&) const;

    friend class ::RockPhysicsFormulaMgr;

};

} // namespace RockPhysics

mGlobal(General) const RockPhysics::FormulaSet& ROCKPHYSFORMS();
mGlobal(General) inline RockPhysics::FormulaSet& eROCKPHYSFORMS()
{ return const_cast<RockPhysics::FormulaSet&>( ROCKPHYSFORMS() ); }


