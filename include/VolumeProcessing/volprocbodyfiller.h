#ifndef volprocbodyfiller_h
#define volprocbodyfiller_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Yuancheng Liu
 Date:		November 2007
 RCS:		$Id: volprocbodyfiller.h,v 1.2 2009-11-18 19:53:34 cvskris Exp $
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
    static void			initClass();
    				BodyFiller(Chain&);
    				~BodyFiller();

    const char*			type() const { return sKeyType(); }
    bool			needsInput(const HorSampling&) const; 
    bool			areSamplesIndependent() const { return true; }
    
    void			fillPar(IOPar&) const;
    bool			usePar(const IOPar&);

    void			setOutput(Attrib::DataCubes*);

    float			getInsideValue()  { return insideval_; }
    float			getOutsideValue() { return outsideval_; } 
    void			setInsideOutsideValue(const float inside, 
	    					      const float ouside);
    
    bool			setSurface(const MultiID&);
    MultiID			getSurfaceID() { return mid_; }
    Task*			createTask();

    static const char*		sKeyType() { return "BodyFiller"; }
    static const char*		sKeyOldType() { return "MarchingCubes"; }
    static const char* 		sUserName(){ return "Body shape painter"; }
    static const char*		sKeyMultiID(){return "Body ID"; }
    static const char*		sKeyOldMultiID(){return "MarchingCubeSurface ID"; }
    static const char*		sKeyInsideOutsideValue() 
    					{ return "Surface InsideOutsideValue"; }
   
protected:

    bool			prefersBinIDWise() const	{ return true; }
    bool			prepareComp(int nrthreads)	{ return true; }
    bool			computeBinID(const BinID&,int);  

    static Step*		create(Chain&);

    EM::Body*			body_;
    EM::EMObject*		emobj_;
    EM::ImplicitBody*		implicitbody_;
    MultiID			mid_;

    float			insideval_;
    float			outsideval_;
};


}; //namespace

#endif
