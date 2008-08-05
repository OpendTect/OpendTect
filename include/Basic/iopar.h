#ifndef iopar_h
#define iopar_h

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	A.H. Bril
 Date:		21-12-1995
 RCS:		$Id: iopar.h,v 1.48 2008-08-05 10:04:48 cvsbert Exp $
________________________________________________________________________

-*/
 
#include "namedobj.h"
#include "sets.h"
#include "ranges.h"

class BinID;
class BufferString;
class BufferStringSet;
class Coord;
class Color;
class Coord3;
class MultiID;
class ascistream;
class ascostream;

/*\brief generalised set of parameters of the keyword-value type.

Part of the function of this class is as in an STL map<string,string> .
Passing a keyword will return the appropriate value.

Tools around this basic idea are paring into other types, key composition,
reading/writing to/from file, merging, and more.

*/


class IOPar : public NamedObject
{
public:
			IOPar(const char* nm=0); //!< empty
			IOPar(ascistream&);
			~IOPar();
			IOPar(const IOPar&);
    IOPar&		operator =(const IOPar&);
    bool		operator ==( const IOPar& iop ) const
			{ return isEqual(iop); }
    void		getFrom(ascistream&);
    void		putTo(ascostream&) const;
    inline bool		isEmpty() const		{ return size() == 0; }

			// serialisation
    void		getFrom(const char*);
    void		getParsFrom(const char*);
    void		putTo(BufferString&) const;
    void		putParsTo(BufferString&) const;

    int			size() const;
    int			indexOf(const char* key) const;
    const char*		getKey(int) const;
    const char*		getValue(int) const;
    bool		setKey(int,const char*);
			//!< Will fail if key is empty or already present
    void		setValue(int,const char*);
    void		remove(int);
    void		remove(const char* key);

    bool		isEqual(const IOPar&,bool include_order=false) const;
    void		clear();
			//!< remove all entries
    void		merge(const IOPar&);
			//!< merge entries using the set() command
    static const char*	compKey(const char*,const char*);
			//!< The composite key: (a,b) -> a.b
    static const char*	compKey(const char*,int);
			//!< The composite key where int will be --> string
    IOPar*		subselect(const char*) const;
			//!< returns iopar with key that start with <str>.
    IOPar*		subselect(int) const;
			//!< returns iopar with key that start with number.
    void		removeSubSelection(const char*);
			//!< removes with key that start with <str>.
    void		removeSubSelection(int);
			//!< removes with key that start with number.
    void		mergeComp(const IOPar&,const char*);
			//!< merge entries, where IOPar's entries get a prefix

    bool		hasKey( const char* s ) const
			{ return find(s) ? true : false; }
    const char*		findKeyFor(const char*,int nr=0) const;
				//!< returns null if value not found
    void		removeWithKey(const char* globexpression);
				//!< removes all entries with key matching
				//!< this glob expression

    const char*		operator[](const char*) const;
			//!< returns empty string if not found
    const char*		find(const char*) const;
			//!< returns null if not found

    bool		get(const char*,int&) const;
    bool		get(const char*,od_uint32&) const;
    bool		get(const char*,od_int64&) const;
    bool		get(const char*,od_uint64&) const;
    bool		get(const char*,int&,int&) const;
    inline bool		get( const char* k, float& v ) const
			{ return getSc(k,v,1,false); }
    inline bool		get( const char* k, double& v ) const
			{ return getSc(k,v,1,false); }
    inline bool		get( const char* k, float& v1, float& v2 ) const
			{ return getSc(k,v1,v2,1,false); }
    inline bool		get( const char* k, float& v1,
	    		     float& v2, float& v3 ) const
			{ return getSc(k,v1,v2,v3,1,false); }
    inline bool		get( const char* k, float& v1,
	    		     float& v2, float& v3, float& v4 ) const
			{ return getSc(k,v1,v2,v3,v4,1,false); }
    inline bool		get( const char* k, double& v1, double& v2 ) const
			{ return getSc(k,v1,v2,1,false); }
    inline bool		get( const char* k, double& v1,
	    		     double& v2, double& v3 ) const
			{ return getSc(k,v1,v2,v3,1,false); }
    inline bool		get( const char* k, double& v1,
	    		     double& v2, double& v3, double& v4 ) const
			{ return getSc(k,v1,v2,v3,v4,1,false); }
    bool		get(const char*,int&,int&,int&) const;
    bool		get(const char*,int&,int&,float&) const;

    bool		get(const char*,TypeSet<int>&) const;
    bool		get(const char*,TypeSet<od_uint32>&) const;
    bool		get(const char*,TypeSet<od_int64>&) const;
    bool		get(const char*,TypeSet<od_uint64>&) const;
    bool		get(const char*,TypeSet<double>&) const;
    bool		get(const char*,TypeSet<float>&) const;

