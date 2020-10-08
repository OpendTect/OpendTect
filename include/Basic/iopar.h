#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Bert
 Date:		21-12-1995
________________________________________________________________________

-*/

#include "basicmod.h"
#include "namedobj.h"
#include "fixedstring.h"
#include "samplingdata.h"
#include "idxpair.h"
#include "integerid.h"
#include "groupedid.h"
#include "geometry.h"
#include "od_iosfwd.h"

class ascistream;
class ascostream;
class BufferStringSet;
class DBKeySet;
class SeparString;
class TrcKey;
class uiString;
class GeomIDSet;
namespace Pos { class GeomID; }
namespace OD
{
    namespace JSON
    {
	class Object;
    };
};

/*!
\brief Generalized set of parameters of the keyword-value type.

  Part of the function of this class is as in an STL map<string,string>.
  Passing a keyword will return the appropriate value.

  Tools around this basic idea are paring into other types, key composition,
  reading/writing to/from file, merging, and more.

  The get() functions return a bool, which is false when:
  * either the key is not present
  * or the value is not acceptable (usually: when empty is not OK,
    like for numerical values).
  In that case the passed variable is untouched; it keeps its original value.

  dumpPretty() is used for reports.  The title of the report is the name of the
  IOPar. If sKeyHdr and sKeySubHdr are the key, there will be a (sub)header
  with the value. Use add() rather than set(). Values may contain newlines.

  A lot of classes will be served by template functions, like BinID, RowCol,
  Coord, ...

*/

