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
#include "trckey.h"
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
    inline bool		is2D() const;
			//!< default is 3D
    Pos::SurvID		getSurvID() const;
			//!< pre-6.0.1 sets will return the survID of first loc

    bool		isPolygon() const;
    void		getPolygon(ODPolygon<double>&) const;
    void		getLocations(ObjectSet<Location>&);
    void		getLocations(ObjectSet<const Location>&) const;
    float		getXYArea() const;
			//!<Only for closed polygons. Returns in m^2.
    size_type		find(const TrcKey&) const;
    size_type		nearestLocation(const Coord&) const;
    size_type		nearestLocation(const Coord3&,bool ignorez=false) const;

    static const char*	sKeyMarkerType()       { return "Marker Type"; }
    void		fillPar(IOPar&) const;
    bool		usePar(const IOPar&);

    void		removeSingleWithUndo(size_type idx);
    void		insertWithUndo(size_type,const Pick::Location&);
    void		appendWithUndo(const Pick::Location&);
    void		moveWithUndo(size_type,const Pick::Location&,
					const Pick::Location&);

    inline Location&	get( size_type idx )		{ return (*this)[idx]; }
    inline const Location& get( size_type idx ) const	{ return (*this)[idx]; }

private:

    enum EventType      { Insert, PolygonClose, Remove, Move };
    void		addUndoEvent(EventType,size_type,const Pick::Location&);

};

/*!\brief ObjectSet of Pick::Location's. Does not manage. */

mExpClass(General) List : public ObjectSet<Location>
{
public:

			List()				{}
			List( const Pick::Set& ps )	{ addAll( ps ); }

    List&		add(const Pick::Location&,bool mkcopy=false);
    inline void		addAll( const Pick::Set& ps )
			{ const_cast<Pick::Set&>(ps).getLocations(*this); }

    inline bool		is2D() const;
    inline Pos::SurvID	getSurvID() const;
    size_type		find(const TrcKey&) const;
    size_type		nearestLocation(const Coord&) const;
    size_type		nearestLocation(const Coord3&,bool ignorez=false) const;

    inline Location&	get( size_type idx )		{ return *(*this)[idx];}
    inline const Location& get( size_type idx ) const	{ return *(*this)[idx];}

};


template <class PicksType>
inline bool is2D( const PicksType& picks )
{
    return TrcKey::is2D( picks.getSurvID() );
}

template <class PicksType>
inline bool getSurvID( const PicksType& picks )
{
    return picks.isEmpty() ? false : picks.get(0).trcKey().survID();
}

inline bool Pick::Set::is2D() const		{ return Pick::is2D( *this ); }
inline bool Pick::List::is2D() const		{ return Pick::is2D( *this ); }
inline Pos::SurvID Pick::List::getSurvID() const
{ return Pick::getSurvID( *this ); }


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
