#ifndef odinstrel_h
#define odinstrel_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          Apr 2010
 RCS:           $Id: odinstrel.h 8009 2013-06-20 06:11:26Z kristofer.tingdahl@dgbes.com $
________________________________________________________________________

-*/

#include "uiodinstmgrmod.h"
#include "odinst.h"
#include "odinstver.h"
#include "enums.h"
class IOPar;
class BufferStringSet;


namespace ODInst
{

const char* 		sBasePkgName();		//!< "base"
const char*		sTempDataDir();


enum LicType		{ Commercial, Academic, GPL };
			DeclareNameSpaceEnumUtils(uiODInstMgr,LicType)

void			getBSSFromIOP(const char* inpky,const IOPar&,
				      BufferStringSet&);
BufferString		fullURL(const char*);



mDefClass(uiODInstMgr) RelData
{
public:
			RelData()		{}

    bool		isAvailable() const	{ return !version_.isEmpty(); }
    BufferString	prettyName() const;

    RelType		reltype_;
    Version		version_;
    BufferString	name_;
};


mDefClass(uiODInstMgr) RelDataSet : public ObjectSet<RelData>
{
public:

    			RelDataSet()		{}
    void		set(const BufferStringSet&);

    RelData*		get(RelType);
    const RelData*	get( RelType rt ) const
			{ return const_cast<RelDataSet*>(this)->get(rt); }
    RelData*		get(const Version& version);
    RelData*		get(const char* name);
    const RelData*	get(const Version& version) const;
    const RelData*	get(const char* name) const;
};

} // namespace

#endif

