#ifndef mathattrib_h
#define mathattrib_h

/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Nanne Hemstra
 Date:          May 2005
 RCS:           $Id: mathattrib.h,v 1.12 2008-01-25 13:32:42 cvshelene Exp $
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

class Math : public Provider
{
public:
    static void			initClass();
    				Math(Desc&);

    static const char*		attribName()		{ return "Math"; }
    static const char*		expressionStr()		{ return "expression"; }
    static const char*		cstStr()		{ return "constant"; }
    static const char*		recstartStr()		{ return "recstart"; }

    static void 		getInputTable(const MathExpression*,
					      TypeSet<int>&,bool);

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
    bool			getInputAndShift(int varidx,int& inpidx,
	    					 int& shift) const;
    void			fillInVarsSet();

    const Interval<int>*        reqZSampMargin(int input,int) const;

private:
    ObjectSet<const DataHolder>	inputdata_;
    TypeSet<int>		inputidxs_;

    TypeSet<int>		cstsinputtable_;
    TypeSet<float>		csts_;
    MathExpression*		expression_;
    float			recstart_;

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

    //all variables including recursive THIS and shifted x0[-1]
    TypeSet<VAR>		varstable_;
    
};

}; // namespace Attrib

#endif
