#ifndef pickset_h
#define pickset_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	A.H.Bril
 Date:		May 2001
 Contents:	PickSet base classes
 RCS:		$Id$
________________________________________________________________________

-*/

#include "picklocation.h"

#include "color.h"
#include "enums.h"
#include "multiid.h"
#include "namedobj.h"
#include "sets.h"
#include "tableascio.h"
#include "undo.h"
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
			Disp()
			    : color_(Color::NoColor())
			    , markertype_(3) // Sphere
			    , connect_(None)
			    , pixsize_(3)		{}
	enum Connection { None, Open, Close };
			mDeclareEnumUtils(Connection);
	Color		color_;		//!< color
	int		pixsize_;	//!< size in pixels
	int		markertype_;	//!< MarkerStyle3D
	Connection	connect_;	//!< connect picks in set order
    };

    Disp		disp_;
    IOPar&		pars_;
    bool		is2D() const;
			//!<Default is 3D.

    bool		isPolygon() const;
    void		getPolygon(ODPolygon<double>&) const;
    float		getXYArea() const;
			//!<Only for closed polygons. Returns in m^2.
    static const char*	sKeyMarkerType()       { return "Marker Type"; }
    void		fillPar(IOPar&) const;
    bool		usePar(const IOPar&);

    Pos::SurvID		getSurvID() const;
			//!<Only defined for post 6.0.1 sets

    void		removeSingleWithUndo(int idx);
    void		insertWithUndo(int,const Pick::Location&);
    void		appendWithUndo(const Pick::Location&);
    void		moveWithUndo(int,const Pick::Location&,
					const Pick::Location&);
private:
    enum EventType      { Insert, PolygonClose, Remove, Move };
    void		addUndoEvent(EventType,int,const Pick::Location&);

};


/*!\brief Utility to manage pick set lifecycles.
          Also supports change notifications.

 You can create your own set manager for your own special pick sets.
 There is a OD-wide Mgr() available which is supposed to hold all 'plain'
 picksets loaded in the OD-tree.

 A new special-purpose manager is created by passing your own name to the
 static getMgr() method.

 */

mExpClass(General) SetMgr : public NamedObject
{
public:
			~SetMgr();
    int			size() const		{ return pss_.size(); }
    Set&		get( int idx )		{ return *pss_[idx]; }
    const Set&		get( int idx ) const	{ return *pss_[idx]; }
    const MultiID&	id( int idx ) const;

    int			indexOf(const char*) const;
    int			indexOf(const Set&) const;
    int			indexOf(const MultiID&) const;

    // Convenience. Check indexOf() if presence is not sure
    Set&		get( const MultiID& i )		{ return *find(i); }
    const Set&		get( const MultiID& i ) const	{ return *find(i); }
    const MultiID&	get( const Set& s ) const	{ return *find(s); }
    Set&		get( const char* s )		{ return *find(s); }
    const Set&		get( const char* s ) const	{ return *find(s); }

    void		set(const MultiID&,Set*);
			//!< add, replace or remove (pass null Set ptr).
			//!< Set is already, or becomes *mine*
			//!< Note that replacement will trigger two callbacks
    void		setID(int idx,const MultiID&);

    struct ChangeData : public CallBacker
    {
	enum Ev		{ Added, Changed, ToBeRemoved };

			ChangeData( Ev e, const Set* s, int l )
			    : ev_(e), set_(s), loc_(l)		{}

	Ev		ev_;
	const Set*	set_;
	const int	loc_;
			//<refers to the idx in set_
    };

    void		reportChange(CallBacker* sender,const ChangeData&);
    void		reportChange(CallBacker* sender,const Set&);
    void		reportDispChange(CallBacker* sender,const Set&);

    Notifier<SetMgr>	locationChanged;//!< Passes ChangeData*
    Notifier<SetMgr>	setToBeRemoved;	//!< Passes Set*
    Notifier<SetMgr>	setAdded;	//!< passes Set*
    Notifier<SetMgr>	setChanged;	//!< passes Set*
    Notifier<SetMgr>	setDispChanged;	//!< passes Set*
    void		removeCBs(CallBacker*);

    bool		isChanged( int idx ) const
			{ return idx < changed_.size()
				? (bool) changed_[idx] : false;}
    void		setUnChanged( int idx, bool yn=true )
			{ if ( changed_.validIdx(idx) ) changed_[idx] = !yn; }

    Undo&		undo();
    const Undo&		undo() const;

    static SetMgr&	getMgr(const char*);

			SetMgr( const char* nm );
			//!< creates an unmanaged SetMgr
			//!< Normally you don't want that, use getMgr() instead

protected:

    Undo&		undo_;
    ObjectSet<Set>	pss_;
    TypeSet<MultiID>	ids_;
    BoolTypeSet		changed_;

    void		add(const MultiID&,Set*);
    Set*		find(const MultiID&) const;
    MultiID*		find(const Set&) const;
    Set*		find(const char*) const;

    void		survChg(CallBacker*);
    void		objRm(CallBacker*);
    void		removeAll();
};

inline SetMgr& Mgr()
{
    return SetMgr::getMgr(0);
}

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
