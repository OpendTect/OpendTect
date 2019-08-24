#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Nanne Hemstra
 Date:		December 2006
________________________________________________________________________

-*/

#include "geometrymod.h"
#include "namedmonitorable.h"
#include "binid.h"
#include "dbkey.h"
#include "notify.h"
#include "ranges.h"
#include "refcount.h"

class TrcKey;
class TrcKeyPath;
class TrcKeyZSampling;
class Line2;
namespace Survey { class Geometry3D; }

namespace Geometry
{

class RandomLineSet;

mExpClass(Geometry) RandomLine	: public RefCount::Referenced
				, public NamedMonitorable
{
public:

    mUseType( Survey,	Geometry3D );

			RandomLine(const char* nm=0);

    int			ID() const		{ return id_; }

    int			addNode(const BinID&);
    void		insertNode(int,const BinID&);
    void		setName(const char*);
    void		setNodePosition(int idx,const BinID&,bool moving=false);
    void		setNodePositions(const TypeSet<BinID>&);
    void		removeNode(int);
    void		removeNode(const BinID&);
    bool		isEmpty() const		{ return nodes_.isEmpty(); }
    void		limitTo(const TrcKeyZSampling&); // nrNodes should be 2

    int			nodeIndex(const BinID&) const;
    int			nrNodes() const;
    const BinID&	nodePosition(int) const;
    void		getNodePositions(TypeSet<BinID>&) const;
    void		getNodePositions(TrcKeyPath&) const;
    static Coord	getNormal(const TrcKeyPath& knots,const TrcKey& pos);
    static int		getNearestPathPosIdx(const TrcKeyPath&,
					     const TrcKeyPath&,const TrcKey&);

    enum		DuplicateMode { NoDups=0, NoConsecutiveDups, AllDups };
    void		getPathBids(const TypeSet<BinID>& knots,
				    TypeSet<BinID>& path,
				    DuplicateMode dupmode=NoConsecutiveDups,
				    TypeSet<int>* segments=0) const;
    static void		getPathBids(const TypeSet<BinID>& knots,
				    const Geometry3D*,
				    TypeSet<BinID>& path,
				    DuplicateMode dupmode=NoConsecutiveDups,
				    TypeSet<int>* segments=0);

    void		setZRange( const Interval<float>& rg )
			{ zrange_ = rg; zrangeChanged.trigger(); }
    Interval<float>	zRange() const		{ return zrange_; }

    void		setDBKey(const DBKey&);
    DBKey		getDBKey() const		{ return dbky_; }

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

    Notifier<RandomLine>			nameChanged;
    CNotifier<RandomLine,const ChangeData&>	nodeChanged;
    Notifier<RandomLine>			zrangeChanged;

    RandomLineSet*	lineSet()		{ return lset_; }
    const RandomLineSet* lineSet() const	{ return lset_; }

    mDeclInstanceCreatedNotifierAccess(RandomLine);

    const Geometry3D*	survGeom() const	{ return survgeom_; }
    void		setSurvGeom( const Geometry3D* g ) { survgeom_ = g; }

protected:

			~RandomLine();

    TypeSet<BinID>	nodes_;
    Interval<float>	zrange_;
    DBKey		dbky_;
    RandomLineSet*	lset_;
    bool		locked_;
    const Geometry3D*	survgeom_;

    friend class	RandomLineSet;

private:

    int			id_;

};


mExpClass(Geometry) RandomLineSet
{
public:

    mUseType( Survey,	Geometry3D );

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
    const Geometry3D*		survgeom_;

    void		createParallelLines(const Line2& baseline,double dist);

public:
    static void		getGeometry(const DBKey&,TypeSet<BinID>& knots,
				    StepInterval<float>* zrg=0);
};


mExpClass(Geometry) RandomLineManager : public CallBacker
{
public:
			~RandomLineManager();

    RandomLine*		get(const DBKey&);
    RandomLine*		getByName(const char*);
    RandomLine*		get(int id);
    const RandomLine*	get(int id) const;
    bool		isLoaded(const DBKey&) const;
    bool		isLoaded(int id) const;

    int			add(RandomLine*);
    void		remove(RandomLine*);

    CNotifier<RandomLineManager,int>	added;
    CNotifier<RandomLineManager,int>	removed;


protected:
			RandomLineManager();
    mGlobal(Geometry) friend RandomLineManager& RLM();

    int			indexOf(const DBKey&) const;

    ObjectSet<RandomLine>	lines_;
};

mGlobal(Geometry) RandomLineManager& RLM();


} // namespace Geometry
