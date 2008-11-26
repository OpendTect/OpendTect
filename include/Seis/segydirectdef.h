#ifndef segydirect_h
#define segydirect_h

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Bert
 Date:		Jul 2008
 RCS:		$Id: segydirectdef.h,v 1.8 2008-11-26 14:43:46 cvsbert Exp $
________________________________________________________________________

-*/

#include "seistype.h"
#include "bufstring.h"
namespace Seis { class PosIndexer; }


namespace SEGY {

class Scanner;
class FileDataSet;
class PosKeyList;


class DirectDef
{
public:

    			DirectDef();			//!< Create empty
    			DirectDef(const char*);		//!< Read from file
			~DirectDef();
    bool		isEmpty() const;

    void		setData(FileDataSet*);
    void		setData(const FileDataSet&,bool no_copy=false);

    bool		readFromFile(const char*);
    bool		writeToFile(const char*) const;
    const char*		errMsg() const		{ return errmsg_.buf(); }

    Seis::GeomType	geomType() const;

    static const char*	sKeyDirectDef;

protected:

    const FileDataSet*	fds_;
    SEGY::PosKeyList&	keylist_;
    Seis::PosIndexer&	indexer_;

    int			curfidx_;
    mutable BufferString errmsg_;

private:

    FileDataSet*	myfds_;
};

}

#endif
