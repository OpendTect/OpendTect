#ifndef iopar_h
#define iopar_h

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	A.H. Bril
 Date:		21-12-1995
 RCS:		$Id: iopar.h,v 1.53 2008-12-04 13:24:01 cvsbert Exp $
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
class SeparString;
class ascistream;
class ascostream;
template <class T> class SamplingData;

/*\brief generalised set of parameters of the keyword-value type.

Part of the function of this class is as in an STL map<string,string> .
Passing a keyword will return the appropriate value.

Tools around this basic idea are paring into other types, key composition,
reading/writing to/from file, merging, and more.

dumpPretty() is used for reports.  The title of the report is the name of the
IOPar. If sKeyHdr and sKeySubHdr are the key, there will be a (sub)header
with the value. Use add() rather than set(). Values may contain newlines.

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

// GET functions

    const char*		find(const char*) const;
			//!< returns null if not found
    const char*		operator[](const char*) const;
			//!< returns empty string if not found

    			// Functions for getting 1,2,3 and 4 of the same type
#define mIOParDeclFns(type) \
    bool		get(const char*,type&) const; \
    bool		get(const char*,type&,type&) const; \
    bool		get(const char*,type&,type&,type&) const; \
    bool		get(const char*,type&,type&,type&,type&) const

    			mIOParDeclFns(int);
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
			{ bool b = false; return getYN(key,b) && b; }

    bool		get(const char*,int&,int&,float&) const;

    bool		get(const char*,TypeSet<int>&) const;
    bool		get(const char*,TypeSet<od_uint32>&) const;
    bool		get(const char*,TypeSet<od_int64>&) const;
    bool		get(const char*,TypeSet<od_uint64>&) const;
    bool		get(const char*,TypeSet<double>&) const;
    bool		get(const char*,TypeSet<float>&) const;

    bool		get(const char*,Interval<int>&) const;
    bool		get(const char*,Interval<float>&) const;
    bool		get(const char*,Interval<double>&) const;
    bool		get(const char*,SamplingData<int>&) const;
    bool		get(const char*,SamplingData<float>&) const;
    bool		get(const char*,SamplingData<double>&) const;
    bool		get(const char*,BinID&) const;
    bool		get(const char*,Coord&) const;
    bool		get(const char*,Coord3&) const;
    bool		get(const char*,MultiID&) const;
    bool		get(const char*,Color&) const;
    bool		get(const char*,SeparString&) const;
    bool		get(const char*,BufferString&) const;
    bool		get(const char*,BufferString&,BufferString&) const;
    bool		get(const char*,BufferStringSet&) const;

    bool		getPtr(const char*,void*&) const;

#define mIOParDeclFns(type) \
    bool		getSc(const char*,type&,type applied_scale, \
	    		      bool set_to_undef_if_not_found) const; \
    bool		getSc(const char*,type&,type&,type,bool) const; \
    bool		getSc(const char*,type&,type&,type&,type,bool) const;\
    bool		getSc(const char*,type&,type&,type&,type&,type, \
	    		      bool) const
    			mIOParDeclFns(float);
    			mIOParDeclFns(double);
#undef mIOParDeclFns


// SET functions

    void		set(const char*,const char*);
			/*!< Set replaces when key already exists */
    void		add(const char*,const char*);
			/*!< Add does not check for duplicate keys */

    			// Functions for 1,2,3 and 4 of the same type
#define mIOParDeclFns(fnnm,type) \
    void		fnnm(const char*,type); \
    void		fnnm(const char*,type,type); \
    void		fnnm(const char*,type,type,type); \
    void		fnnm(const char*,type,type,type,type)

			mIOParDeclFns(set,int);
			mIOParDeclFns(set,od_uint32);
			mIOParDeclFns(set,od_int64);
			mIOParDeclFns(set,od_uint64);
			mIOParDeclFns(set,float);
			mIOParDeclFns(set,double);
			mIOParDeclFns(add,int);
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

    void		set(const char*,int,int,float);
    void		setPtr(const char*,void*);

    void		set(const char*,const char*,const char*);
    void		set(const char*,const Interval<int>&);
    void		set(const char*,const Interval<float>&);
    void		set(const char*,const Interval<double>&);
    void		set(const char*,const SamplingData<int>&);
    void		set(const char*,const SamplingData<float>&);
    void		set(const char*,const SamplingData<double>&);
    void		set(const char*,const BinID&);
    void		set(const char*,const Coord&);
    void		set(const char*,const Coord3&);
    void		set(const char*,const MultiID&);
    void		set(const char*,const Color&);
    void		set(const char*,const SeparString&);
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

// I/O  functions

    bool		read(const char* filename,const char* filetype,
	    			bool chktype=false);
    			//!< filetype null will assume no file header
    			//!< uses set(). no clear() done
    void		read(std::istream&,const char* filetype,
	    			bool chktype=false);
    bool		write(const char* filename,const char* filetype) const;
    			//!< If filetype is set to null no ascstream header
    			//!< sKeyDumpPretty calls dumpPretty.
    bool		write(std::ostream&,const char* filetyp) const;

    const char*		mkKey(int) const;

    void		dumpPretty(std::ostream&) const;
    static const char*	sKeyDumpPretty;
    static const char*	sKeyHdr;
    static const char*	sKeySubHdr;

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
