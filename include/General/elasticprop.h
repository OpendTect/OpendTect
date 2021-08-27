#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Bruno
 Date:		July 2011
________________________________________________________________________

-*/

#include "propertyref.h"
#include "repos.h"

/*!
\brief Elastic formula def to generate elastic layers.
*/

mExpClass(General) ElasticFormula : public NamedObject
{
public:

			enum Type	{ Den, PVel, SVel };
			mDeclareEnumUtils(Type)

			ElasticFormula(const char* nm,const char* expr,Type tp)
				: NamedObject( nm )
				, expression_(expr ? expr : "")
				, type_(tp)		{}

			ElasticFormula( const ElasticFormula& fm )
							{ *this = fm; }

    ElasticFormula&	operator =(const ElasticFormula&);
    inline bool operator ==( const ElasticFormula& pr ) const
			{ return name() == pr.name(); }
    inline bool		operator !=( const ElasticFormula& pr ) const
			{ return name() != pr.name(); }

    void		setExpression( const char* expr) { expression_ = expr; }
    const char*		expression() const	{ return expression_.str();}

    inline Type		type() const		{ return type_; }
    inline bool		hasType( Type t ) const { return type_ == t;}

    const BufferStringSet& variables() const	{ return variables_; }
    BufferStringSet&	variables()		{ return variables_; }
    const BufferStringSet& units() const	{ return units_; }
    BufferStringSet&	units()			{ return units_; }
    const char*		parseVariable(int idx,float&) const;

    void		fillPar(IOPar&) const;
    void		usePar(const IOPar&);

protected:


    BufferString	expression_;
    BufferStringSet	variables_;
    BufferStringSet	units_;

    Type		type_;

    friend class ElasticFormulaRepository;
    friend class ElasticPropGuess;
};


/*!
\brief ElasticFormula repository.
*/

mExpClass(General) ElasticFormulaRepository
{
public:

    void			addFormula(const ElasticFormula&);
    void			addFormula(const char* nm, const char* expr,
					ElasticFormula::Type,
					const BufferStringSet& vars);

    void			getByType(ElasticFormula::Type,
					  TypeSet<ElasticFormula>&) const;

    void			clear()  { formulas_.erase(); }

    bool			write(Repos::Source) const;

protected:

    TypeSet<ElasticFormula>	formulas_;

    void			addRockPhysicsFormulas();
    void			addPreDefinedFormulas();

    mGlobal(General) friend ElasticFormulaRepository& ElFR();
};

mGlobal(General) ElasticFormulaRepository& ElFR();


/*!
\brief Elastic property reference data.
*/

mExpClass(General) ElasticPropertyRef : public PropertyRef
{
public:
			ElasticPropertyRef(const Mnemonic&,const char*,
					   const ElasticFormula&);

    ElasticPropertyRef* clone() const override;

    bool		isElasticForm() const override	{ return true; }

    static Mnemonic::StdType elasticToStdType(ElasticFormula::Type);

    ElasticFormula&	formula()		{ return formula_; }
    const ElasticFormula& formula() const	{ return formula_; }

    ElasticFormula::Type elasticType()		{ return formula_.type(); }
    ElasticFormula::Type elasticType() const	{ return formula_.type(); }

private:

    ElasticFormula	formula_;

};


