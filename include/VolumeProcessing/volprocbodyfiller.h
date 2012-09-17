#ifndef volprocbodyfiller_h
#define volprocbodyfiller_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Yuancheng Liu
 Date:		November 2007
 RCS:		$Id: volprocbodyfiller.h,v 1.7 2011/08/26 08:24:52 cvskris Exp $
________________________________________________________________________


-*/

#include "volprocchain.h"
#include "arrayndimpl.h"

namespace EM { class EMObject; class Body; class ImplicitBody; }

namespace VolProc
{

class Step;

mClass BodyFiller : public Step
{
public:
	mDefaultFactoryCreatorImpl( VolProc::Step, BodyFiller );
	mDefaultFactoryInstanciationBase( "BodyFiller", "Body shape painter");

    				BodyFiller();
    				~BodyFiller();

    bool			needsInput() const; 
    bool			areSamplesIndependent() const { return true; }
    
    void			fillPar(IOPar&) const;
    bool			usePar(const IOPar&);

    void			releaseData();
    bool			canInputAndOutputBeSame() const { return true; }
    bool			needsFullVolume() const		{ return false;}
	
    void			setOutput(Attrib::DataCubes*);
    void			setOutput( Attrib::DataCubes* ad,
					    const StepInterval<int>&,
					    const StepInterval<int>&,
					    const StepInterval<int>& )
				{ return setOutput(ad); }

    float			getInsideValue()  { return insideval_; }
    float			getOutsideValue() { return outsideval_; } 
    void			setInsideOutsideValue(const float inside, 
	    					      const float ouside);
    
    bool			setSurface(const MultiID&);
    MultiID			getSurfaceID() { return mid_; }
    Task*			createTask();

    static const char*		sKeyOldType() { return "MarchingCubes"; }
    static const char*		sKeyMultiID(){return "Body ID"; }
    static const char*		sKeyOldMultiID()
    					{ return "MarchingCubeSurface ID"; }
    static const char*		sKeyInsideOutsideValue() 
    					{ return "Surface InsideOutsideValue"; }
   
protected:

    bool			prefersBinIDWise() const	{ return true; }
    bool			prepareComp(int nrthreads)	{ return true; }
    bool			computeBinID(const BinID&,int); 
    bool			getFlatPlgZRange(const BinID&,
						 Interval<double>& result); 

    EM::Body*			body_;
    EM::EMObject*		emobj_;
    EM::ImplicitBody*		implicitbody_;
    MultiID			mid_;

    float			insideval_;
    float			outsideval_;

    				//For flat body_ only, no implicitbody_.
    CubeSampling		flatpolygon_;
    TypeSet<Coord3>		plgknots_;
    TypeSet<Coord3>		plgbids_;
    char			plgdir_;
    				/* inline=0; crosline=1; z=2; other=3 */
    double			epsilon_;
};


}; //namespace

#endif
