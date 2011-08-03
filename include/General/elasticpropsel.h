#ifndef elasticpropsel_h
#define elasticpropsel_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Bruno
 Date:		May 2011
 RCS:		$Id: elasticpropsel.h,v 1.6 2011-08-03 15:17:51 cvsbruno Exp $
________________________________________________________________________

-*/

/*! brief assigns values to an elastic layer depending on user defined parameters !*/

#include "ailayer.h"
#include "elasticprop.h"
#include "propertyref.h"
#include "transl.h"

class IOObj;

mClass ElasticPropSelection : public NamedObject
{
public:
				ElasticPropSelection(const char* nm=0);

    ElasticFormula&		getFormula(ElasticFormula::ElasticType);
    const ElasticFormula&	getFormula(ElasticFormula::ElasticType) const;

    static ElasticPropSelection* get(const IOObj*);
    bool                	put(const IOObj*) const;

    bool			isValidInput() const;

    void			fillPar(IOPar&) const;
    void			usePar(const IOPar&);

protected :
    TypeSet<ElasticFormula> 	selectedformulas_;
};



mClass ElasticPropSelectionTranslatorGroup : public TranslatorGroup
{                			isTranslatorGroup(ElasticPropSelection)
public:
		    mDefEmptyTranslatorGroupConstructor(ElasticPropSelection)

    const char*          defExtension() const           { return "elastic"; }
};



mClass ElasticPropSelectionTranslator : public Translator
{
public:
			mDefEmptyTranslatorBaseConstructor(ElasticPropSelection)

    bool 		read(ElasticPropSelection*,Conn&);
    bool 		write(const ElasticPropSelection*,Conn&);
};


mClass ElasticPropGen
{
public:
    			ElasticPropGen(const ElasticPropSelection& eps,
					const PropertyRefSelection& rps)
			    : elasticprops_(eps), refprops_(rps) {}

    void		fill(AILayer&,const float* proprefvals,int sz);
    void		fill(ElasticLayer&,const float* proprefvals,int sz);
protected:

    const ElasticPropSelection& elasticprops_;
    const PropertyRefSelection& refprops_;

    float		setVal(const ElasticFormula& ef,
	    			const float* proprefvals,int proprefsz); 
};



mClass ElasticPropGuess
{
public:
    			ElasticPropGuess(const PropertyRefSelection&,
						ElasticPropSelection&);
protected:
    void		guessQuantity(const PropertyRefSelection&,
				    ElasticFormula::ElasticType);

    PropertyRef::StdType elasticToStdType(ElasticFormula::ElasticType) const;

    ElasticPropSelection& elasticprops_; 
};


#endif