mExpClass(Basic) IOPar : public NamedObject
{
public:
			IOPar(const char* nm=nullptr); //!< empty
			IOPar(ascistream&);
			IOPar(const IOPar&);
			~IOPar();
    IOPar&		operator =(const IOPar&);
    inline bool		operator ==( const IOPar& iop ) const
			{ return isEqual(iop); }
    inline bool		operator !=( const IOPar& iop ) const
			{ return !isEqual(iop); }

    int			size() const;
    int			indexOf(const char* key) const;
    inline bool		isPresent( const char* ky ) const
						{ return indexOf(ky) >= 0; }

    inline bool		isEmpty() const		{ return size() == 0; }
    bool		isEqual(const IOPar&,bool need_same_order=false) const;
    bool		includes(const IOPar&) const;
    FixedString		getKey(int) const;
    FixedString		getValue(int) const;
    bool		setKey(int,const char*);
    void		setValue(int,const char*);
    void		sortOnKeys();
    int			maxContentSize(bool keys_else_values) const;

    inline bool		hasKey( const char* s ) const { return isPresent(s); }
    const char*		findKeyFor(const char*,int nr=0) const;
				//!< returns null if value not found
    void		fillJSON(OD::JSON::Object& obj);
			//!< only save the top level objects

    void		remove(int);
    void		removeWithKey(const char* key);
    void		removeWithKeyPattern(const char* globexpression);
				//!< removes all entries with key matching
				//!< this glob expression

    void		setEmpty();
			//!< remove all entries (doesn't clear name)
    void		merge(const IOPar&);
			//!< merge entries using the set() command
    void		addFrom(const IOPar&);
			//!< merge entries but do not overwrite
    static const char*	compKey(const char*,const char*);
			//!< The composite key: (a,b) -> a.b
    static const char*	compKey(const char*,int);
			//!< The composite key where int will be --> string
    static const char*	compKey( const char* ky1, const OD::String& ky2 )
			{ return compKey(ky1,ky2.str()); }
    bool		hasSubSelection(const char*) const;
    bool		hasSubSelection(int) const;
    bool		hasSubSelection( const OD::String& str ) const
			{ return hasSubSelection( str.str() ); }
    IOPar*		subselect(const char*) const;
			//!< returns iopar with key that start with <str>.
    IOPar*		subselect(int) const;
			//!< returns iopar with key that start with number.
    IOPar*		subselect( const OD::String& str ) const
			{ return subselect( str.str() ); }
    void		removeSubSelection(const char*);
			//!< removes with key that start with <str>.
    void		removeSubSelection(int);
			//!< removes with key that start with number.
    void		removeSubSelection( const OD::String& fs )
			{ removeSubSelection( fs.str() ); }
    void		mergeComp(const IOPar&,const char*);
			//!< merge entries, where IOPar's entries get a prefix
    void		updateComp(const IOPar&,const char*);
			//!< use update to comp-merge. May remove stuff.

// GET functions

    const char*		find(const char*) const;
				//!< returns null if not found
    FixedString		operator[]( const char* ky ) const
			{ return FixedString( find(ky) ); }

			// Functions for getting 1,2,3 and 4 of the same type
#define mIOParDeclFns(type) \
    bool		get(const char*,type&) const; \
    bool		get(const char*,type&,type&) const; \
    bool		get(const char*,type&,type&,type&) const; \
    bool		get(const char*,type&,type&,type&,type&) const

			mIOParDeclFns(od_int16);
			mIOParDeclFns(od_uint16);
			mIOParDeclFns(od_int32);
			mIOParDeclFns(od_uint32);
			mIOParDeclFns(od_int64);
			mIOParDeclFns(od_uint64);
			mIOParDeclFns(float);
			mIOParDeclFns(double);
#undef mIOParDeclFns
    bool		getYN(const char*,bool&) const;
    bool		getYN(const char*,bool&,bool&) const;
    bool		getYN(const char*,bool&,bool&,bool&) const;
    bool		getYN(const char*,bool&,bool&,bool&,bool&) const;
    inline bool		isTrue( const char* key ) const
			{ bool is = false; return getYN(key,is) && is; }
    inline bool		isFalse( const char* key ) const
			{ bool is = true; return getYN(key,is) && !is; }


    bool		get(const char*,int&,int&,float&) const;

    bool		get(const char*,BoolTypeSet&) const;
    bool		get(const char*,TypeSet<od_int16>&) const;
    bool		get(const char*,TypeSet<od_uint16>&) const;
    bool		get(const char*,TypeSet<od_int32>&) const;
    bool		get(const char*,TypeSet<od_uint32>&) const;
    bool		get(const char*,TypeSet<od_int64>&) const;
    bool		get(const char*,TypeSet<od_uint64>&) const;
    bool		get(const char*,TypeSet<double>&) const;
    bool		get(const char*,TypeSet<float>&) const;
    bool		get(const char*,GeomIDSet&) const;

    template <class iT>
    inline bool		get(const char*,IntegerID<iT>&) const;
    template <class fT>
    inline bool		get(const char*,Geom::Point2D<fT>&) const;
    template <class fT>
    inline bool		get(const char*,Geom::Point3D<fT>&) const;
    template <class T>
    inline bool		get(const char*,Interval<T>&) const;
    template <class T>
    inline bool		get(const char*,SamplingData<T>&) const;
    template <class T1,class T2>
    inline bool		get(const char*,std::pair<T1,T2>&) const;
    template <class T>
    inline bool		get(const char*,Twins<T>&) const;
    template <class T>
    inline bool		get(const char*,Triplets<T>&) const;

    bool		get(const char*,TrcKey&) const;
    bool		get(const char*,DBKey&) const;
    bool		get(const char*,Pos::GeomID&) const;
    bool		get(const char*,Color&) const;
    bool		get(const char*,SeparString&) const;
    bool		get(const char*,BufferString&) const;
    bool		get(const char*,BufferString&,BufferString&) const;
    bool		get(const char*,BufferString&,BufferString&,
					BufferString&) const;
    bool		get(const char*,BufferStringSet&) const;
    bool		get(const char*,DBKeySet&) const;

    bool		getPtr(const char*,void*&) const;

#define mIOParDeclFns(type) \
    bool		getScaled(const char*,type&,type applied_scale, \
			      bool set_to_undef_if_not_found) const; \
    bool		getScaled(const char*,type&,type&,type,bool) const; \
    bool		getScaled(const char*,type&,type&,type&,type, \
				  bool) const; \
    bool		getScaled(const char*,type&,type&,type&,type&,type, \
			      bool) const
			mIOParDeclFns(float);
			mIOParDeclFns(double);
#undef mIOParDeclFns


// SET functions

    void		set(const char* ky,const char* val);
			/*!< replaces when key already exists, otherwise adds */
    void		add(const char* ky,const char* val);
			/*!< unsafe: does not check for duplicate keys */
    void		add( const char* ky, const OD::String& val )
			{ add( ky, val.str() ); }
    void		update(const char* ky,const char* val);
			/*!< removes if val is empty or null */

			// Functions for 1,2,3 and 4 of the same type
#define mIOParDeclFns(fnnm,type) \
    void		fnnm(const char*,type); \
    void		fnnm(const char*,type,type); \
    void		fnnm(const char*,type,type,type); \
    void		fnnm(const char*,type,type,type,type)

			mIOParDeclFns(set,od_int16);
			mIOParDeclFns(set,od_uint16);
			mIOParDeclFns(set,od_int32);
			mIOParDeclFns(set,od_uint32);
			mIOParDeclFns(set,od_int64);
			mIOParDeclFns(set,od_uint64);
			mIOParDeclFns(set,float);
			mIOParDeclFns(set,double);
			mIOParDeclFns(add,od_int16);
			mIOParDeclFns(add,od_uint16);
			mIOParDeclFns(add,od_int32);
			mIOParDeclFns(add,od_uint32);
			mIOParDeclFns(add,od_int64);
			mIOParDeclFns(add,od_uint64);
			mIOParDeclFns(add,float);
			mIOParDeclFns(add,double);
#undef mIOParDeclFns
#define mIOParDeclYNFns(fnnm) \
    void		fnnm##YN(const char*,bool); \
    void		fnnm##YN(const char*,bool,bool); \
    void		fnnm##YN(const char*,bool,bool,bool); \
    void		fnnm##YN(const char*,bool,bool,bool,bool)
			mIOParDeclYNFns(set);
			mIOParDeclYNFns(add);
#undef mIOParDeclYNFns

    template<class iT>
    inline void		set(const char*,const IntegerID<iT>&);
    template <class fT>
    inline void		set(const char*,const Geom::Point2D<fT>&);
    template <class fT>
    inline void		set(const char*,const Geom::Point3D<fT>&);
    template <class T>
    inline void		set(const char*,const Interval<T>&);
    template <class T>
    inline void		set(const char*,const SamplingData<T>&);
    template <class T1,class T2>
    inline void		set(const char*,const std::pair<T1,T2>&);
    template <class T>
    inline void		set(const char*,const Twins<T>&);
    template <class T>
    inline void		set(const char*,const Triplets<T>&);

    template<class iT>
    inline void		update(const char*,const IntegerID<iT>&);
    void		update(const char*,const DBKey&);
    void		update(const char*,const Pos::GeomID&);

    void		set(const char*,int,int,float);
    void		setPtr(const char*,void*);

    void		set(const char*,const char*,const char*);
    void		set(const char*,const char*,const char*,const char*);
    void		set(const char*,const TrcKey&);
    void		set(const char*,const DBKey&);
    void		set(const char*,const Pos::GeomID&);
    void		set(const char*,const Color&);
    void		set(const char*,const SeparString&);
    void		set(const char*,const OD::String&);
    void		set(const char*,const OD::String&,
					const OD::String&);
    void		set(const char*,const OD::String&,
					const OD::String&,
					const OD::String&);
    void		set(const char*,const BufferStringSet&);
    void		set(const char*,const DBKeySet&);

    void		set(const char*,const BoolTypeSet&);
    void		set(const char*,const TypeSet<od_int16>&);
    void		set(const char*,const TypeSet<od_uint16>&);
    void		set(const char*,const TypeSet<od_int32>&);
    void		set(const char*,const TypeSet<od_uint32>&);
    void		set(const char*,const TypeSet<od_int64>&);
    void		set(const char*,const TypeSet<od_uint64>&);
    void		set(const char*,const TypeSet<double>&);
    void		set(const char*,const TypeSet<float>&);
    void		set(const char*,const GeomIDSet&);

    void		setToDateTime(const char* ky=nullptr);
    void		setToUser(const char* ky=nullptr);
    void		setStdCreationEntries();

			// Do you want to use these? Probably not. Think twice!
    bool		getUiString(const char*,uiString&) const;
    void		setUiString(const char*,const uiString&);

// I/O  functions

    // to/from string: 'serialisation'
    void		getFrom(const char*);
    void		getParsFrom(const char*);
    void		putTo(BufferString&) const;
    void		putParsTo(BufferString&) const;

    // to/from file
    void		getFrom(ascistream&);
    void		putTo(ascostream&,bool endparagraph=true) const;
    bool		read(const char* filename,const char* filetype,
				bool chktype=false);
			//!< filetype null will assume no file header
			//!< uses set(). no clear() done
    bool		read(od_istream&,const char* filetype,
				bool chktype=false);
    bool		write(const char* filename,const char* filetype) const;
			//!< If filetype is set to null no ascstream header
			//!< sKeyDumpPretty calls dumpPretty.
    bool		write(od_ostream&,const char* filetyp) const;
    int			majorVersion() const	{ return majorversion_; }
			//!<Only set if read from file. Otherwise set to current
    int			minorVersion() const	{ return minorversion_; }
			//!<Only set if read from file. Otherwise set to current
    int			odVersion() const;
			/*!<Only set if read from file. Otherwise set to current
			    v4.6.0 returns as 460 */

    void		dumpPretty(BufferString&) const;
    void		dumpPretty(od_ostream&) const;

    void		collectIDs(TypeSet<int>&) const;

    static const char*	sKeyDumpPretty()         { return "_pretty"; }
    static const char*	sKeyHdr()		 { return "->";	     }
    static const char*	sKeySubHdr()		 { return "-->";     }

protected:

    int			majorversion_;
    int			minorversion_;

    BufferStringSet&	keys_;
    BufferStringSet&	vals_;

    void		doMergeComp(const IOPar&,const char*,bool);

};


