#ifndef pickset_h
#define pickset_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Bert
 Date:		May 2001 / Mar 2016
 Contents:	PickSet base classes
________________________________________________________________________

-*/

#include "picklocation.h"
#include "enums.h"
#include "namedobj.h"
#include "sets.h"
#include "draw.h"
#include "tableascio.h"
template <class T> class ODPolygon;


namespace Pick
{

/*!\brief Set of picks with something in common */

mExpClass(General) Set : public NamedObject, public TypeSet<Location>
{
public:

			Set(const char* nm=0);
			Set(const Set&);
			~Set();

    Set&		operator =(const Set&);

    struct Disp
    {
	enum Connection { None, Open, Close };
			mDeclareEnumUtils(Connection);
			Disp() : connect_(None) {}

	Connection		connect_;	//!< connect picks in set order
	OD::MarkerStyle3D	mkstyle_;
    };

    Disp		disp_;
    IOPar&		pars_;
    bool		is2D() const;
			//!<Default is 3D.

    bool		isPolygon() const;
    void		getPolygon(ODPolygon<double>&) const;
    void		getLocations(ObjectSet<Location>&);
    void		getLocations(ObjectSet<const Location>&) const;
    float		getXYArea() const;
			//!<Only for closed polygons. Returns in m^2.
    size_type		nearestLocation(const Coord&) const;
    size_type		nearestLocation(const Coord3&,bool ignorez=false) const;

    static const char*	sKeyMarkerType()       { return "Marker Type"; }
    void		fillPar(IOPar&) const;
    bool		usePar(const IOPar&);

    Pos::SurvID		getSurvID() const;
			//!<Only defined for post 6.0.1 sets

    void		removeSingleWithUndo(size_type idx);
    void		insertWithUndo(size_type,const Pick::Location&);
    void		appendWithUndo(const Pick::Location&);
    void		moveWithUndo(size_type,const Pick::Location&,
					const Pick::Location&);

private:

    enum EventType      { Insert, PolygonClose, Remove, Move };
    void		addUndoEvent(EventType,size_type,const Pick::Location&);

};


/*!\brief List of locations managed by a PickSet */

mExpClass(General) List : OD::Set
{
public:

    typedef ObjectSet<Location>::size_type   size_type;

				List(Pick::Set&,bool addall=true);
				List(const Pick::Set&,bool addall=true);

    inline Location&		location( int idx ) { return *locs_[idx]; }
    inline const Location&	location( int idx ) const
    				{ return *locations()[idx]; }
    inline List&		add( int srcidx )
				{ locs_ += &set_[srcidx]; return *this; }

    ObjectSet<Location>&	locations();
    ObjectSet<const Location>&	locations() const;

    Pick::Set&			source();
    const Pick::Set&		source() const	    { return set_; }

    inline size_type		size() const	    { return locs_.size(); }

				// OD::Set interface
    virtual od_int64		nrItems() const	    { return size(); }
    virtual bool		validIdx( od_int64 i ) const
						    { return locs_.validIdx(i);}
    virtual void		swap( od_int64 i1, od_int64 i2 )
						    { locs_.swap(i1,i2);}
    virtual void		erase()		    { locs_.erase(); }

    void			reFill();

protected:

    const bool		isconst_;
    ObjectSet<Location>	locs_;
    Pick::Set&		set_;

};

} // namespace Pick


mExpClass(General) PickSetAscIO : public Table::AscIO
{
public:
				PickSetAscIO( const Table::FormatDesc& fd )
				    : Table::AscIO(fd)          {}

    static Table::FormatDesc*   getDesc(bool iszreq);
    static void			updateDesc(Table::FormatDesc&,bool iszreq);
    static void                 createDescBody(Table::FormatDesc*,bool iszreq);

    bool			isXY() const;
    bool			get(od_istream&,Pick::Set&,bool iszreq,
				    float zval) const;
};


#endif
