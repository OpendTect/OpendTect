#ifndef twodseisdataconverterfromod4tood5format_h
#define twodseisdataconverterfromod4tood5format_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Salil Agarwal
 Date:		Dec 2013
 RCS:		$Id$
________________________________________________________________________

-*/


#include "seismod.h"
#include "bufstring.h"
#include "objectset.h"

class IOObj;
class IOPar;
class BufferStringSet;

/*!
/brief DO NOT USE THIS CLASS. 
       It is a highly specialized class for 2D Seismic data management 
       during the transition from OD4 to OD5.
*/

mExpClass(Seis) TwoDSeisDataConverterFromOD4ToOD5Format
{
public:

    bool		    convertSeisData(BufferString& errmsg);
protected:
			    TwoDSeisDataConverterFromOD4ToOD5Format()	    {}
			    ~TwoDSeisDataConverterFromOD4ToOD5Format();

    void		    makeListOfLineSets(ObjectSet<IOObj>&) const;
    void		    fillIOParsFrom2DSFile(const ObjectSet<IOObj>&);
    void		    getCBVSFilePaths(BufferStringSet&);
    bool		    copyDataAndAddToDelList(BufferStringSet&,
						    BufferStringSet&,
						    BufferString&);
    void		    update2DSFiles(ObjectSet<IOObj>& ioobjlist);
    void		    removeDuplicateData(BufferStringSet&);
    

    BufferString	    getAttrFolderPath(const IOPar&) const;

    	    
    ObjectSet<IOPar>	    all2dseisiopars_;
};

#endif
