#pragma once

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

class TrcKeyZSampling;
class uiComboBox;


namespace Pick
{

/*!\brief Set of picks with something in common */

mExpClass(General) Set : public NamedCallBacker, public TypeSet<Location>
{
public:
    typedef idx_type	LocID;

			Set(const char* nm=0);
			Set(const Set&);
			~Set();

    Set&		operator =(const Set&);

    struct Disp
    {
			Disp()
			    : color_(OD::Color::NoColor())
			    , fillcolor_(OD::Color::NoColor())
			    , pixsize_(3)
			    , markertype_(3) // Sphere
			    , dofill_(false)
			    , connect_(None)
			{}
	enum Connection { None, Open, Close };
			mDeclareEnumUtils(Connection)
	OD::Color	color_;		//!< marker color
	OD::Color	fillcolor_;	//!< surface color
	int		pixsize_;	//!< size in pixels
	int		markertype_;	//!< MarkerStyle3D
	bool		dofill_;	//!< Fill?
	OD::LineStyle	linestyle_;		//!< line type
	Connection	connect_;	//!< connect picks in set order
    };

    Disp		disp_;
    IOPar&		pars_;
    bool		is2D() const;
			//!< default is 3D
    Pos::SurvID		getSurvID() const;
			//!< pre-6.0.1 sets will return the survID of first loc

    bool		isPolygon() const;
    void		getPolygon(ODPolygon<double>&,int idx=0) const;
    void		getLocations(ObjectSet<Location>&,int idx=0);
    void		getLocations(ObjectSet<const Location>&,int idx=0)const;
    float		getXYArea(int idx=0) const;
			//!<Only for closed polygons. Returns in m^2.
    LocID		find(const TrcKey&) const;
    LocID		nearestLocation(const Coord&) const;
    LocID		nearestLocation(const Coord3&,bool ignorez=false) const;
    void		getBoundingBox(TrcKeyZSampling&) const;

    static const char*	sKeyMarkerType()	{ return "Marker Type"; }
    static const char*	sKeyFillColor()		{ return "Surface Color"; }
    static const char*	sKeyFill()		{ return "Fill"; }
    static const char*	sKeyConnect()		{ return "Connect"; }

    void		fillPar(IOPar&) const;
    bool		usePar(const IOPar&);
    void		fillDisplayPars(IOPar&) const;
    bool		useDisplayPars(const IOPar&);
    bool		writeDisplayPars() const;

    void		removeSingleWithUndo(LocID);
    void		insertWithUndo(LocID,const Pick::Location&);
    void		appendWithUndo(const Pick::Location&);
    void		moveWithUndo(LocID,const Pick::Location&,
					const Pick::Location&);

    void		bulkAppendWithUndo(const TypeSet<Pick::Location>&,
					   const TypeSet<int>& indexes);
    void		bulkRemoveWithUndo(const TypeSet<Pick::Location>&,
					   const TypeSet<int>& indexes);

    Location&		get(LocID);
    const Location&	get(LocID) const;

    void		setReadOnly( bool yn )		{ readonly_ = yn; }
    bool		isReadOnly() const		{ return readonly_; }

    void		addStartIdx(int locidx);
    void		setStartIdx(int setidx,int locidx);
    int			nrSets() const		{ return startidxs_.size(); }
    void		getStartStopIdx(int setidx,int& start,int&stop) const;
    const TypeSet<int>&	startIndexs() const { return startidxs_; }

    bool		isSizeLargerThanThreshold() const;
    static const char*	sKeyThresholdSize()
			{ return "PointSet Size Threshold";}
    static const char*	sKeyUseThreshold()
			{ return "Use PointSet Size Threshold";}
    static int		getSizeThreshold();

private:

    enum EventType	{ Insert, PolygonClose, Remove, Move };
    void		addUndoEvent(EventType,LocID,const Pick::Location&);
    void		addBulkUndoEvent(EventType,const TypeSet<int>&,
					 const TypeSet<Pick::Location>&);

    TypeSet<int>	startidxs_;
    bool		readonly_;

};
/*!\brief ObjectSet of Pick::Location's. Does not manage. */

/*
mExpClass(General) List : public ObjectSet<Location>
{
public:
    typedef idx_type	LocID;

			List()				{}
			List( const Pick::Set& ps )	{ addAll( ps ); }

    List&		add(const Pick::Location&,bool mkcopy=false);
    inline void		addAll( const Pick::Set& ps )
			{ const_cast<Pick::Set&>(ps).getLocations(*this); }

    inline bool		is2D() const;
    inline Pos::SurvID	getSurvID() const;
    LocID		find(const TrcKey&) const;
    LocID		nearestLocation(const Coord&) const;
    LocID		nearestLocation(const Coord3&,bool ignorez=false) const;

    Location&		get(LocID);
    const Location&	get(LocID) const;

};*/


template <class PicksType>
inline bool is2D( const PicksType& picks )
{
    return ::is2D( picks.getSurvID() );
}

template <class PicksType>
inline Pos::SurvID getSurvID( const PicksType& picks )
{
    return picks.isEmpty() ? OD::GeomSynth : picks.get(0).trcKey().survID();
}
/*
inline bool Pick::List::is2D() const		{ return Pick::is2D( *this ); }
inline Pos::SurvID Pick::List::getSurvID() const
{ return Pick::getSurvID( *this ); }
*/

} // namespace Pick


mExpClass(General) PickSetAscIO : public Table::AscIO
{
public:
				PickSetAscIO( const Table::FormatDesc& fd )
				    : Table::AscIO(fd)          {}

    static Table::FormatDesc*   getDesc(bool iszreq);
    static void			updateDesc(Table::FormatDesc&,bool iszreq);
    static void			createDescBody(Table::FormatDesc*,bool iszreq);

    bool			isXY() const;
    bool			get(od_istream&,Pick::Set&,bool iszreq,
				    float zval) const;
};


/* This include will go away after 6.0 */
#include "picksetmgr.h"


