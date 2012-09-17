#ifndef velocityfunctiongrid_h
#define velocityfunctiongrid_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	K. Tingdahl
 Date:		April 2005
 RCS:		$Id: velocityfunctiongrid.h,v 1.10 2011/08/15 12:28:51 cvskris Exp $
________________________________________________________________________


-*/

#include "binidvalset.h"
#include "samplingdata.h"
#include "thread.h"
#include "velocityfunction.h"

class BinIDValueSet;
class MultiID;
class SeisTrcReader;
class Gridder2D;

namespace Vel
{

class GriddedSource;

/*!A velocity funcion where the velocity is computed from
   Residual Moveout picks. */

mClass GriddedFunction : public Function
{
public:
			GriddedFunction(GriddedSource&);

    StepInterval<float>	getAvailableZ() const;
    bool		moveTo(const BinID&);
    bool		fetchSources();

    bool		isInfluencedBy(const BinID&) const;

    void		setGridder(const Gridder2D&); //!<I will clone
    Gridder2D*		getGridder() { return gridder_; }

protected:
    			~GriddedFunction();
    friend		class GriddedSource;			

    bool		computeVelocity(float z0, float dz, int nr,
					float* res ) const;
    RefMan<const Function> getInputFunction(const BinID& bid,int& source);

    ObjectSet<const Function>		velocityfunctions_;
    TypeSet<int>			sources_;

    Gridder2D*				gridder_;

    mutable TypeSet<float>		gridvalues_;
};


mClass GriddedSource : public FunctionSource
{
public:
    			GriddedSource();
    const VelocityDesc&	getDesc() const;
    const char*		factoryKeyword() const { return sType(); }
    static const char*	sType() { return "GridVelocity"; }

    const Gridder2D*	getGridder() const;
    void		setGridder(Gridder2D*); //!<Becomes mine

    void		setSource(ObjectSet<FunctionSource>&);
    void		setSource(const TypeSet<MultiID>&);
    void		getSources(TypeSet<MultiID>&) const;

    const ObjectSet<FunctionSource>&	getSources() const;

    NotifierAccess*	changeNotifier() { return &notifier_; }
    BinID		changeBinID() const { return changebid_; }

    GriddedFunction*	createFunction();

    void		fillPar(IOPar&) const;
    bool		usePar(const IOPar&);

protected:
    friend		class GriddedFunction;
    GriddedFunction*	createFunction(const BinID&);
    			~GriddedSource();
    bool		initGridder();
    static const char*	sKeyGridder() { return "Gridder"; }

    void		sourceChangeCB(CallBacker*);

    ObjectSet<FunctionSource>		datasources_;

    Notifier<GriddedSource>		notifier_;
    BinID				changebid_;
    Gridder2D*				gridder_;
    bool				gridderinited_;

    BinIDValueSet			sourcepos_;		//All sources

    TypeSet<BinID>			gridsourcebids_;	//Filtered
    TypeSet<Coord>			gridsourcecoords_;	//Filtered
};


}; //namespace

#endif
