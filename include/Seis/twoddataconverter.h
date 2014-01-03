#ifndef twoddataconverter_h
#define twoddataconverter_h

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

mExpClass(Seis) TwoDDataConverter
{
public:
			    TwoDDataConverter()	    {}

    bool		    convert2DDataToNewFormat(BufferString& errmsg);
protected:

    void		    createListOfLinesetIOObjs(ObjectSet<IOObj>&) const;
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
