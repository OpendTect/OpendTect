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

#include "color.h"
#include "multiid.h"
#include "namedobj.h"
#include "sets.h"
#include "enums.h"
#include "tableascio.h"
#include "trigonometry.h"

class IOPar;

namespace Pick
{

/*!\brief Pick location in space */

mClass Location
{
public:
			Location( double x=0, double y=0, double z=0 )
			: pos(x,y,z), text(0)			{}
			Location( const Coord& c, float f=0 )
			: pos(c,f), text(0)			{}
			Location( const Coord3& c )
			: pos(c), text(0)			{}
			Location( const Coord3& c, const Coord3& d )
			: pos(c), dir(d), text(0)		{}
			Location( const Coord3& c, const Sphere& d )
			: pos(c), dir(d), text(0)		{}
			Location( const Location& pl )
			: text(0)				{ *this = pl; }

			~Location();

    inline bool		operator ==( const Location& pl ) const
			{ return pos == pl.pos && dir == pl.dir; }
    inline bool		operator !=( const Location& pl ) const
			{ return !(*this == pl); }
    void		operator =(const Location&);

    bool		fromString(const char*,bool doxy=true,
	    			  bool checkdir=true);
    			/*!<If checkdir is true, a more rigourous test is done
			    on dir. */
    void		toString(BufferString&,bool forexport=false) const;

    Coord3		pos;
    Sphere		dir; /*!< Optional direction at location.
			          phi is defined as the direction's
				  counter-clockwise angle from the x-axis in
				  the x-y plane.
				  theta is defined as the directions angle from
				  the upward pointing z axis (i.e. opposite to
				  survey-z-axis).
			     \note theta and the radius are defined after thes
			    	  SI().zFactor is applied to the z-coordinate.
			     */
			          
    BufferString*	text; //!<Optional text at location

    void		setText(const char* key,const char* txt);
    void		unSetText(const char* key);
    bool		getText(const char* key,BufferString&) const;

    inline bool		hasDir() const
    			{ return !mIsZero(dir.radius,mDefEps)
			      || !mIsZero(dir.theta,mDefEps)
			      || !mIsZero(dir.phi,mDefEps); }
	
};


/*!\brief Set of picks with something in common */

mClass Set : public NamedObject
	  , public TypeSet<Location>
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
			DeclareEnumUtils(Connection);
	Color		color_;		//!< color
	int		pixsize_;	//!< size in pixels
	int		markertype_;	//!< MarkerStyle3D
	Connection	connect_;	//!< connect picks in set order
    };

    Disp		disp_;
    IOPar&		pars_;

    float		getXYArea() const;
    			//!<Only for closed polygons. Returns in m^2.
    static const char*	sKeyMarkerType()       { return "Marker Type"; }
    void		fillPar(IOPar&) const;
    bool		usePar(const IOPar&);
};


/*!\brief Utility to manage pick set lifecycles.
          Also supports change notifications.
 
 You can create your own set manager for your own special pick sets.
 There is a OD-wide Mgr() available which is supposed to hold all 'plain'
 picksets loaded in the OD-tree.

 A new special-purpose manager is created by passing your own name to the
 static getMgr() method.
 
 */

mClass SetMgr : public NamedObject
{
public:

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
			{ return idx < changed_.size() ? changed_[idx] : false;}
    void		setUnChanged( int idx, bool yn=true )
			{ if ( idx < changed_.size() ) changed_[idx] = !yn; }

    static SetMgr&	getMgr(const char*);

    			SetMgr( const char* nm );
			//!< creates an unmanaged SetMgr
			//!< Normally you don't want that, use getMgr() instead

protected:

    ObjectSet<Set>	pss_;
    TypeSet<MultiID>	ids_;
    BoolTypeSet		changed_;

    void		add(const MultiID&,Set*);
    Set*		find(const MultiID&) const;
    MultiID*		find(const Set&) const;
    Set*		find(const char*) const;

    void		survChg(CallBacker*);
    void		objRm(CallBacker*);
};

inline SetMgr& Mgr()
{
    return SetMgr::getMgr(0);
}


}; // namespace Pick


mClass PickSetAscIO : public Table::AscIO
{
public:
    				PickSetAscIO( const Table::FormatDesc& fd )
				    : Table::AscIO(fd)          {}

    static Table::FormatDesc*   getDesc(bool iszreq);
    static void			updateDesc(Table::FormatDesc&,bool iszreq);
    static void                 createDescBody(Table::FormatDesc*,bool iszreq);

    bool			isXY() const;
    bool			get(std::istream&,Pick::Set&,bool iszreq,
	    			    float zval) const;
    bool                        put(std::ostream&) const;
};

#endif
