#ifndef wellmarker_h
#define wellmarker_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Bert Bril
 Date:		Aug 2003
 RCS:		$Id$
________________________________________________________________________


-*/

#include "wellmod.h"
#include "color.h"
#include "ranges.h"
#include "namedobj.h"
#include "manobjectset.h"
class IOPar;


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

			Marker( const char* nm=0, float dh=0, Color c=Color() )
			: ::NamedObject(nm)
			, dah_(dh)
			, color_(c)
			, levelid_(-1)		{}
			Marker( int lvlid, float dh )
			    : dah_(dh)
			    , color_(Color::Black())
			    , levelid_(lvlid)	{}
    Marker&		operator =(const Marker&);
    inline bool		operator ==( const Marker& m )
			{ return m.name() == name(); }

    inline float	dah() const		{ return dah_; }
    inline void		setDah( float v )	{ dah_ = v; }
    inline int		levelID() const		{ return levelid_; }
    inline void		setLevelID( int id )	{ levelid_ = id; }
    Color		color() const;

    static const char*	sKeyDah();

    // setName() and setColor() only used as fallback, if not attached to level
    void		setColor( Color col )	{ color_ = col; }
    bool                operator > (const Marker& dm) const 
    			{ return dah_ >= dm.dah_; }

protected:

    float		dah_;
    Color		color_;
    int			levelid_;

};


/*!\brief Set of Markers */

mExpClass(Well) MarkerSet : public ManagedObjectSet<Marker>
{
public:
    			MarkerSet()			{}

    virtual ObjectSet<Marker>& operator +=(Marker*);

    const Marker* 	getByName(const char* nm) const { return gtByName(nm); }
    Marker* 		getByName(const char* nm) 	{ return gtByName(nm); }
    const Marker* 	getByLvlID(int id) const	{ return gtByLvlID(id);}
    Marker* 		getByLvlID(int id) 		{ return gtByLvlID(id);}
    int			getIdxAbove(float z,const Well::Track* trck=0) const;
    			//!< is trck provided, compares TVDs

    bool		isPresent(const char* n) const 	{ return getByName(n); }
    int			indexOf(const char*) const;		  
    bool		insertNew(Well::Marker*); //becomes mine
    virtual void	append(const ObjectSet<Marker>&);

    int			indexOf( const Marker* m ) const
			{ return ObjectSet<Marker>::indexOf(m); }	
    bool		isPresent( const Marker* m ) const
			{ return ObjectSet<Marker>::isPresent(m); }	

    void		getNames(BufferStringSet&) const;
    void		getColors(TypeSet<Color>&) const;
    void		fillPar(IOPar&) const;
    void		usePar(const IOPar&);

protected:

    Marker* 		gtByName(const char*) const;
    Marker* 		gtByLvlID(int) const;
};


/*!\brief Range of markers (typically describing zone of interest) */

mExpClass(Well) MarkerRange
{
public:
    			MarkerRange(const MarkerSet&,
				    Interval<int> idxrg=Interval<int>(-1,-1));

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

protected:

    const MarkerSet&	markers_;
    Interval<int>	rg_;

};


} // namespace Well

#endif

