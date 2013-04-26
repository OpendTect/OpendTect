#ifndef horizongridder_h
#define horizongridder_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Raman K Singh
 Date:          Mar 2013
 RCS:           $Id$
________________________________________________________________________

-*/


#include "array2dinterpolimpl.h"
#include "horsampling.h"
#include "multiid.h"
#include "earthmodelmod.h"

class FaultTrcDataProvider;
class TaskRunner;

/*!
\brief Base class for Horizon Gridders.
*/

mExpClass(EarthModel) HorizonGridder
{
public:

    const char*		infoMsg() const	{ return infomsg_.buf(); }

    void		setFaultIds(const TypeSet<MultiID>&);

    static const char*	sKeyNrFaults();
    static const char*	sKeyFaultID();
    
    virtual bool	fillPar(IOPar&) const;
    virtual bool	usePar(const IOPar&);

protected:

    			HorizonGridder();
			~HorizonGridder();

    FaultTrcDataProvider* fltdataprov_;

    BufferString	infomsg_;
    HorSampling		hs_;
    TypeSet<MultiID>	faultids_;

    bool		init(const HorSampling&,TaskRunner*);
    bool		blockSrcPoints(const float*,const od_int64*,int,
	    			       ObjectSet< TypeSet<int> >&) const;
    bool		setFrom(float*,od_int64,const od_int64*,
	    			const float*,int nrsrc);
};


mExpClass(EarthModel) InvDistHor3DGridder
	: public InverseDistanceArray2DInterpol, public HorizonGridder
{
public:

			mDefaultFactoryInstantiation( Array2DInterpol,
				InvDistHor3DGridder,
				"InvDistWithFaults", sFactoryKeyword() )

    bool		fillPar(IOPar&) const;
    bool		usePar(const IOPar&);

protected:

    bool		initFromArray(TaskRunner*);
    void		setFrom(od_int64 target, const od_int64* sources,
	                        const float* weights, int nrsrc);
};


mExpClass(EarthModel) TriangulationHor3DGridder
	: public TriangulationArray2DInterpol, public HorizonGridder
{
public:

			mDefaultFactoryInstantiation( Array2DInterpol,
				TriangulationHor3DGridder,
				"TriangulationWithFaults", sFactoryKeyword() )

    bool		fillPar(IOPar&) const;
    bool		usePar(const IOPar&);

protected:

    bool		initFromArray(TaskRunner*);
    void		setFrom(od_int64 target, const od_int64* sources,
	                        const float* weights, int nrsrc);
};


#endif
