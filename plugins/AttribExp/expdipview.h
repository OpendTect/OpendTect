#ifndef expdipview_h
#define expdipview_h

/*@+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Kristofer Tingdahl
 Date:          07-10-1999
 RCS:           $Id: expdipview.h,v 1.6 2009/07/22 16:01:26 cvsbert Exp $
________________________________________________________________________

DipView [aspect=dip,azi]

DipView makes a relief shading from the dip and azimuth, useful for
visualisation of dips in grids. The aspect parameter determines the position
of the "light" in the resulting image.

Input:
0	Inline dip (as angle)
1	Crossline dip (as angle)

Output:
0	view

@$*/

#include <attribcalc.h>
#include <task.h>
#include <position.h>
#include <limits.h>
#include <seistrc.h>
#include <attribparamimpl.h>

    
class DipViewAttrib : public AttribCalc
{
public:
    mAttrib1Param( DipViewAttrib
	, "DipView"
	, BinIDAttribParameter
	    , aspect
	    , BinIDAttribParameter( "aspect"
		, AttribParameter::Required
		, BinID(60,0)
		, Interval<int>(0,90)
		, Interval<int>(-180,180)
	    )
	, mAttribParamFormHasNoUpdate );

				DipViewAttrib( Parameters* );
				~DipViewAttrib();
    const Interval<float>*	inlDipMargin(int,int) const { return 0;}
    const Interval<float>*	crlDipMargin(int,int) const { return 0;}


    int                 	nrAttribs() const { return 1; }
    const char* 		definitionStr() const { return desc; }

    virtual Seis::DataType	dataType(int val,const TypeSet<Seis::DataType>&) const 
				{ return val?Seis::UnknowData:Seis::Dip; }

protected:
   
    BinID		aspect; //Inl = dip, crl=azimuth
    BufferString	desc;

    class Task : public AttribCalc::Task
    {
    public:
	class Input : public AttribCalc::Task::Input
	{
	public:
				Input( const DipViewAttrib& calculator_ )
				: calculator ( calculator_ ) {}

	    bool                set( const BinID&, 
				    const ObjectSet<AttribProvider>&, 
				    const TypeSet<int>&,
				    const TypeSet<float*>&  );

	    AttribCalc::Task::Input* clone() const
			{ return new DipViewAttrib::Task::Input(*this); }

	    const SeisTrc*	inldip;
	    const SeisTrc*	crldip;

	    int			inlattrib;
	    int			crlattrib;

	    const DipViewAttrib& calculator;
	};

			    Task( const DipViewAttrib& calculator_ )
				: outp( 0 )
				, calculator( calculator_ ) {}
	
			    Task( const Task& );
			    // Not impl. Only to give error if someone uses it
	
	void		    set( float t1_, int nrtimes_, float step_, 
					    const AttribCalc::Task::Input* inp,
                                            const TypeSet<float*>& outp_)
				{ t1 = t1_; nrtimes = nrtimes_; 
				  step = step_; input = inp; outp = outp_[0]; }

	AttribCalc::Task*    clone() const;

	int		    getFastestSz() const { return 25; }

	int		    nextStep();

	AttribCalc::Task::Input* getInput() const
		    { return new DipViewAttrib::Task::Input( calculator ); }

    protected:
	float*			outp;
	const DipViewAttrib& 	calculator;

    };

    friend class	DipViewAttrib::Task;
    friend class	DipViewAttrib::Task::Input;
};

#endif
