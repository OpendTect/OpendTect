#ifndef ioman_H
#define ioman_H

/*@+
________________________________________________________________________

 CopyRight:	(C) de Groot-Bril Earth Sciences B.V.
 Author:	A.H. Bril
 Date:		3-8-1995
 RCS:		$Id: ioman.h,v 1.2 2000-05-29 10:33:55 bert Exp $
________________________________________________________________________

@$*/
 
/*@+
@$*/

#include <uidobj.h>
#include <unitid.h>
#include <sets.h>
class IOLink;
class IOParList;
class IOPar;
class CtxtIOObj;


class IOMan : public UserIDObject
{
    friend class	dMain;
    friend class	IOObj;
    friend class	IODir;
    friend IOMan&	IOM();

public:
    enum State		{ Bad, NeedInit, Good };
    int			bad() const		{ return state_ != Good; }
    State		state() const		{ return state_; }

    int			to(const IOLink*);	// NULL -> ".."
    int			to(const UnitID&);
    void		back();

    // The following functions return a cloned IOObj (=mem man by caller)
    IOObj*		get(const UnitID&) const;
    IOObj*		getIfOnlyOne(const char* trgroupname) const;
    IOObj*		getByName(const char* objname,
			      const char* partrgname=0,const char* parname=0);

    IODir*		dirPtr() const		{ return (IODir*)dirptr; }
    UnitID		unitID() const;
    const char*		curDir() const;
    int			curLevel() const	{ return curlvl; }
    const char*		rootDir() const		{ return rootdir; }
    int			levelOf(const char* dirnm) const;
    const char*		nameOf(const char* uid) const;

    void		getEntry(CtxtIOObj&,UnitID parentid="");
    IOParList*		getParList(const char* typ=0) const;

    IOPar*		getAux(const UnitID&) const;
    int			putAux(const UnitID&,const IOPar*) const;
    IOParList*		getAuxList(const UnitID&) const;
    int			putAuxList(const UnitID&,const IOParList*) const;
    int			hasAux(const UnitID&) const;
    int			removeAux(const UnitID&) const;

    			~IOMan();
    static int		newSurvey();
    static void		setSurvey(const char*);

private:
    State		state_;
    IODir*		dirptr;
    int			curlvl;
    UnitID		prevunitid;
    FileNameString	rootdir;

    static IOMan*	theinst_;
			IOMan();
    void		init();
    static void		stop();
    int			setRootDir(const char*);

    int			setDir(const char*);
    UnitID		newId() const;
    int			getAuxfname(const UnitID&,FileNameString&) const;

};


inline IOMan& IOM()
{
    if ( !IOMan::theinst_ )
	{ IOMan::theinst_ = new IOMan; IOMan::theinst_->init(); }
    return *IOMan::theinst_;
}


#endif