    bool		getSc(const char*,float&,float sc,
			      bool set_undef_if_not_found) const;
			//!< get with a scale applied
    bool		getSc(const char*,double&,double sc,
			      bool set_undef_if_not_found) const;
    bool		getSc(const char*,float&,float&,float sc,
			      bool set_undef_if_not_found) const;
    bool		getSc(const char*,float&,float&,float&,float sc,
			      bool set_undef_if_not_found) const;
    bool		getSc(const char*,float&,float&,float&,float&,float sc,
			      bool set_undef_if_not_found) const;
    bool		getSc(const char*,double&,double&,double sc,
			      bool set_undef_if_not_found) const;
    bool		getSc(const char*,double&,double&,double&,double sc,
			      bool set_undef_if_not_found) const;
    bool		getSc(const char*,double&,double&,double&,double&,
	    			double sc, bool set_undef_if_not_found) const;
    bool		getYN(const char*,bool&) const;
    bool		getYN(const char*,bool&,bool&) const;
    bool		getPtr(const char*,void*&) const;
    inline bool		isTrue( const char* key ) const
			{ bool b = false; return getYN(key,b) && b; }
    bool		get(const char*,Interval<int>&) const;
    bool		get(const char*,Interval<float>&) const;
    bool		get(const char*,Interval<double>&) const;
    bool		get(const char*,BinID&) const;
    bool		get(const char*,Coord&) const;
    bool		get(const char*,Coord3&) const;
    bool		get(const char*,MultiID&) const;
    bool		get(const char*,Color&) const;
    bool		get(const char*,BufferString&) const;
    bool		get(const char*,BufferString&,BufferString&) const;
    bool		get(const char*,BufferStringSet&) const;


#define mSet(type) \
    void		set(const char*,type); \
    void		set(const char*,type,type); \
    void		set(const char*,type,type,type); \
    void		set(const char*,type,type,type,type);

			mSet(int);
			mSet(od_uint32);
			mSet(od_int64);
			mSet(od_uint64);
			mSet(float);
			mSet(double);
#undef mSet
    
    void		set(const char*,const char*);
    void		set(const char*,const char*,const char*);
    void		set(const char*,int,int,float);

    void		setYN(const char*,bool);
    void		setYN(const char*,bool,bool);
    void		setPtr(const char*,void*);

    void		set(const char*,const Interval<int>&);
    void		set(const char*,const Interval<float>&);
    void		set(const char*,const Interval<double>&);
    void		set(const char*,const BinID&);
    void		set(const char*,const Coord&);
    void		set(const char*,const Coord3&);
    void		set(const char*,const MultiID&);
    void		set(const char*,const Color&);
    void		set(const char*,const BufferString&);
    void		set(const char*,const BufferString&,
	    				const BufferString&);
    void		set(const char*,const BufferStringSet&);

    void		set(const char*,const TypeSet<int>&);
    void		set(const char*,const TypeSet<od_uint32>&);
    void		set(const char*,const TypeSet<od_int64>&);
    void		set(const char*,const TypeSet<od_uint64>&);
    void		set(const char*,const TypeSet<double>&);
    void		set(const char*,const TypeSet<float>&);

    void		add(const char*,const char*);
			/*!< Only to save performance: responsibility for
			     caller to avoid duplicate keys! */

    const char*		mkKey(int) const;

    bool		read(const char* filename,const char* filetype,
	    			bool chktype=false);
    			//!< filetype null will assume no file header
    			//!< uses set(). no clear() done
    void		read(std::istream&,const char* filetype,
	    			bool chktype=false);
    bool		write(const char* filename,const char* filetype) const;
    			//!< If filetype is set to null no ascstream header
    			//!< If filetype is set to "_pretty", calls dumpPretty.
    bool		write(std::ostream&,const char* filetyp) const;
    void		dumpPretty(std::ostream&) const;

protected:

    BufferStringSet&	keys_;
    BufferStringSet&	vals_;

};


template <class T>
inline void use( const TypeSet<T>& ts, IOPar& iopar, const char* basekey=0 )
{
    const int sz = ts.size();
    for ( int idx=0; idx<sz; idx++ )
	iopar.set( IOPar::compKey(basekey,idx+1), ts[idx] );
}


template <class T>
inline void use( const IOPar& iopar, TypeSet<T>& ts, const char* basekey=0 )
{
    T t;
    for ( int idx=0; ; idx++ )
    {
	if ( !iopar.get( IOPar::compKey(basekey,idx), t ) )
	    { if ( idx ) return; else continue; }
	ts += t;
    }
}


#endif
