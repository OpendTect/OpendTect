#ifndef elasticpropsel_h
#define elasticpropsel_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Bruno
 Date:		May 2011
 RCS:		$Id: elasticpropsel.h,v 1.5 2011-08-01 15:41:04 cvsbruno Exp $
________________________________________________________________________

-*/

/*! brief assigns values to an elastic layer depending on user defined parameters !*/

#include "ailayer.h"
#include "enums.h"
#include "propertyref.h"


static const char* sKeyElasticPropSel = "Elastic Properties selection";

mStruct ElasticFormula : public NamedObject
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

    static const char*	type2Char(ElasticType tp);
    static ElasticType		char2Type(const char* tptxt);

protected:
    BufferString 	expression_; 
    ElasticType		type_;
};



mStruct ElasticFormulaPropSel 
{
			ElasticFormulaPropSel(const char* nm, const char* expr,
			       			ElasticFormula::ElasticType tp )
			    : formula_(nm,expr,tp) {} 

			ElasticFormulaPropSel( const ElasticFormula& fm )
			    : formula_(fm) {} 

    TypeSet<int> 	selidxs_; //index of selected variables, -1 is constant
    TypeSet<float> 	ctes_; 	  //values of constants

    const char* 	subjectName() const;

    virtual void 	fillPar(IOPar&) const;
    virtual void 	usePar(const IOPar&);

    ElasticFormula	formula_;
};


mClass ElasticFormulaRepository 
{
public:
    void			addFormula(const ElasticFormula&); 
    void			addFormula(const char* nm, const char* expr,
					    ElasticFormula::ElasticType);

    const TypeSet<ElasticFormula>& denFormulas() const { return denformulas_; }
    const TypeSet<ElasticFormula>& pvelFormulas() const { return pvelformulas_;}
    const TypeSet<ElasticFormula>& svelFormulas() const { return svelformulas_;}

    void			clear() 
				{ 
				    denformulas_.erase(); 
				    pvelformulas_.erase();
				    svelformulas_.erase();
				}
protected:
    				ElasticFormulaRepository()
				{ 
				    fillPreDefFormulas(); 
				}

    void			fillPreDefFormulas();

    TypeSet<ElasticFormula> 	denformulas_;
    TypeSet<ElasticFormula> 	pvelformulas_;
    TypeSet<ElasticFormula> 	svelformulas_;

    static ElasticFormulaRepository* elasticrepos_;
    mGlobal friend ElasticFormulaRepository& elasticFormulas();
};

mGlobal ElasticFormulaRepository& elasticFormulas();



mClass ElasticPropSelection
{
public:
				ElasticPropSelection()
				    : denformula_("","",ElasticFormula::Den)
				    , pvelformula_("","",ElasticFormula::PVel)
				    , svelformula_("","",ElasticFormula::SVel)
				    {}

    ElasticFormulaPropSel	denformula_;
    ElasticFormulaPropSel	pvelformula_;
    ElasticFormulaPropSel 	svelformula_;

    bool			isValidInput() const;

    void			fillPar(IOPar&) const;
    void			usePar(const IOPar&);
};


mClass ElasticPropGuess
{
public:
    			ElasticPropGuess(const PropertyRefSelection&,
						ElasticPropSelection&);
protected:
    bool 		guessInputFromProps(const PropertyRefSelection&);
    bool		guessDen(const PropertyRefSelection& pps);
    bool		guessPVel(const PropertyRefSelection& pps);
    bool		guessSVel(const PropertyRefSelection& pps);
    int			guessQuantity(const PropertyRefSelection&,
				     const PropertyRef::StdType&);

    ElasticPropSelection& elasticprops_; 
};


mClass ElasticPropGen
{
public:
    			ElasticPropGen(const ElasticPropSelection& eps)
			    : elasticprops_(eps) {}

    void		fill(AILayer&,const float* vals,int sz);
    void		fill(ElasticLayer&,const float* vals,int sz);

protected:

    const ElasticPropSelection& elasticprops_;

    float		getPVel(const float* vals,int valsz);
    float		getSVel(const float* vals,int valsz);
    float		getDen(const float* vals,int valsz);

    float		getVal(const ElasticFormulaPropSel&,const float*,int); 
};

#endif

