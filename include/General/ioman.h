#ifndef ioman_H
#define ioman_H

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	A.H. Bril
 Date:		3-8-1995
 RCS:		$Id: ioman.h,v 1.29 2006-09-14 08:01:44 cvsbert Exp $
________________________________________________________________________

-*/
 

#include "namedobj.h"
#include "multiid.h"
#include "sets.h"
class IOLink;
class IOParList;
class IOPar;
class CtxtIOObj;
class IOObjContext;
class Translator;
class IODir;
class IOObj;
class IOMan;


/*!\brief manages the Meta-data store for the IOObj's. This info
is read from the .omf files.

There will be one IOMan available through the gloabal function IOM(). Creating
more instances is probably not be a good idea, but may work.

A current IODir is maintained. Auxiliary info, not needed for read/write the
object, but useful info can be stored in .aux files.

Access to the parameter save files (e.g. '.Process_Seismic') is also provided
through getParList().

*/

inline IOMan& IOM();


class IOMan : public NamedObject
{
public:

    bool		bad() const		{ return state_ != Good; }
    bool		isReady() const;

    bool		to(const IOLink*);	//!< NULL -> ".."
    bool		to(const MultiID&);
    void		back();

    //! The following functions return a cloned IOObj (=mem man by caller)
    IOObj*		get(const MultiID&) const;
    IOObj*		getLocal(const char* objname) const;
    IOObj*		getOfGroup(const char* tgname,bool first=true,
	    			   bool onlyifsingle=false) const;
    IOObj*		getIfOnlyOne( const char* trgroupname )
			{ return getOfGroup(trgroupname,true,true); }
    IOObj*		getByName(const char* objname,
			      const char* partrgname=0,const char* parname=0);
    IOObj*		getFirst(const IOObjContext&,int* nrpresent=0) const;
    			//!< if interested in nrpresent pass valid address

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
    bool		commitChanges(const IOObj&);
    bool		permRemove(const MultiID&);

    const char*		surveyName() const;
    static bool		newSurvey();
			/*!< if an external source has changed
				the .od/survey, force re-read it. */
    static void		setSurvey(const char*);
			/*!< will remove a possible existing IO manager and
			     set the survey to 'name', thus bypassing the
			     .od/survey file */

    class CustomDirData
    {
    public:
			CustomDirData( int idnr, const char* dirnm,
					const char* desc="Custom data" )
			    : idnumber_(idnr)
			    , dirname_(dirnm)
			    , desc_(desc)		{}

	int		idnumber_; //!< Required: 10000 < idnumber_ < 100000
				   //!< Take a nice wild number
	BufferString	dirname_; //!< The subdirectory name in the tree
	BufferString	desc_; //!< Short description, mainly for error messages
			       //!< Example: "Geostatistical data"
    private:

	friend class	SurveyDataTreePreparer;
	friend class	IOMan;
	MultiID		dirid_;
	BufferString	dirnm_;

    };
    static const MultiID& addCustomDataDir(const CustomDirData&);
    			//!< Need to do this only once per OD run
    			//!< At survey change, dir will automatically be added

    bool		setRootDir(const char*);
    bool		setFileName(MultiID,const char*);
    const char*		generateFileName(Translator*,const char*);
    static bool		validSurveySetup(BufferString& errmsg);
    MultiID		newKey() const;

    Notifier<IOMan>	newIODir;
    Notifier<IOMan>	entryRemoved; // CallBacker will be CBCapsule<MultiID>
    Notifier<IOMan>	surveyChanged;

private:

    enum State		{ Bad, NeedInit, Good };
    State		state_;
    IODir*		dirptr;
    int			curlvl;
    MultiID		prevkey;
    FileNameString	rootdir;

    void		init();
			IOMan(const char* rd=0);
    			~IOMan();

    static IOMan*	theinst_;
    static void		setupCustomDataDirs();

    bool		setDir(const char*);

    friend class	IOObj;
    friend class	IODir;
    friend IOMan&	IOM();

};


inline IOMan& IOM()
{
    if ( !IOMan::theinst_ )
	{ IOMan::theinst_ = new IOMan; IOMan::theinst_->init(); }
    return *IOMan::theinst_;
}


/*!\mainpage
  \section Introduction Introduction

  This module uses the services from the Basic module and adds services that
  are (in general) more or less OpendTect specific. Just like the Basic module
  the services are used by all other modules.


  \section Content Content
  Some of the groups of services are:

<ul>
 <li>I/O management system
  <ul>
   <li>ioman.h : the IOM() object of the IOMan class provides a lookup of
       objects in the data store
   <li>ioobj.h : Subclasses of IOObj hold all data necessary to access a stored
       object.
   <li>iostrm.h : IOStream is the most common subclass of IOObj because
       OpendTect stores its data in files.
   <li>ctxtioobj.h : The context of an IOObj selection: what type of object,
       is it for read or write, should the user be able to create a new entry,
       etc.
  </ul>
 <li>Translators
  <ul>
   <li>transl.h : Translators are the objects that know file and database
       formats. All normal data will be put into and written from in-memory
       objects via subclasses of Translator.
  </ul>
 <li>ArrayND
  <ul>
   <li>arraynd.h and arrayndinfo.h : interface for multi-dimensional but
       flexible arrays of any (simple) type.
   <li>arrayndimpl.h : implementation with specific classed for 1, 2, and 3D.
   <li>arrayndslice.h and arrayndutils.h : Slices and other utlities.
  </ul>
 <li>CBVS
  <ul>
   <li>cbvsreadmgr.h : reads the 'Common Basic Volume Storage' format
   <li>cbvswritemgr.h : writes CBSV format.
  </ul>
 <li>Transformations
  <ul>
   <li>transform.h and transfact.h : Interface and factory for transforms
   <li>costrans.h, fft.h, wavelettrans.h : transforms
  </ul>
</ul>

*/


#endif
