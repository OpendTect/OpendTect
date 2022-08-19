#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "mathformula.h"
#include "mnemonics.h"
#include "repos.h"

class ascistream;
class ascostream;
class ElasticFormula;
class MathProperty;
class PropertyRef;
class RockPhysicsFormulaMgr;


namespace RockPhysics
{

/*!\brief A Mathematics formula based on Mnemonics */

mExpClass(General) Formula : public Math::Formula
{
public:
			Formula(const Mnemonic&,const char* nm=nullptr);
			Formula(const Formula&);
			~Formula();
    bool		operator ==(const Formula&) const;
			//!*< Does not use the source
    bool		operator !=(const Formula&) const;
			//!*< Does not use the source

    bool		hasSameForm(const Math::Formula&) const;

    bool		usePar(const IOPar&);
    void		fillPar(IOPar&) const;

    void		setSource( Repos::Source src )	{ src_ = src; }

    MathProperty*	getProperty(const PropertyRef* =nullptr) const;

    void		setConstantName(int,const char*);
    void		setInputTypicalRange(int,const Interval<float>&);

    const char*		inputConstantName(int) const;
    Interval<float>	inputTypicalRange(int) const;

    void		setText(const char*) override;

private:

    static Formula*	get(const IOPar&);	//!< returns null if bad IOPar

    Formula&		operator =(const Formula&);

			//Not  supported by the form:
    BufferStringSet	constantnms_;
    ObjectSet<Interval<float> > typicalrgs_;

    Repos::Source	src_;

    friend class FormulaSet;
    friend class ::ElasticFormula;

public:
			mDeprecated("Use setText")
    bool		setDef(const char*);

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
    const Formula*	getByName(const Mnemonic&,const char*) const;
    void		getRelevant(PropType,MnemonicSelection&) const;
    bool		getRelevant(const Mnemonic&,
				    ObjectSet<const Math::Formula>&) const;

private:

    bool		save(Repos::Source) const;

    void		readFrom(ascistream&);
    bool		writeTo(ascostream&) const;

    friend class ::RockPhysicsFormulaMgr;

public:

    mDeprecated("Name may not be unique. Provide a mnemonic")
    const Formula*	getByName( const char* nm ) const
			{
			    const int idxof = getIndexOf( nm );
			    return validIdx(idxof) ? get( idxof ) : nullptr;
			}

    mDeprecated("Use ObjectSet<const Math::Formula>")
    void		getRelevant(const Mnemonic&,
				    BufferStringSet&) const;

};

} // namespace RockPhysics

mGlobal(General) const RockPhysics::FormulaSet& ROCKPHYSFORMS();
mGlobal(General) inline RockPhysics::FormulaSet& eROCKPHYSFORMS()
{ return const_cast<RockPhysics::FormulaSet&>( ROCKPHYSFORMS() ); }
