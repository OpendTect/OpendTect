#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Nanne Hemstra
 Date:		December 2006
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

    int			addNode(const BinID&);
    void		insertNode(int,const BinID&);
    void		setName(const char*) override;
    void		setNodePosition(int idx,const BinID&,bool moving=false);
    void		setNodePositions(const TypeSet<BinID>&);
    void		removeNode(int);
    void		removeNode(const BinID&);
    bool		isEmpty() const		{ return nodes_.isEmpty(); }
    void		limitTo(const TrcKeyZSampling&); // nrNodes should be 2

    int			nodeIndex(const BinID&) const;
    int			nrNodes() const;
    const BinID&	nodePosition(int) const;
    void		allNodePositions(TrcKeyPath&) const;

    void		setZRange( const Interval<float>& rg )
			{ zrange_ = rg; zrangeChanged.trigger(); }
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

    TypeSet<BinID>	nodes_;
    Interval<float>	zrange_;
    MultiID		mid_;
    RandomLineSet*	lset_;
    bool		locked_;

    friend class	RandomLineSet;

private:
    RandomLineID	id_;

public:
    mDeprecated("Use TrcKey")
    void		allNodePositions(TypeSet<BinID>&) const;
    static Coord	getNormal(const TrcKeyPath& knots,const TrcKey& pos);
    static int		getNearestPathPosIdx(const TrcKeyPath&,
					     const TrcKeyPath&,const TrcKey&);

    enum		DuplicateMode { NoDups=0, NoConsecutiveDups, AllDups };
    static void		getPathBids(const TrcKeyPath& knots,TrcKeyPath& path,
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
    static void		getGeometry(const MultiID&,TrcKeyPath& knots,
				    StepInterval<float>* zrg=nullptr);
    mDeprecated("Use TrcKeyPath")
    static void		getGeometry(const MultiID&,TypeSet<BinID>& knots,
				    StepInterval<float>* zrg=nullptr);
};


mExpClass(Geometry) RandomLineManager : public CallBacker
{
public:
			~RandomLineManager();

    RandomLine*		get(const MultiID&);
    RandomLine*		get(RandomLineID);
    const RandomLine*	get(RandomLineID) const;
    bool		isLoaded(const MultiID&) const;
    bool		isLoaded(RandomLineID) const;

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
