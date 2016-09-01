#ifndef wellmarker_h
#define wellmarker_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Bert Bril
 Date:		Aug 2003
________________________________________________________________________


-*/

#include "wellcommon.h"
#include "sharedobject.h"
#include "color.h"
#include "ranges.h"
#include "manobjectset.h"
#include "stratlevel.h"

class TaskRunner;



namespace Well
{

//TODO remove when these objects become true shared objects
typedef Monitorable::DirtyCountType DirtyCountType;

/*!\brief Marker, can be attached to Strat level. When not attached, uses
  the object's own (fallback) name and color. */

mExpClass(Well) Marker : public ::SharedObject
{
public:

    typedef Strat::Level::ID	LevelID;
    typedef float		ZType;

			Marker(const char*,ZType d=0.f,Color c=Color());
			Marker(LevelID,ZType);
			~Marker();
			mDeclInstanceCreatedNotifierAccess(Marker);
			mDeclAbstractMonitorableAssignment(Marker);
    inline bool		operator ==(const Marker&) const;
    bool                operator >(const Marker&) const;

    virtual const OD::String& name() const;
    virtual BufferString getName() const;

    mImplSimpleMonitoredGetSet(inline,dah,setDah,ZType,dah_,cDahChange());
    mImplSimpleMonitoredGetSet(inline,levelID,setLevelID,LevelID,levelid_,
							cLevelChange());
    Color		color() const;
    mImplSimpleMonitoredSet(setColor,Color,color_,cColorChange());

    Strat::Level	getLevel() const;
    void		setNoLevelID();

    static const char*	sKeyDah();
    static ChangeType	cColorChange()		{ return 2; }
    static ChangeType	cLevelChange()		{ return 3; }
    static ChangeType	cDahChange()		{ return 4; } // like DahObj

protected:

    float		dah_;
    Color		color_;
    LevelID		levelid_;

			// Usable in locked state
    BufferString	gtName() const;
    Strat::Level	gtLevel() const;

};


/*!\brief Set of Markers */

mExpClass(Well) MarkerSet : public ManagedObjectSet<Marker>
{
public:

    typedef Marker::LevelID	LevelID;

			MarkerSet()			{}
    void		fillWithAll(TaskRunner* tr=0);

    const Marker*	getByName(const char* nm) const { return gtByName(nm); }
    Marker*		getByName(const char* nm)	{ return gtByName(nm); }
    const Marker*	getByLvlID(LevelID id) const	{ return gtByLvlID(id);}
    Marker*		getByLvlID(LevelID id)		{ return gtByLvlID(id);}
    int			getIdxAbove(float z,const Well::Track* trck=0) const;
			//!< is trck provided, compares TVDs

    bool		isPresent(const char* n) const	{ return getByName(n); }
    int			indexOf(const char*) const;
    bool		insertNew(Well::Marker*); //becomes mine
    void		addSameWell(const ObjectSet<Marker>&);
    void		mergeOtherWell(const ObjectSet<Marker>&);
    virtual void	append( const ObjectSet<Marker>& ms )
							{ mergeOtherWell(ms); }

    int			indexOf( const Marker* m ) const
			{ return ObjectSet<Marker>::indexOf(m); }
    bool		isPresent( const Marker* m ) const
			{ return ObjectSet<Marker>::isPresent(m); }

    void		getNames(BufferStringSet&) const;
    void		getColors(TypeSet<Color>&) const;
    void		fillPar(IOPar&) const;
    void		usePar(const IOPar&);

    //TODO remove when object becomes a true NamedMonitorable
    void touch() const {}
    DirtyCountType dirtyCount() const { return 0; }

protected:

    virtual ObjectSet<Marker>& doAdd(Marker*);
    Marker*		gtByName(const char*) const;
    Marker*		gtByLvlID(LevelID) const;
    void		addCopy(const ObjectSet<Marker>&,int,float);
    void		alignOrderingWith(const ObjectSet<Marker>&);
    void		moveBlock(int,int,const TypeSet<int>&);
    void		insertNewAfter(int,ObjectSet<Marker>&);

};


/*!\brief Range of markers (typically describing zone of interest) */

mExpClass(Well) MarkerRange
{
public:

			MarkerRange(const MarkerSet&,
			    const Interval<int>& idxrg=Interval<int>(-1,-1));
			MarkerRange(const MarkerSet&,
				const char*,const char*);

    inline int		size() const		{ return rg_.width(false) + 1; }
    bool		isValid() const;

    inline bool		isIncluded( int i ) const
						{ return rg_.includes(i,false);}
    bool		isIncluded(const char*) const;
    bool		isIncluded(float z) const;

    void		getNames(BufferStringSet&) const;
    MarkerSet*		getResultSet() const; //!< returns new set


    const MarkerSet&	markers() const		{ return markers_; }
    const Interval<int>& idxRange() const	{ return rg_; }
    Interval<int>&	idxRange()		{ return rg_; }
    float		thickness() const;

protected:

    const MarkerSet&	markers_;
    Interval<int>	rg_;
    bool		isconst_;

    void		init(const Interval<int>&);

};


/*!\brief MarkerRange that can change its MarkerSet  */

mExpClass(Well) MarkerChgRange : public MarkerRange
{
public:

			MarkerChgRange( MarkerSet& ms,
			    const Interval<int>& idxrg=Interval<int>(-1,-1) )
			    : MarkerRange(ms,idxrg)	{}
			MarkerChgRange( MarkerSet& ms, const char* m1,
							const char* m2 )
			    : MarkerRange(ms,m1,m2)	{}

    void		setThickness(float);
    void		remove();

    inline MarkerSet&	getMarkers()
			{ return const_cast<MarkerSet&>(markers_); }

};


} // namespace Well

#endif
