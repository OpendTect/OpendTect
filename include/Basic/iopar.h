#ifndef iopar_H
#define iopar_H

/*+
________________________________________________________________________

 CopyRight:	(C) de Groot-Bril Earth Sciences B.V.
 Author:	A.H. Bril
 Date:		21-12-1995
 RCS:		$Id: iopar.h,v 1.15 2002-03-18 14:17:21 kristofer Exp $
________________________________________________________________________

-*/
 
#include <uidobj.h>
#include <sets.h>
class ascistream;
class ascostream;
class Coord;
class BinID;
class MultiID;
class AliasObjectSet;
class BufferString;

/*\brief generalised set of parameters of the keyword-value type.

Part of the function of this class is as in an STL map<string,string> .
Passing a keyword will return the appropriate value.

Tools around this basic idea are paring into other types, key composition,
reading/writing to/from file, merging, and more.

*/


class IOPar : public UserIDObject
{

   friend class		EdIOPar;

public:
			IOPar(const char* nm=0);
			IOPar(UserIDObject*);
			IOPar(ascistream&,bool withname=true);
    void		putTo(ascostream&,bool withname=true) const;
			~IOPar();
			IOPar(const IOPar&);
    IOPar&		operator=(const IOPar&);

			// serialisation
    void		getFrom(const char*);
    void		putTo(BufferString&) const;

    int			size() const;
    const char*		getKey(int) const;
    const char*		getValue(int) const;
    bool		setKey(int,const char*);
			//!< Will fail if key is empty or already present
    void		setValue(int,const char*);
    void		remove(int);

    void		clear();
			//!< remove all entries
    void		merge(const IOPar&);
			//!< merge entries using the set() command
    static const char*	compKey(const char*,const char*);
			//!< The composite key
    static const char*	compKey(const char*,int);
			//!< The composite key where int will be --> string
    IOPar*		subselect(const char*) const;
			//!< returns iopar with key that start with <str>.
    IOPar*		subselect(int) const;
			//!< returns iopar with key that start with number.
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
    bool		get(const char*,int&,int&,int&) const;
    bool		getYN(const char*,bool&,char c=0) const;

    void		set(const char*,const char*);
    void		set(const char*,int);
    void		set(const char*,float);
    void		set(const char*,double);
    void		set(const char*,int,int);
    void		set(const char*,float,float);
    void		set(const char*,float,float,float);
    void		set(const char*,float,float,float,float);
    void		set(const char*,double,double);
    void		set(const char*,double,double,double);
    void		set(const char*,double,double,double,double);
    void		set(const char*,int,int,int);
    void		setYN(const char*,bool);

    void		set(const char*,const BinID&);
    void		set(const char*,const Coord&);
    void		set(const char*,const MultiID&);
    void		set(const char*,const BufferString&);
    bool		get(const char*,BinID&) const;
    bool		get(const char*,Coord&) const;
    bool		get(const char*,MultiID&) const;
    bool		get(const char*,BufferString&) const;

    void		add(const char*,const char*);
			/*!< Only to save performance: responsibility for
			     caller to avoid duplicate keys! */

    const char*		mkKey(int) const;

protected:

    AliasObjectSet&	pars_;

};


template <class T>
inline void use( const TypeSet<T>& ts, IOPar& iopar, const char* basekey=0 )
{
    const int sz = ts.size();
    for ( int idx=0; idx<sz; idx++ )
	iopar.set( compKey(basekey,idx+1), ts[idx] );
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
