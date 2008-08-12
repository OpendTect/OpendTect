#ifndef velocitygridder_h
#define velocitygridder_h

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	K. Tingdahl
 Date:		October 2006
 RCS:		$Id: velocitygridder.h,v 1.2 2008-08-12 19:22:30 cvskris Exp $
________________________________________________________________________


-*/

#include "volprocchain.h"
#include "veldesc.h"

class Gridder2D;
namespace Vel
{
    class Function;
    class FunctionSource;
    class GriddedFunction;
    class GriddedSource;
}


namespace VolProc
{

class VelGriddingStep : public VolProc::Step
{
public:
    static void		initClass();

			VelGriddingStep(VolProc::Chain&);
    			~VelGriddingStep();

    const char*		type() const			{ return sType(); }
    const VelocityDesc& outputVelocityType() const;

    void		setPicks(ObjectSet<Vel::FunctionSource>&);
    void		setPicks(const TypeSet<MultiID>&);
    void		getPicks(TypeSet<MultiID>&) const;
    const ObjectSet<Vel::FunctionSource>&	getPicks() const;

    void		setGridder(Gridder2D*); //becomes mine
    const Gridder2D*	getGridder() const;

    bool		needsInput(const HorSampling&) const;
    void		fillPar(IOPar&) const;
    bool		usePar(const IOPar&);

    static const char*	sType()			{ return "Gridding"; }
    static const char*	sUserName()		{ return sType(); }
    static const char*	sKeyType()		{ return "Type"; }
    static const char*	sKeyID()		{ return "ID"; }
    static const char*	sKeyNrSources()		{ return "NrSources"; }

protected:

    bool				prefersBinIDWise() const { return true;}
    bool				computeBinID(const BinID&,int threadid);
    bool				prepareComp(int nrthreads);

    int					addFunction(const BinID&,int);
    void				removeOldFunctions();
    static VolProc::Step*		create(VolProc::Chain&);

    ObjectSet<Vel::Function>		velfuncs_;
    Vel::GriddedSource*			velfuncsource_;
};


}; //namespace

#endif
