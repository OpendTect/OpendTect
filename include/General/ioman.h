#ifndef ioman_H
#define ioman_H

/*+
________________________________________________________________________

 CopyRight:	(C) de Groot-Bril Earth Sciences B.V.
 Author:	A.H. Bril
 Date:		3-8-1995
 RCS:		$Id: ioman.h,v 1.14 2003-03-27 12:49:58 bert Exp $
________________________________________________________________________

-*/
 

#include <uidobj.h>
#include <multiid.h>
#include <sets.h>
class IOLink;
class IOParList;
class IOPar;
class CtxtIOObj;
class Translator;


/*!> Class IOMan manages the Meta-data store for the IOObj's. This info
is read from the .omf files.

There will be one IOMan available through the gloabal function IOM(). Creating
more instances is probably not be a good idea, but may work.

A current IODir is maintained. Auxiliary info, not needed for read/write the
object, but useful info can be stored in .aux files.

Access to the parameter save files (e.g. '.Process_Seismic') is also provided
through getParList().

*/

class IOMan : public UserIDObject
{
    friend class	IOObj;
    friend class	IODir;
    friend IOMan&	IOM();

public:

    enum State		{ Bad, NeedInit, Good };
    bool		bad() const		{ return state_ != Good; }
    State		state() const		{ return state_; }

    bool		to(const IOLink*);	//!< NULL -> ".."
    bool		to(const MultiID&);
    void		back();

    //! The following functions return a cloned IOObj (=mem man by caller)
    IOObj*		get(const MultiID&) const;
    IOObj*		getOfGroup(const char* tgname,bool first=true,
	    			   bool onlyifsingle=false) const;
    IOObj*		getIfOnlyOne( const char* trgroupname )
			{ return getOfGroup(trgroupname,true,true); }
    IOObj*		getByName(const char* objname,
			      const char* partrgname=0,const char* parname=0);

    IODir*		dirPtr() const		{ return (IODir*)dirptr; }
    MultiID		key() const;		//!< of current IODir
    const char*		curDir() const;		//!< OS dir name
    int			curLevel() const	{ return curlvl; }
    const char*		rootDir() const		{ return rootdir; }
    int			levelOf(const char* dirnm) const;
    const char*		nameOf(const char* keystr,bool inc_parents=false) const;

    void		getEntry(CtxtIOObj&,MultiID parentid="");
			//!< will create a new entry if necessary
    bool		haveEntries(const MultiID& dirid,const char* trgrpnm=0,
				     const char* trnm=0) const;
    IOParList*		getParList(const char* typ=0) const;
			//!< Reads the file on the root of the survey
    bool		commitChanges(const IOObj&);
    bool		permRemove(const MultiID&);

    IOPar*		getAux(const MultiID&) const;
    bool		putAux(const MultiID&,const IOPar*) const;
    IOParList*		getAuxList(const MultiID&) const;
    bool		putAuxList(const MultiID&,const IOParList*) const;
    bool		hasAux(const MultiID&) const;
    bool		removeAux(const MultiID&) const;

    const char*		surveyName() const;
    static bool		newSurvey();
			/*!< if an external source has changed
				the $HOME/.dgbSurvey, force re-read it. */
    static void		setSurvey(const char*);
			/*!< will remove a possible existing IO manager and
			     set the survey to 'name', thus bypassing the
			     $HOME/.dgbSurvey file */

    bool		setRootDir(const char*);
    bool		setFileName(MultiID,const char*);
    const char*		generateFileName(Translator*,const char*);
    static bool		validSurveySetup(BufferString& errmsg);
    MultiID		newKey() const;

private:

    State		state_;
    IODir*		dirptr;
    int			curlvl;
    MultiID		prevkey;
    FileNameString	rootdir;

    static IOMan*	theinst_;
			IOMan(const char* rd=0);
    			~IOMan();
    void		init();
    static void		stop();

    bool		setDir(const char*);
    bool		getAuxfname(const MultiID&,FileNameString&) const;

};


inline IOMan& IOM()
{
    if ( !IOMan::theinst_ )
	{ IOMan::theinst_ = new IOMan; IOMan::theinst_->init(); }
    return *IOMan::theinst_;
}


#endif
