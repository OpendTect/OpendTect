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
#include "wellid.h"

class TaskRunner;


namespace Well
{
class Track;

/*!
\brief Marker, should be attached to Strat level.

  Can be unattached, then uses the fallback name, color, and age
  The age is an interval, to support unconformities
*/

mExpClass(Well) Marker : public ::NamedObject
{
public:
			Marker(const char* nm=nullptr,float dh=0.f,
			       OD::Color c=OD::Color());
			Marker(const Strat::LevelID&,float dh);
			Marker(const Marker&);
			~Marker();

    const OD::String&	name() const override;
    const OD::String&	getDatabaseName() const;
    const OD::String&	getStratLvlName() const;
    Marker&		operator =(const Marker&);
    inline bool		operator ==( const Marker& m )
			{ return m.name() == name(); }
    bool		operator > ( const Marker& wm ) const
			{ return dah_ > wm.dah_; }

    inline float	dah() const		{ return dah_; }
    OD::Color		color() const;
    OD::Color		getDatabaseColor() const;
    const Interval<float>& getAgeMa() const	{ return agema_; }
    float		getAgeMa(bool start) const;
    inline Strat::LevelID levelID() const	{ return levelid_; }
    Strat::Level	getLevel() const;
    inline const MarkerID& ID() const		{ return id_; }
    inline const char*	description() const	{ return description_.buf(); }
    bool		hasDescription() const;

    /* setName(), setColor(), setAge only used as fallback,
	if not attached to level  */
    Marker&		setDah(float);
    Marker&		setColor(const OD::Color&);
    Marker&		setAgeMa(float);
    Marker&		setAgeMa(const Interval<float>&);
    Marker&		setLevelID(const Strat::LevelID&);
    Marker&		setNoLevelID();
    Marker&		setID(const MarkerID&);
    Marker&		setDescription(const char*);

    static const char*	sKeyDah();
    static const char*	sKeyAgeMa();

private:

    float			dah_;
    OD::Color			color_	    = OD::Color::Black();
    Interval<float>		agema_ = Interval<float>::udf();
    Strat::LevelID		levelid_;
    mutable BufferString	stratlvlnm_;
    MarkerID			id_;
    BufferString		description_;

    bool			isValidStratLevelName() const;

};


/*!\brief Set of Markers */

mExpClass(Well) MarkerSet : public ManagedObjectSet<Marker>
{
public:

			MarkerSet();
			MarkerSet(const MarkerSet&);
    virtual		~MarkerSet();

    MarkerSet&		operator=(const MarkerSet&);
    void		fillWithAll(TaskRunner* tr=0);

    const Marker*	getByName(const char* nm,bool isdbnm=false) const
			{ return gtByName(nm,isdbnm); }
    Marker*		getByName(const char* nm,bool isdbnm=false)
			{ return gtByName(nm,isdbnm); }
    const Marker*	getByLvlID(Strat::LevelID id) const
			{ return gtByLvlID(id);}
    Marker*		getByLvlID(Strat::LevelID id)
			{ return gtByLvlID(id);}
    int			getIdxAbove(float z,const Well::Track* trck=0) const;
    int			getIdxBelow(float z,const Well::Track* trck=0) const;
			//!< is trck provided, compares TVDs

    bool		isPresent(const char* n,bool isdbnm=false) const
			{ return getByName(n,isdbnm); }
    int			indexOf(const char*,bool isdbnm=false) const;
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
    void		getDatabaseNames(BufferStringSet&) const;
    void		getColors(TypeSet<OD::Color>&) const;
    void		getNamesColorsMDs(BufferStringSet&,TypeSet<OD::Color>&,
					  TypeSet<float>& mds) const;
    void		fillPar(IOPar&) const;
    void		usePar(const IOPar&);

protected:

    virtual ObjectSet<Marker>& doAdd(Marker*) override;
    Marker*		gtByName(const char*,bool isdbnm=false) const;
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
