#ifndef elasticpropsel_h
#define elasticpropsel_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Bruno
 Date:		May 2011
 RCS:		$Id: elasticpropsel.h,v 1.3 2011-07-25 15:07:49 cvsbruno Exp $
________________________________________________________________________

-*/

/*! brief assigns values to an elastic layer depending on user defined parameters !*/

#include "ailayer.h"
#include "propertyref.h"
#include "undefval.h"

class MathExpression;

mStruct ElasticFormula 
{ 
				ElasticFormula( const char* title, 
						const char* expr )
				    : expression_(expr)
				    , title_(title)
				    {} 

    BufferString 		expression_; 
    BufferString 		title_; 
};


mClass ElasticFormulaRepository 
{
public:

    virtual void 		fillPar(IOPar&) const {};
    virtual void 		usePar(const IOPar&) {};

    void			addFormula( const char* title, 
					    const char* formulatxt );

    int				size() const 	{ return formulas_.size(); }
    const ElasticFormula&	formula(int id) const { return *formulas_[id]; }

protected:
    ObjectSet<ElasticFormula> 	formulas_;
};


mClass DenElasticFormulaRepository : public ElasticFormulaRepository
{
public:
    				DenElasticFormulaRepository();
};


mClass PVelElasticFormulaRepository : public ElasticFormulaRepository
{
public:
    				PVelElasticFormulaRepository();
};


mClass SVelElasticFormulaRepository : public ElasticFormulaRepository
{
public:
    				SVelElasticFormulaRepository();
};



mClass ElasticPropGen
{
public:

    mStruct InpData
    {			
			InpData() : expr_(0) {}

	MathExpression* expr_;
	BufferString 	inputtxt_;
	TypeSet<int> 	selidxs_;

	virtual ElasticFormulaRepository& formulaRepos() = 0;
    };

    mStruct DenInputData : public InpData 
    {
        ElasticFormulaRepository& formulaRepos() { return repos_; }	
	DenElasticFormulaRepository repos_; 
    };

    mStruct PVelInputData : public InpData 
    { 
	ElasticFormulaRepository& formulaRepos() { return repos_; }	
	PVelElasticFormulaRepository repos_; 
    };

    mStruct SVelInputData : public InpData 
    { 
	ElasticFormulaRepository& formulaRepos() { return repos_; }	
	SVelElasticFormulaRepository repos_; 
    };

    DenInputData&	denInput() 	{ return deninpdata_; }
    PVelInputData&	pvelInput() 	{ return pvelinpdata_; }
    SVelInputData&	svelInput() 	{ return svelinpdata_; }

    void		fill(AILayer&,const float* vals,int sz);
    void		fill(ElasticLayer&,const float* vals,int sz);

    void 		guessInputFromProps(const PropertyRefSelection&);

    void		findDen(const PropertyRefSelection& pps);
    void		findPVel(const PropertyRefSelection& pps);
    void		findSVel(const PropertyRefSelection& pps);
    int			findQuantity(const PropertyRefSelection&,
				     const PropertyRef::StdType&);

protected:

    DenInputData	deninpdata_;
    PVelInputData	pvelinpdata_;
    SVelInputData	svelinpdata_;

    float		getPVel(const float* vals,int valsz);
    float		getSVel(const float* vals,int valsz);
    float		getDen(const float* vals,int valsz);

    float		getVal(InpData&,const float*,int); 
};

#endif

