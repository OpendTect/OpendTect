#ifndef iopar_h
#define iopar_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	A.H. Bril
 Date:		21-12-1995
 RCS:		$Id$
________________________________________________________________________

-*/
 
#include "basicmod.h"
#include "namedobj.h"
#include "sets.h"
#include "fixedstring.h"
#include "samplingdata.h"

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

/*!
\ingroup Basic
\brief Generalized set of parameters of the keyword-value type.

  Part of the function of this class is as in an STL map<string,string>.
  Passing a keyword will return the appropriate value.
  
  Tools around this basic idea are paring into other types, key composition,
  reading/writing to/from file, merging, and more.
  
  dumpPretty() is used for reports.  The title of the report is the name of the
  IOPar. If sKeyHdr and sKeySubHdr are the key, there will be a (sub)header
  with the value. Use add() rather than set(). Values may contain newlines.
*/

mExpClass(Basic) IOPar : public NamedObject
{
public:
			IOPar(const char* nm=0); //!< empty
			IOPar(ascistream&);
			IOPar(const IOPar&);
			~IOPar();
    IOPar&		operator =(const IOPar&);
    inline bool		operator ==( const IOPar& iop ) const
			{ return isEqual(iop); }
    inline bool		operator !=( const IOPar& iop ) const
			{ return !isEqual(iop); }

    int			size() const;
    inline bool		isEmpty() const		{ return size() == 0; }
    bool		isEqual(const IOPar&,bool need_same_order=false) const;

    int			indexOf(const char* key) const;
    FixedString		getKey(int) const;
    FixedString		getValue(int) const;
    bool		setKey(int,const char*);
    void		setValue(int,const char*);
    void		remove(int);
    void		remove(const char* key);
    bool		hasKey( const char* s ) const
			{ return !find(s).isEmpty(); }
    const char*		findKeyFor(const char*,int nr=0) const;
				//!< returns null if value not found
    void		removeWithKey(const char* globexpression);
				//!< removes all entries with key matching
				//!< this glob expression

    void		setEmpty();
			//!< remove all entries (doesn't clear name)
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

// GET functions

    FixedString		find(const char*) const;
			//!< returns null if not found
    FixedString		operator[](const char*) const;
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
			{ bool is = false; return getYN(key,is) && is; }
    inline bool		isFalse( const char* key ) const
			{ bool is = true; return getYN(key,is) && !is; }

    bool		get(const char*,int&,int&,float&) const;

    bool		get(const char*,TypeSet<int>&) const;
    bool		get(const char*,TypeSet<od_uint32>&) const;
    bool		get(const char*,TypeSet<od_int64>&) const;
    bool		get(const char*,TypeSet<od_uint64>&) const;
    bool		get(const char*,TypeSet<double>&) const;
    bool		get(const char*,TypeSet<float>&) const;

    bool		get(const char*,BinID&) const;
    bool		get(const char*,Coord&) const;
    bool		get(const char*,Coord3&) const;
    bool		get(const char*,MultiID&) const;
    bool		get(const char*,Color&) const;
    bool		get(const char*,SeparString&) const;
    bool		get(const char*,BufferString&) const;
    bool		get(const char*,BufferString&,BufferString&) const;
    bool		get(const char*,BufferStringSet&) const;
    template <class T>
    bool		get(const char*,Interval<T>&) const;
    template <class T>
    bool		get(const char*,SamplingData<T>&) const;

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
    void		set(const char*,const BinID&);
    void		set(const char*,const Coord&);
    void		set(const char*,const Coord3&);
    void		set(const char*,const MultiID&);
    void		set(const char*,const Color&);
    void		set(const char*,const SeparString&);
    void		set(const char*,const FixedString&);
    void		set(const char*,const BufferString&);
    void		set(const char*,const BufferString&,
	    				const BufferString&);
    void		set(const char*,const BufferStringSet&);
    template <class T>
    void		set(const char*,const Interval<T>&);
    template <class T>
    void		set(const char*,const SamplingData<T>&);

    void		set(const char*,const TypeSet<int>&);
    void		set(const char*,const TypeSet<od_uint32>&);
    void		set(const char*,const TypeSet<od_int64>&);
    void		set(const char*,const TypeSet<od_uint64>&);
    void		set(const char*,const TypeSet<double>&);
    void		set(const char*,const TypeSet<float>&);


// I/O  functions

    // to/from string: 'serialisation'
    void		getFrom(const char*);
    void		getParsFrom(const char*);
    void		putTo(BufferString&) const;
    void		putParsTo(BufferString&) const;

    // to/from file
    void		getFrom(ascistream&);
    void		putTo(ascostream&) const;
    bool		read(const char* filename,const char* filetype,
	    			bool chktype=false);
    			//!< filetype null will assume no file header
    			//!< uses set(). no clear() done
    bool		read(std::istream&,const char* filetype,
	    			bool chktype=false);
    bool		write(const char* filename,const char* filetype) const;
    			//!< If filetype is set to null no ascstream header
    			//!< sKeyDumpPretty calls dumpPretty.
    bool		write(std::ostream&,const char* filetyp) const;
    void		dumpPretty(BufferString&) const;
    void		dumpPretty(std::ostream&) const;

    static const char*	sKeyDumpPretty()         { return "_pretty"; }
    static const char*	sKeyHdr()		 { return "->";	     }	
    static const char*	sKeySubHdr()		 { return "-->";     }   	

protected:

    BufferStringSet&	keys_;
    BufferStringSet&	vals_;

};


template <class T>
inline bool IOPar::get( const char* k, Interval<T>& i ) const
{
    mDynamicCastGet(StepInterval<T>*,si,&i)
    return si ? get( k, i.start, i.stop, si->step )
	      : get( k, i.start, i.stop );
}


template <class T>
inline void IOPar::set( const char* k, const Interval<T>& i )
{
    mDynamicCastGet(const StepInterval<T>*,si,&i)
    if ( si )	set( k, i.start, i.stop, si->step );
    else	set( k, i.start, i.stop );
}


template <class T>
inline bool IOPar::get( const char* k, SamplingData<T>& sd ) const
{
    return get( k, sd.start, sd.step );
}


template <class T>
inline void IOPar::set( const char* k, const SamplingData<T>& sd )
{
    set( k, sd.start, sd.step );
}


#endif

