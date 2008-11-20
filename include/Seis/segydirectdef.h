#ifndef segydirect_h
#define segydirect_h

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Bert
 Date:		Jul 2008
 RCS:		$Id: segydirectdef.h,v 1.6 2008-11-20 13:26:15 cvsbert Exp $
________________________________________________________________________

-*/

#include "seistype.h"


namespace SEGY {

class Scanner;
class FileDataSet;


class DirectDef
{
public:

    			DirectDef(Seis::GeomType);	//!< Create empty
    			DirectDef(const char*);		//!< Read from file
			~DirectDef();

    void		setData(FileDataSet*);
    void		setData(const FileDataSet&,bool no_copy=false);

    bool		readFromFile(const char*);
    bool		writeToFile(const char*) const;

    Seis::GeomType	geomType() const	{ return geom_; }   

    static const char*	sKeyDirectDef;

protected:

    Seis::GeomType	geom_;
    const FileDataSet*	fds_;

    int			curfidx_;

private:

    FileDataSet*	myfds_;
};

}

#endif
