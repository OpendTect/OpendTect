#ifndef mathattrib_h
#define mathattrib_h

/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Nanne Hemstra
 Date:          May 2005
 RCS:           $Id: mathattrib.h,v 1.17 2009-06-19 13:02:30 cvshelene Exp $
________________________________________________________________________

-*/

/*! \brief
#### Short description
\par
#### Detailed description.

*/

#include "attribprovider.h"

class MathExpression;

namespace Attrib
{

mClass Math : public Provider
{
public:
    static void			initClass();
    				Math(Desc&);

    static const char*		attribName()		{ return "Math"; }
    static const char*		expressionStr()		{ return "expression"; }
    static const char*		cstStr()		{ return "constant"; }
    static const char*		recstartStr()		{ return "recstart"; }
    static const char*		recstartposStr()	{ return "recstartpos";}

protected:
    				~Math()	{}
    static Provider*		createInstance(Desc&);
    static void			updateDesc(Desc&);

    bool			getInputOutput(int in,TypeSet<int>& res) const;
    bool			getInputData(const BinID&, int);
    bool			computeData(const DataHolder&,const BinID& pos,
	    				    int t0,int nrsamples,
					    int threadid) const;

    bool			allowParallelComputation() const;
    void			setUpVarsSets();

    const Interval<float>*	desZMargin(int input,int) const;
    const Interval<int>*        reqZSampMargin(int input,int) const;

private:
    ObjectSet<const DataHolder>	inputdata_;
    TypeSet<int>		inputidxs_;

    TypeSet<float>		csts_;
    MathExpression*		expression_;
    float			recstartval_;
    float			recstartpos_;
    Interval<float>		desintv_;
    Interval<int>		reqintv_;

    struct VAR
    {
				VAR( int varidx, int inputidx, int shift )
					: varidx_( varidx )
					, inputidx_( inputidx )
					, shift_( shift )
					, sampgate_( Interval<int>(shift,0) )
				{ sampgate_.sort(); }
				
	bool			operator ==(VAR var) const
				{ return var.varidx_ == varidx_
				    	 && var.inputidx_ == inputidx_
					 && var.shift_ == shift_; }

	int			varidx_;	//index of var in expression_ 
	int			inputidx_;	//corresponding inputdata_
	int			shift_;		//corresponding sample shift
	Interval<int>		sampgate_;	//sample gate (for convenience)
    };

    struct CSTS
    {
				CSTS( int fexpvaridx, int cstidx )
					: fexpvaridx_( fexpvaridx )
					, cstidx_( cstidx )
				{}
				
	bool			operator ==(CSTS csts) const
				{ return csts.fexpvaridx_ == fexpvaridx_
				    	 && csts.cstidx_ == cstidx_; }

	int			fexpvaridx_;	//index of var in expression_ 
	int			cstidx_;	//corresponding ConstantN param
    };

    //all variables including recursive THIS and shifted x0[-1]
    TypeSet<VAR>		varstable_;
    TypeSet<CSTS>		cststable_;
    
};

}; // namespace Attrib

#endif
