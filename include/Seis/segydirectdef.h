#ifndef segydirect_h
#define segydirect_h

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Bert
 Date:		Jul 2008
 RCS:		$Id: segydirectdef.h,v 1.9 2008-12-02 16:10:39 cvsbert Exp $
________________________________________________________________________

-*/

#include "segyfiledata.h"
namespace Seis { class PosIndexer; }
namespace PosInfo { class CubeData; class Line2DData; }


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

    FileDataSet::TrcIdx	find(const Seis::PosKey&,bool chkoffs) const;

    bool		readFromFile(const char*);
    bool		writeToFile(const char*) const;
    const char*		errMsg() const		{ return errmsg_.buf(); }

    Seis::GeomType	geomType() const;

    static const char*	sKeyDirectDef;

    void		getPosData(PosInfo::CubeData&) const;
    void		getPosData(PosInfo::Line2DData&) const;

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
