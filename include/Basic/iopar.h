#ifndef iopar_H
#define iopar_H

/*@+
________________________________________________________________________

 CopyRight:	(C) de Groot-Bril Earth Sciences B.V.
 Author:	A.H. Bril
 Date:		21-12-1995
 RCS:		$Id: iopar.h,v 1.1.1.1 1999-09-03 10:11:41 dgb Exp $
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
			IOPar(ascistream&);
    void		putTo(ascostream&) const;
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
    bool		get(const char*,float&) const;
    bool		get(const char*,double&) const;
    bool		get(const char*,int&,int&) const;
    bool		get(const char*,float&,float&,float sc=1,
			    bool set_undef=NO) const;
    bool		get(const char*,double&,double&,double sc=1,
			    bool set_undef=NO) const;
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
    void		get(const char*,BinID&) const;
    void		get(const char*,Coord&) const;

private:
    UserIDString	type_;
    AliasObjectSet&	pars_;

    void		newPar(const char*,const char*);

};


#endif
