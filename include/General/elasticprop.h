#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Bruno
 Date:		July 2011
________________________________________________________________________

-*/

#include "mathformula.h"
#include "propertyref.h"

namespace RockPhysics { class Formula; }


/*!
\brief Elastic formula def to generate elastic layers.
*/

mExpClass(General) ElasticFormula : public Math::Formula
{
public:

			enum Type	{ Den, PVel, SVel, Undef };
			mDeclareEnumUtils(Type)

			ElasticFormula(const char* nm,const char* expr,Type);
			~ElasticFormula();

    static ElasticFormula* getFrom(const RockPhysics::Formula&);

    Type		type() const;
    bool		hasType(Type) const;
    void		setType(Type);

    void		fillPar(IOPar&) const;
    void		usePar(const IOPar&);

    static const Mnemonic& getMnemonic(Type);
    static Type		getType(const Mnemonic&);
    static Mnemonic::StdType getStdType(Type);

private:

    friend class ElasticFormulaRepository;
    friend class ElasticPropGuess;

public:

    mDeprecated("Use setText")
    void		setExpression(const char*);
    mDeprecated("Use text")
    const char*		expression() const;

    mDeprecatedObs
    const BufferStringSet variables() const;
    mDeprecatedObs
    BufferStringSet	variables();
    mDeprecatedObs
    const BufferStringSet units() const;
    mDeprecatedObs
    BufferStringSet	units();

    mDeprecatedObs
    const char*		parseVariable(int idx,float&) const;
};


/*!
\brief ElasticFormula repository.
*/

mExpClass(General) ElasticFormulaRepository
{
public:
				~ElasticFormulaRepository();

    void			getByType(ElasticFormula::Type,
				      ObjectSet<const Math::Formula>&) const;

    bool			write(Repos::Source) const;

protected:

    ObjectSet<ElasticFormula>	formulas_;

    void			addRockPhysicsFormulas();

    mGlobal(General) friend ElasticFormulaRepository& ElFR();
};

mGlobal(General) ElasticFormulaRepository& ElFR();


/*!
\brief Elastic property reference data. Either a link to an existing
	PropertyRef, or an ElasticFormula
*/

mExpClass(General) ElasticPropertyRef : public PropertyRef
{
public:
			ElasticPropertyRef(const Mnemonic&,const char*);
			ElasticPropertyRef(const ElasticPropertyRef&);
			~ElasticPropertyRef();
    ElasticPropertyRef& operator =(const ElasticPropertyRef&);

    bool		operator ==(const ElasticPropertyRef&) const;
    bool		operator !=(const ElasticPropertyRef&) const;

    ElasticPropertyRef* clone() const override;

    bool		isOK(const PropertyRefSelection*) const;
			//<! Try to provide a selection whenever possible
    bool		isElasticForm() const override	{ return true; }

			//<! Direct: not formula based
    const PropertyRef*	ref() const			{ return pr_; }
    void		setRef(const PropertyRef*);

			//<! Formula based
    const ElasticFormula* formula() const		{ return formula_; }
    void		setFormula(const ElasticFormula&);

    bool		usePar(const IOPar&);
    void		fillPar(IOPar&) const;

    ElasticFormula::Type elasticType() const;

private:

    static void		setFormNameFromRepos(ElasticFormula&);

    const PropertyRef*	pr_ = nullptr;
    ElasticFormula*	formula_ = nullptr;

public:

    mDeprecated("Use ElasticFormula::getMnemonic")
    static const Mnemonic& elasticToMnemonic(ElasticFormula::Type);
    mDeprecated("Use ElasticFormula::getStdType")
    static Mnemonic::StdType elasticToStdType(ElasticFormula::Type);

};


