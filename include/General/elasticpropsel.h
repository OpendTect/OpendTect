#ifndef elasticpropsel_h
#define elasticpropsel_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Bruno
 Date:		May 2011
 RCS:		$Id$
________________________________________________________________________

-*/

/*! brief assigns values to an elastic layer depending on user defined parameters !*/

#include "elasticprop.h"

class IOObj;
class MultiID;


mClass ElasticPropSelection : public NamedObject
{
public:
				ElasticPropSelection(const char* nm=0);
				ElasticPropSelection(
					const ElasticPropSelection& elp)
				{ *this = elp; }

    ElasticPropSelection&     	operator =(const ElasticPropSelection&);
    inline bool         	operator ==(const ElasticPropSelection& e) const
   				{ return name() == e.name(); }
    inline bool         	operator !=(const ElasticPropSelection& e) const
    				{ return name() != e.name(); }

    bool			isPresent(const char*) const;
    int				indexOf(const char*) const;

    ElasticPropertyRef&		getPropertyRef(ElasticFormula::Type);
    const ElasticPropertyRef&	getPropertyRef(ElasticFormula::Type) const;

    static ElasticPropSelection* get(const MultiID&);
    static ElasticPropSelection* get(const IOObj*);
    bool                	put(const IOObj*) const;

    bool			isValidInput(BufferString* errmsg=0) const;

    void			fillPar(IOPar&) const;
    void			usePar(const IOPar&);

    const TypeSet<ElasticPropertyRef>& getPropertyRefs() const 
    				{ return elasticprops_; }

protected :
    TypeSet<ElasticPropertyRef>	elasticprops_;
};


mClass ElasticPropGen
{
public:
    			ElasticPropGen(const ElasticPropSelection& eps,
					const PropertyRefSelection& rps)
			    : elasticprops_(eps), refprops_(rps) {}

    float		getVal(const ElasticPropertyRef& ef,
	    			const float* proprefvals,
				int proprefsz) const
    			{ return getVal(ef.formula(),proprefvals, proprefsz); }
protected:

    ElasticPropSelection elasticprops_;
    const PropertyRefSelection& refprops_;

    float		getVal(const ElasticFormula& ef,
	    			const float* proprefvals,
				int proprefsz) const;
};



mClass ElasticPropGuess
{
public:
    			ElasticPropGuess(const PropertyRefSelection&,
						ElasticPropSelection&);
protected:
    void		guessQuantity(const PropertyRefSelection&,
					ElasticFormula::Type);

    ElasticPropSelection& elasticprops_; 

    bool		guessQuantity(const PropertyRef&,ElasticFormula::Type);
};


#endif

