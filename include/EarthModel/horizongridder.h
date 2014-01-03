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
    mDefineFactoryInClass(HorizonGridder,factory);

    const char*		infoMsg() const	{ return infomsg_.buf(); }

    void		setFaultIds(const TypeSet<MultiID>&);

    virtual void	setHorSampling(const HorSampling&);
    virtual bool	setArray2D(Array2D<float>&,TaskRunner* =0);

    static const char*	sKeyMethod();
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

    bool		init(TaskRunner*);
    bool		blockSrcPoints(const float*,const od_int64*,int,
	    			       ObjectSet< TypeSet<int> >&) const;
    bool		setFrom(float*,od_int64,const od_int64*,
	    			const float*,int nrsrc);
};


mExpClass(EarthModel) InvDistHor3DGridder
	: public InverseDistanceArray2DInterpol, public HorizonGridder
{
public:

			mDefaultFactoryInstantiation( HorizonGridder,
				InvDistHor3DGridder,
				"Inverse Distance", sFactoryKeyword() )

    virtual void	setHorSampling(const HorSampling&);
    virtual bool	setArray2D(Array2D<float>&,TaskRunner* =0);

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

			mDefaultFactoryInstantiation( HorizonGridder,
				TriangulationHor3DGridder,
				"Triangulation", sFactoryKeyword() )

    virtual void	setHorSampling(const HorSampling&);
    virtual bool	setArray2D(Array2D<float>&,TaskRunner* =0);

    bool		fillPar(IOPar&) const;
    bool		usePar(const IOPar&);

protected:

    bool		initFromArray(TaskRunner*);
    void		setFrom(od_int64 target, const od_int64* sources,
	                        const float* weights, int nrsrc);
};


mExpClass(EarthModel) ExtensionHor3DGridder
	: public ExtensionArray2DInterpol, public HorizonGridder
{
public:

			mDefaultFactoryInstantiation( HorizonGridder,
				ExtensionHor3DGridder,
				"Extension", sFactoryKeyword() )

    virtual void	setHorSampling(const HorSampling&);
    virtual bool	setArray2D(Array2D<float>&,TaskRunner* =0);

    bool		fillPar(IOPar&) const;
    bool		usePar(const IOPar&);

protected:

};



#endif