template <class T>
inline bool IOPar::get( const char* keyw, Interval<T>& intv ) const
{
    mDynamicCastGet(StepInterval<T>*,si,&intv)
    return si ? get( keyw, intv.start, intv.stop, si->step )
	      : get( keyw, intv.start, intv.stop );
}


template <class T>
inline void IOPar::set( const char* keyw, const Interval<T>& intv )
{
    mDynamicCastGet(const StepInterval<T>*,si,&intv)
    if ( si )	set( keyw, intv.start, intv.stop, si->step );
    else	set( keyw, intv.start, intv.stop );
}


template <class T>
inline bool IOPar::get( const char* keyw, SamplingData<T>& sd ) const
{
    return get( keyw, sd.start, sd.step );
}


template <class T>
inline void IOPar::set( const char* keyw, const SamplingData<T>& sd )
{
    set( keyw, sd.start, sd.step );
}


template<class iT>
bool IOPar::get( const char* keyw, IntegerID<iT>& id ) const
{
    iT idio = id.getI();
    const bool rv = get( keyw, idio );
    if ( rv )
	id.setI( idio );
    return rv;
}


template<class iT>
void IOPar::set( const char* keyw, const IntegerID<iT>& id )
{
    set( keyw, id.getI() );
}


template<class iT>
void IOPar::update( const char* keyw, const IntegerID<iT>& id )
{
    if ( id.isInvalid() )
	removeWithKey( keyw );
    else
	set( keyw, id.getI() );
}


