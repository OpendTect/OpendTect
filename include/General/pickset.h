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

class SetMgr;


/*!\brief Set of picks with something in common.

  The Set can be managed by a SetMgr. You will have to ask the SetMgr to do
  this, but you can designate the set to be part of a certain set manager.
  By default, this designation is the Pick::Mgr().

*/

mExpClass(General) Set : public NamedMonitorable
{
public:

    typedef TypeSet<Location>::size_type    size_type;

			Set(const char* nm=0,SetMgr* mgr=0);
			Set(const Set&);
			~Set();
    Set&		operator =(const Set&); //!< will not change SetMgr

    size_type		size() const;
    inline bool		isEmpty() const			    { return size()<1; }
    bool		validIdx(size_type) const;
    Location		get(size_type) const;
    Coord		getPos(size_type) const;
    double		getZ(size_type) const;

    Set&		setEmpty();
    Set&		append(const Set&);
    Set&		set(size_type,const Location&,bool withundo=false);
    Set&		add(const Location&,bool withundo=false);
    Set&		insert(size_type,const Location&,bool withundo=false);
    Set&		remove(size_type,bool withundo=false);
    inline Set&		operator +=( const Location& loc )
			{ return add( loc ); }
    Set&		setPos(size_type,const Coord&);
    Set&		setZ(size_type,double);

    void		setSetMgr(SetMgr* mgr=0);
			//!< null for Pick::Mgr(). Will only add to new mgr
			//!< if set was already owned by its old mgr.
    SetMgr&		getSetMgr() const;
			//!< the SetMgr may not actually own this set (yet)

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
    size_type		find(const TrcKey&) const;
    size_type		nearestLocation(const Coord&) const;
    size_type		nearestLocation(const Coord3&,bool ignorez=false) const;

    mImplSimpleMonitoredGetSet(inline,pars,setPars,IOPar,pars_)
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
    mImplSimpleMonitoredGetSet(inline,getDisp,setDisp,Disp,disp_)
    mImplSimpleMonitoredGetSet(inline,connection,setConnection,
				Disp::Connection,disp_.connect_)
    mImplSimpleMonitoredGetSet(inline,markerStyle,setMarkerStyle,
				OD::MarkerStyle3D,disp_.mkstyle_)
    mImplSimpleMonitoredGetSet(inline,dispColor,setDispColor,
				Color,disp_.mkstyle_.color_)
    mImplSimpleMonitoredGetSet(inline,dispSize,setDispSize,
				int,disp_.mkstyle_.size_)

    static const char*	sKeyMarkerType()       { return "Marker Type"; }

protected:

    TypeSet<Location>	locs_;
    Disp		disp_;
    IOPar		pars_;
    mutable SetMgr*	mgr_;

    void		addUndoEvent(int,size_type,const Pick::Location&);
    friend class	SetMgr;
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
