#ifndef iopar_H
#define iopar_H

/*@+
________________________________________________________________________

 CopyRight:	(C) de Groot-Bril Earth Sciences B.V.
 Author:	A.H. Bril
 Date:		21-12-1995
 RCS:		$Id: iopar.h,v 1.4 2000-08-08 14:16:18 bert Exp $
________________________________________________________________________

@$*/
 
#include <uidobj.h>
class ascistream;
class ascostream;
class Coord;
class BinID;
class AliasObjectSet;


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

    AliasObjectSet&	getPars() const	      { return (AliasObjectSet&)pars_; }

    int			size() const;
    void		clear();
    void		merge(const IOPar&);
    static const char*	compKey(const char*,const char*);
    const char*		compFind(const char*,const char*) const;
    IOPar*		subselect(const char*) const;
    void		mergeComp(const IOPar&,const char*);
    const char*		findKey(const char*) const;
			// First matching, null if none

    const char*		operator[](const char*) const;
    bool		get(const char*,int&) const;
    bool		get(const char*,int&,int&) const;
    inline bool		get( const char* k, float& v ) const
			{ return getSc(k,v,1,NO); }
    inline bool		get( const char* k, double& v ) const
			{ return getSc(k,v,1,NO); }
    inline bool		get( const char* k, float& v1, float& v2 ) const
			{ return getSc(k,v1,v2,1,NO); }
    inline bool		get( const char* k, double& v1, double& v2 ) const
			{ return getSc(k,v1,v2,1,NO); }
    bool		getSc(const char*,float&,float sc,
			      bool set_undef_if_not_found) const;
    bool		getSc(const char*,double&,double sc,
			      bool set_undef_if_not_found) const;
    bool		getSc(const char*,float&,float&,float sc,
			      bool set_undef_if_not_found) const;
    bool		getSc(const char*,double&,double&,double sc,
			      bool set_undef_if_not_found) const;
    bool		get(const char*,int&,int&,int&) const;
    bool		getYN(const char*,bool&,char c=0) const;
    void		set(const char*,const char*);
    void		set(const char*,int);
    void		set(const char*,double);
    void		set(const char*,int,int);
    void		set(const char*,double,double);
    void		set(const char*,int,int,int);
    void		setYN(const char*,bool);

    void		set(const char*,const BinID&);
    void		set(const char*,const Coord&);
    bool		get(const char*,BinID&) const;
    bool		get(const char*,Coord&) const;

private:
    UserIDString	type_;
    AliasObjectSet&	pars_;

    void		newPar(const char*,const char*);

};


#endif
