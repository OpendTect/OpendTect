#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "wellmod.h"

#include "manobjectset.h"
#include "ranges.h"
#include "stratlevel.h"

class TaskRunner;


namespace Well
{
class Track;

/*!
\brief Marker, should be attached to Strat level.

  Can be unattached, then uses the fallback name and color.
*/

mExpClass(Well) Marker : public ::NamedObject
{
public:
			Marker(const char* nm=nullptr,float dh=0.f,
			       OD::Color c=OD::Color());
			Marker(Strat::LevelID,float dh);
			Marker(const Marker&);
    virtual		~Marker();

    Marker&		operator =(const Marker&);
    inline bool		operator ==( const Marker& m )
			{ return m.name() == name(); }
    bool		operator > ( const Marker& wm ) const
			{ return dah_ > wm.dah_; }

    inline float	dah() const		{ return dah_; }
    inline void		setDah( float v )	{ dah_ = v; }
    OD::Color		color() const;
    inline Strat::LevelID levelID() const	{ return levelid_; }
    Strat::Level	getLevel() const;

    static const char*	sKeyDah();

    // setName() and setColor() only used as fallback, if not attached to level
    inline void		setColor( OD::Color col )	  { color_ = col; }
    inline void		setLevelID( Strat::LevelID id ) { levelid_ = id; }
    void		setNoLevelID();

protected:

    float		dah_;
    OD::Color		color_;
    Strat::LevelID	levelid_;

};


/*!\brief Set of Markers */

mExpClass(Well) MarkerSet : public ManagedObjectSet<Marker>
{
public:

			MarkerSet();
    virtual		~MarkerSet();

    void		fillWithAll(TaskRunner* tr=0);

    const Marker*	getByName(const char* nm) const { return gtByName(nm); }
    Marker*		getByName(const char* nm)	{ return gtByName(nm); }
    const Marker*	getByLvlID(Strat::LevelID id) const
			{ return gtByLvlID(id);}
    Marker*		getByLvlID(Strat::LevelID id)
			{ return gtByLvlID(id);}
    int			getIdxAbove(float z,const Well::Track* trck=0) const;
    int			getIdxBelow(float z,const Well::Track* trck=0) const;
			//!< is trck provided, compares TVDs

    bool		isPresent(const char* n) const	{ return getByName(n); }
    int			indexOf(const char*) const;
    void		sortByDAH();
    bool		insertNew(Well::Marker*); //becomes mine
    void		addSameWell(const ObjectSet<Marker>&);
    bool		addSameWell(const Marker&);
    void		mergeOtherWell(const ObjectSet<Marker>&);
    virtual void	append( const ObjectSet<Marker>& ms ) override
							{ mergeOtherWell(ms); }

    int			indexOf( const Marker* m ) const override
			{ return ObjectSet<Marker>::indexOf(m); }
    bool		isPresent( const Marker* m ) const override
			{ return ObjectSet<Marker>::isPresent(m); }

    void		getNames(BufferStringSet&) const;
    void		getColors(TypeSet<OD::Color>&) const;
    void		getNamesColorsMDs(BufferStringSet&,TypeSet<OD::Color>&,
					  TypeSet<float>& mds) const;
    void		fillPar(IOPar&) const;
    void		usePar(const IOPar&);

protected:

    virtual ObjectSet<Marker>& doAdd(Marker*) override;
    Marker*		gtByName(const char*) const;
    Marker*		gtByLvlID(Strat::LevelID) const;
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
    virtual		~MarkerRange();

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
			MarkerChgRange(MarkerSet&,
			    const Interval<int>& idxrg=Interval<int>(-1,-1));
			MarkerChgRange(MarkerSet&,const char* m1,
						  const char* m2);
			~MarkerChgRange();

    void		setThickness(float);
    void		remove();

    inline MarkerSet&	getMarkers()
			{ return const_cast<MarkerSet&>(markers_); }
};

} // namespace Well
