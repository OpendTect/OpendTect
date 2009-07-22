#ifndef segydirect_h
#define segydirect_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Bert
 Date:		Jul 2008
 RCS:		$Id: segydirectdef.h,v 1.12 2009-07-22 16:01:18 cvsbert Exp $
________________________________________________________________________

-*/

#include "segyfiledata.h"
namespace Seis { class PosIndexer; }
namespace PosInfo { class CubeData; class Line2DData; }


namespace SEGY {

class Scanner;
class FileDataSet;
class PosKeyList;


mClass DirectDef
{
public:

    			DirectDef();			//!< Create empty
    			DirectDef(const char*);		//!< Read from file
			~DirectDef();
    bool		isEmpty() const;

    const FileDataSet*	dataSet() const			{ return fds_; }
    void		setData(FileDataSet*);
    void		setData(const FileDataSet&,bool no_copy=false);

    FileDataSet::TrcIdx	find(const Seis::PosKey&,bool chkoffs) const;

    bool		readFromFile(const char*);
    bool		writeToFile(const char*) const;
    const char*		errMsg() const		{ return errmsg_.buf(); }

    Seis::GeomType	geomType() const;

    static const char*	sKeyDirectDef;
    static const char*	get2DFileName(const char*,const char*);

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