template <class fT>
bool IOPar::get( const char* keyw, Geom::Point2D<fT>& crd ) const
{
    return get( keyw, crd.x_, crd.y_ );
}


template <class fT>
bool IOPar::get( const char* keyw, Geom::Point3D<fT>& crd ) const
{
    return get( keyw, crd.x_, crd.y_, crd.z_ );
}


template<class fT>
void IOPar::set( const char* keyw, const Geom::Point2D<fT>& crd )
{
    set( keyw, crd.x_, crd.y_ );
}


template<class fT>
void IOPar::set( const char* keyw, const Geom::Point3D<fT>& crd )
{
    set( keyw, crd.x_, crd.y_, crd.z_ );
}


template <class T1,class T2>
bool IOPar::get( const char* keyw, std::pair<T1,T2>& p ) const
{
    return get( keyw, p.first, p.second );
}


template <class T>
bool IOPar::get( const char* keyw, Twins<T>& t ) const
{
    return get( keyw, t.first(), t.second() );
}


template <class T>
bool IOPar::get( const char* keyw, Triplets<T>& t ) const
{
    return get( keyw, t.first(), t.second(), t.third() );
}


template<class T1,class T2>
void IOPar::set( const char* keyw, const std::pair<T1,T2>& p )
{
    set( keyw, p.first, p.second );
}


template<class T>
void IOPar::set( const char* keyw, const Twins<T>& t )
{
    set( keyw, t.first(), t.second() );
}


template<class T>
void IOPar::set( const char* keyw, const Triplets<T>& t )
{
    set( keyw, t.first(), t.second(), t.third() );
}
