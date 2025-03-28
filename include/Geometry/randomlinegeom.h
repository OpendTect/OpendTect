#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "geometrymod.h"
#include "sharedobject.h"

#include "binid.h"
#include "integerid.h"
#include "multiid.h"
#include "ranges.h"
#include "trckeysampling.h"

class TrcKeyZSampling;
class Line2;

namespace Geometry
{

class RandomLineSet;

mExpClass(Geometry) RandomLine : public SharedObject
{
public:
			RandomLine(const char* nm=0);

    RandomLineID	ID() const		{ return id_; }

    int			nrNodes() const;
    int			size() const		{ return nrNodes(); }
    void		removeNode(int);
    bool		isEmpty() const ;
    void		limitTo(const TrcKeyZSampling&); // nrNodes should be 2

    // BinID based functions
    int			addNode(const BinID&);
    void		insertNode(int,const BinID&);
    void		setName(const char*) override;
    void		setNodePosition(int idx,const BinID&,bool moving=false);
    void		setNodePositions(const TypeSet<BinID>&);
    void		removeNode(const BinID&);
    int			nodeIndex(const BinID&) const;
    BinID		nodePosition(int) const;
    void		allNodePositions(TypeSet<TrcKey>&) const;

    // Coord based functions
    int			addNode(const Coord&);
    void		insertNode(int,const Coord&);
    void		setNodePosition(int idx,const Coord&,bool moving=false);
    void		setNodePositions(const TypeSet<Coord>&);
    const Coord&	nodeCoord(int idx) const;
    int			nearestNode(const Coord&,double& distance) const;
    void		allNodePositions(TypeSet<Coord>&) const;

    void		setZRange(const Interval<float>&);
    Interval<float>	zRange() const		{ return zrange_; }

    void		setMultiID(const MultiID&);
    MultiID		getMultiID() const			{ return mid_; }

    void		setLocked( bool yn )		{ locked_ = yn; }
    bool		isLocked() const		{ return locked_; }

    struct ChangeData : public CallBacker
    {
	enum Event	{ Undef, Added, Inserted, Moving, Moved, Removed };

			ChangeData( Event ev=Undef, int nodeidx=-1 )
			    : ev_(ev)
			    , nodeidx_(nodeidx)
			{}
			~ChangeData()
			{}

	Event		ev_;
	int		nodeidx_;
    };

    Notifier<RandomLine>	nameChanged;
    CNotifier<RandomLine,const ChangeData&>	nodeChanged;
    Notifier<RandomLine>	zrangeChanged;

    RandomLineSet*	lineSet()		{ return lset_; }
    const RandomLineSet* lineSet() const	{ return lset_; }

protected:
    virtual		~RandomLine();

    TypeSet<Coord>	nodes_;
    Interval<float>	zrange_;
    MultiID		mid_			= MultiID::udf();
    RandomLineSet*	lset_			= nullptr;
    bool		locked_			= false;

    friend class	RandomLineSet;

private:
    RandomLineID	id_;

public:
    mDeprecated("Use TrcKey")
    void		allNodePositions(TypeSet<BinID>&) const;
    static Coord	getNormal(const TrcKeySet& knots,const TrcKey& pos);
    static int		getNearestPathPosIdx(const TrcKeySet&,
					     const TrcKeySet&,const TrcKey&);

    enum		DuplicateMode { NoDups=0, NoConsecutiveDups, AllDups };
    static void		getPathBids(const TrcKeySet& knots,TrcKeySet& path,
				    DuplicateMode dupmode=NoConsecutiveDups,
				    TypeSet<int>* segments=nullptr);

    mDeprecated("Use TrcKey")
    static void		getPathBids(const TypeSet<BinID>& knots,
				    OD::GeomSystem,
				    TypeSet<BinID>& path,
				    DuplicateMode dupmode=NoConsecutiveDups,
				    TypeSet<int>* segments=nullptr);

    mDeprecated("Use TrcKey and DuplicateMode enum")
    static void		getPathBids(const TypeSet<BinID>& knots,
				    OD::GeomSystem,
				    TypeSet<BinID>& path,
				    bool allowduplicate=false,
				    TypeSet<int>* segments=nullptr);
    mDeprecated("Use TrcKey and DuplicateMode enum")
    static void		getPathBids(const TypeSet<BinID>& knots,
				    TypeSet<BinID>& path,
				    bool allowduplicate=false,
				    TypeSet<int>* segments=nullptr);
};


mExpClass(Geometry) RandomLineSet
{
public:
			RandomLineSet();
			RandomLineSet(const RandomLine&,double dist,
				      bool parallel);
			//!< dist in XY units

    virtual		~RandomLineSet();
    bool		isEmpty() const		{ return lines_.isEmpty(); }
    void		setEmpty();
    RandomLine*		getRandomLine(int);
    const RandomLine*	getRandomLine(int) const;

    int			size() const		{ return lines_.size(); }
    const ObjectSet<RandomLine>& lines() const	{ return lines_; }
    void		removeLine(int idx);
    void		addLine(RandomLine&);
    void		insertLine(RandomLine&,int idx);
    void		limitTo(const TrcKeyZSampling&);

    const IOPar&	pars() const		{ return pars_; }
    IOPar&		pars()			{ return pars_; }

protected:

    ObjectSet<RandomLine>	lines_;
    IOPar&			pars_;

    void		createParallelLines(const Line2& baseline,double dist);

public:
    static void		getGeometry(const MultiID&,TrcKeySet& knots,
				    StepInterval<float>* zrg=nullptr);
    mDeprecated("Use TrcKeySet")
    static void		getGeometry(const MultiID&,TypeSet<BinID>& knots,
				    StepInterval<float>* zrg=nullptr);
};


mExpClass(Geometry) RandomLineManager : public CallBacker
{
public:
			~RandomLineManager();

    RandomLine*		get(const MultiID&);
    RandomLine*		get(const RandomLineID&);
    const RandomLine*	get(const RandomLineID&) const;
    bool		isLoaded(const MultiID&) const;
    bool		isLoaded(const RandomLineID&) const;

    RandomLineID	add(RandomLine*);
    void		remove(RandomLine*);

    CNotifier<RandomLineManager,RandomLineID>	added;
    CNotifier<RandomLineManager,RandomLineID>	removed;


protected:
			RandomLineManager();
    mGlobal(Geometry) friend RandomLineManager& RLM();

    int			indexOf(const MultiID&) const;

    ObjectSet<RandomLine>	lines_;
};

mGlobal(Geometry) RandomLineManager& RLM();

} // namespace Geometry
