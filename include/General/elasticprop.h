#ifndef elasticprop_h
#define elasticprop_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Bruno
 Date:		July 2011
 RCS:		$Id: elasticprop.h,v 1.1 2011-08-03 15:17:51 cvsbruno Exp $
________________________________________________________________________

-*/

/*! brief elastic formula def to generate elastic layers !*/

#include "enums.h"
#include "bufstringset.h"
#include "repos.h"


mClass ElasticFormula : public NamedObject
{
public:
    enum ElasticType 	{ Den, PVel, SVel };
    			DeclareEnumUtils(ElasticType)

    			ElasticFormula(const char* nm,const char* expr,
					ElasticType tp)
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

    inline ElasticType 	type() const 			{ return type_; }
    inline bool        	hasType( ElasticType t ) const 	{ return type_ == t;}

    BufferStringSet&	variables() 			{ return variables_; }
    const BufferStringSet& variables() const 		{ return variables_; }
    const char*		parseVariable(int idx,float&) const;

    void 		fillPar(IOPar&) const;
    void 		usePar(const IOPar&);

protected:

    BufferString 	expression_; 
    BufferStringSet	variables_;
    ElasticType		type_;
};



mClass ElasticFormulaRepository 
{
public:
    void			addFormula(const ElasticFormula&); 
    void			addFormula(const char* nm, const char* expr,
					ElasticFormula::ElasticType,
					const BufferStringSet& vars);

    void 			getByType(ElasticFormula::ElasticType,
					      TypeSet<ElasticFormula>&) const;

    void			clear()  { formulas_.erase(); }

    bool                	write(Repos::Source) const;

protected:
    				ElasticFormulaRepository();

    TypeSet<ElasticFormula> 	formulas_;

    void 			addFormulasFromFile(const char*,Repos::Source);

    friend ElasticFormulaRepository& ElFR();
};

mGlobal ElasticFormulaRepository& ElFR();

#endif

