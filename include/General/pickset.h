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


/*!\brief Monitorable set of picks. */

mExpClass(General) Set : public NamedMonitorable
{
public:

    typedef TypeSet<Location>::size_type    size_type;
    typedef size_type	IdxType;

			Set(const char* nm=0,const char* category=0);
			Set(const Set&);
			~Set();
    Set&		operator =(const Set&);
    const char*		category() const		    { return category_;}

    size_type		size() const;
    inline bool		isEmpty() const			    { return size()<1; }
    bool		validIdx(IdxType) const;
    Location		get(IdxType) const;
    Coord		getPos(IdxType) const;
    double		getZ(IdxType) const;

    Set&		setEmpty();
    Set&		append(const Set&);
    Set&		set(IdxType,const Location&,bool withundo=false);
    Set&		add(const Location&,bool withundo=false);
    Set&		insert(IdxType,const Location&,bool withundo=false);
    Set&		remove(IdxType,bool withundo=false);
    inline Set&		operator +=( const Location& loc )
			{ return add( loc ); }
    Set&		setPos(IdxType,const Coord&);
    Set&		setZ(IdxType,double);

    bool		isPolygon() const;
    bool		isMultiGeom() const;
    Pos::GeomID		firstGeomID() const;
    bool		has2D() const;
    bool		has3D() const;
    bool		hasOnly2D() const;
    bool		hasOnly3D() const;

    void		getPolygon(ODPolygon<double>&) const;
    void		getLocations(ObjectSet<const Location>&) const;
    float		getXYArea() const;
			//!<Only for closed polygons. Returns in m^2.
    IdxType		find(const TrcKey&) const;
    IdxType		nearestLocation(const Coord&) const;
    IdxType		nearestLocation(const Coord3&,bool ignorez=false) const;

    mImplSimpleMonitoredGetSet(inline,pars,setPars,IOPar,pars_,cDispChange())
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

protected:

    TypeSet<Location>	locs_;
    Disp		disp_;
    IOPar		pars_;
    const BufferString	category_;

    void		addUndoEvent(int,IdxType,const Pick::Location&);
    friend class	LocationUndoEvent;

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
