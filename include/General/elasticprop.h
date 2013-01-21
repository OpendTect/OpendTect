#ifndef elasticprop_h
#define elasticprop_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Bruno
 Date:		July 2011
 RCS:		$Id$
________________________________________________________________________

-*/

/*! brief elastic formula def to generate elastic layers !*/

#include "generalmod.h"
#include "enums.h"
#include "bufstringset.h"
#include "repos.h"
#include "propertyref.h"


mExpClass(General) ElasticFormula : public NamedObject
{
public:
			enum Type 	{ Den, PVel, SVel };
			DeclareEnumUtils(Type)

			ElasticFormula(const char* nm,const char* expr,Type tp)
				: NamedObject( nm ) 
				, expression_(expr ? expr : "")
				, type_(tp)
				{}

			ElasticFormula( const ElasticFormula& fm )
			{ *this = fm; }

    ElasticFormula&	operator =(const ElasticFormula&);
    inline bool 	operator ==( const ElasticFormula& pr ) const
			{ return name() == pr.name(); }
    inline bool         operator !=( const ElasticFormula& pr ) const
			{ return name() != pr.name(); }

    void		setExpression( const char* expr) 
			{ expression_ = expr ? expr : ""; }
    const char*		expression() const 
			{ return expression_.isEmpty() ? 0 : expression_.buf();}

    inline Type 	type() const 			{ return type_; }
    inline bool        	hasType( Type t ) const 	{ return type_ == t;}

    BufferStringSet&	variables() 			{ return variables_; }
    const BufferStringSet& variables() const 		{ return variables_; }
    BufferStringSet&	units() 			{ return units_; }
    const BufferStringSet& units() const 		{ return units_; }
    const char*		parseVariable(int idx,float&) const;

    void 		fillPar(IOPar&) const;
    void 		usePar(const IOPar&);

protected:

    BufferString 	expression_; 
    BufferStringSet	variables_;
    BufferStringSet	units_;
    
    Type		type_;
};



mExpClass(General) ElasticFormulaRepository 
{
public:
    void			addFormula(const ElasticFormula&); 
    void			addFormula(const char* nm, const char* expr,
					ElasticFormula::Type,
					const BufferStringSet& vars);

    void 			getByType(ElasticFormula::Type,
					  TypeSet<ElasticFormula>&) const;

    void			clear()  { formulas_.erase(); }

    bool                	write(Repos::Source) const;

protected:

    TypeSet<ElasticFormula> 	formulas_;

    void 			addRockPhysicsFormulas();
    void 			addPreDefinedFormulas();

    mGlobal(General) friend ElasticFormulaRepository& ElFR();
};

mGlobal(General) ElasticFormulaRepository& ElFR();



mExpClass(General) ElasticPropertyRef : public PropertyRef
{
public:
			ElasticPropertyRef(const char* nm,
					const ElasticFormula& f)
			    : PropertyRef(nm)
			    , formula_(f)
			    {
				stdtype_ = elasticToStdType( formula_.type() );
			    }


    static PropertyRef::StdType elasticToStdType(ElasticFormula::Type); 

    ElasticFormula& formula() 			{ return formula_; }
    const ElasticFormula& formula() const 	{ return formula_; }

    ElasticFormula::Type elasticType()  	{ return formula_.type(); }
    ElasticFormula::Type elasticType() const	{ return formula_.type(); }

protected:
    ElasticFormula      formula_;
};


#endif


