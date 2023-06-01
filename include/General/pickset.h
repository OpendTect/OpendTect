#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "sharedobject.h"
#include "picklocation.h"
#include "enums.h"
#include "namedobj.h"
#include "trckey.h"
#include "sets.h"
#include "draw.h"
#include "tableascio.h"
template <class T> class ODPolygon;

class DataPointSet;
class TrcKeyZSampling;
class uiComboBox;


namespace Pick
{

/*!\brief Set of picks with something in common */

mExpClass(General) Set : public SharedObject
{
public:
			Set(const char* nm=0);
			Set(const Set&);

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

    int			add(const Location&);
    int			add(const Coord3&);
    int			add(const Coord&,float z);
    void		set(int idx,const Location&);
    void		setPos(int idx,const Coord&);
    void		setPos(int idx,const Coord3&);
    void		setZ(int idx,float z);
    void		setDir(int idx,const Sphere&);
    void		setDip(int idx,float inldip,float crldip);
    void		setKeyedText(int idx,const char* key,const char* txt);
    void		insert(int idx,const Location&);
    void		remove(int idx);
    const Location&	get(int idx) const;
    const Coord3&	getPos(int idx) const;
    float		getZ(int idx) const;
    int			size() const		{ return locations_.size(); }
    bool		isEmpty() const		{ return locations_.isEmpty(); }
    void		setEmpty()		{ locations_.setEmpty(); }
    bool		validIdx(int idx) const;
    bool		setCapacity(int sz);
    bool		append(const Pick::Set&);
    const TypeSet<Location>& locations() const		{ return locations_; }
    void		getLocations(TypeSet<Coord3>&,int setidx=0) const;

    Disp		disp_;
    IOPar&		pars_;
    bool		is2D() const;
			//!< default is 3D
    OD::GeomSystem	geomSystem() const;
			//!< pre-6.0.1 sets will return the survID of first loc

    bool		isPolygon() const;
    void		getPolygon(ODPolygon<double>&,int idx=0) const;
    float		getXYArea(int idx=0) const;
			//!<Only for closed polygons. Returns in m^2.
    int			find(const TrcKey&) const;
    int			nearestLocation(const Coord&) const;
    int			nearestLocation(const Coord3&,bool ignorez=false) const;
    void		getBoundingBox(TrcKeyZSampling&) const;
    bool		fillDataPointSet(DataPointSet&) const;

    static const char*	sKeyMarkerType()	{ return "Marker Type"; }
    static const char*	sKeyFillColor()		{ return "Surface Color"; }
    static const char*	sKeyFill()		{ return "Fill"; }
    static const char*	sKeyConnect()		{ return "Connect"; }

    void		fillPar(IOPar&) const;
    bool		usePar(const IOPar&);
    void		fillDisplayPars(IOPar&) const;
    bool		useDisplayPars(const IOPar&);
    bool		writeDisplayPars() const;
    void		setDefaultDispPars();

    void		removeSingleWithUndo(int);
    void		insertWithUndo(int,const Pick::Location&);
    void		appendWithUndo(const Pick::Location&);
    void		moveWithUndo(int,const Pick::Location&,
					const Pick::Location&);

    void		bulkAppendWithUndo(const TypeSet<Pick::Location>&,
					   const TypeSet<int>& indexes);
    void		bulkRemoveWithUndo(const TypeSet<Pick::Location>&,
					   const TypeSet<int>& indexes);

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

protected:
			~Set();

private:

    enum EventType	{ Insert, PolygonClose, Remove, Move };
    void		addUndoEvent(EventType,int,const Pick::Location&);
    void		addBulkUndoEvent(EventType,const TypeSet<int>&,
					 const TypeSet<Pick::Location>&);

    TypeSet<int>	startidxs_;
    bool		readonly_;

    TypeSet<Location>	locations_;

    void		refNotify() const override;
    void		unRefNotify() const override;

public:

    mDeprecated("use geomSystem")
    OD::GeomSystem	getSurvID() const	{ return geomSystem(); }

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
    inline OD::GeomSystem geomSystem() const;
    LocID		find(const TrcKey&) const;
    LocID		nearestLocation(const Coord&) const;
    LocID		nearestLocation(const Coord3&,bool ignorez=false) const;

    Location&		get(LocID);
    const Location&	get(LocID) const;

};*/


template <class PicksType>
inline bool is2D( const PicksType& picks )
{
    return ::is2D( picks.geomSystem() );
}

template <class PicksType>
mDeprecated("use geomSystem")
inline OD::GeomSystem getSurvID( const PicksType& picks )
{
    return geomSystem( picks );
}


template <class PicksType>
inline OD::GeomSystem geomSystem( const PicksType& picks )
{
    return picks.isEmpty() ? OD::GeomSynth
			   : picks.get(0).trcKey().geomSystem();
}
/*
inline bool Pick::List::is2D() const		{ return Pick::is2D( *this ); }
inline OD::GeomSystem Pick::List::geomSystem() const
{ return Pick::geomSystem( *this ); }
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
