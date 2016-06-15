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
#include "iopar.h"
#include "tableascio.h"
template <class T> class ODPolygon;


namespace Pick
{


/*!\brief Monitorable set of picks.

  A Pick::set is either a loose bunch of locations, or a connected set of
  points: a polygon. Apart from this, a set may be labeled to be part of a
  'category', like 'ArrowAnnotations'.

*/

mExpClass(General) Set	: public RefCount::Referenced
			, public NamedMonitorable
{
public:

    typedef TypeSet<Location>::size_type    size_type;
    typedef size_type	IdxType;

			Set(const char* nm=0,const char* category=0);
			mDeclMonitorableAssignment(Set);

    bool		isPolygon() const;
    void		setIsPolygon(bool yn=true);
    void		setCategory(const char*);
    BufferString	type() const; //!< sKey::Polygon() or sKey::PickSet()
    BufferString	category() const;

    size_type		size() const;
    inline bool		isEmpty() const			    { return size()<1; }
    bool		validIdx(IdxType) const;
    Location		get(IdxType) const;
    Coord		getPos(IdxType) const;
    double		getZ(IdxType) const;

    Set&		setEmpty();
    Set&		append(const Set&);
    Set&		set(IdxType,const Location&);
    Set&		add(const Location&);
    Set&		insert(IdxType,const Location&);
    Set&		remove(IdxType);
    inline Set&		operator +=( const Location& loc )
			{ return add( loc ); }
    Set&		setPos(IdxType,const Coord&);
    Set&		setZ(IdxType,double);

    bool		isMultiGeom() const;
    Pos::GeomID		firstGeomID() const;
    bool		has2D() const;
    bool		has3D() const;
    bool		hasOnly2D() const;
    bool		hasOnly3D() const;

    void		getPolygon(ODPolygon<double>&) const; // coords
    void		getPolygon(ODPolygon<float>&) const; // binids
    void		getLocations(TypeSet<Coord>&) const;
    float		getXYArea() const;
			//!<Only for closed polygons. Returns in m^2.
    IdxType		find(const TrcKey&) const;
    IdxType		nearestLocation(const Coord&) const;
    IdxType		nearestLocation(const Coord3&,bool ignorez=false) const;

    mImplSimpleMonitoredGetSet(inline,pars,setPars,IOPar,pars_,0)
    void		fillPar(IOPar&) const;
    bool		usePar(const IOPar&);
    void		updateInPar(const char* ky,const char* val);

    mExpClass(General) Disp
    {
    public:
	enum Connection { None, Open, Close };
			mDeclareEnumUtils(Connection);
			Disp() : connect_(None) {}
	bool		operator ==( const Disp& oth ) const
			{ return connect_ == oth.connect_
			      && mkstyle_ == oth.mkstyle_; }

	Connection		connect_;	//!< connect picks in set order
	OD::MarkerStyle3D	mkstyle_;
    };
    mImplSimpleMonitoredGetSet(inline,getDisp,setDisp,Disp,disp_,cDispChange())
    mImplSimpleMonitoredGetSet(inline,connection,setConnection,
				Disp::Connection,disp_.connect_,cDispChange())
    mImplSimpleMonitoredGetSet(inline,markerStyle,setMarkerStyle,
				OD::MarkerStyle3D,disp_.mkstyle_,cDispChange())
    mImplSimpleMonitoredGetSet(inline,dispColor,setDispColor,
				Color,disp_.mkstyle_.color_,cDispChange())
    mImplSimpleMonitoredGetSet(inline,dispSize,setDispSize,
				int,disp_.mkstyle_.size_,cDispChange())

    static ChangeType	cDispChange()		{ return 2; }
    static ChangeType	cLocationInsert()	{ return 3; }
    static ChangeType	cLocationChange()	{ return 4; }
    static ChangeType	cLocationRemove()	{ return 5; }
    static const char*	sKeyMarkerType()	{ return "Marker Type"; }

    mDeclInstanceCreatedNotifierAccess(Set);
    static const Set&	emptySet()		{ return emptyset_; }
    static Set&		dummySet()		{ return dummyset_; }

protected:

			~Set();

    TypeSet<Location>	locs_;
    Disp		disp_;
    IOPar		pars_;
    static const Set	emptyset_;
    static Set		dummyset_;

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
