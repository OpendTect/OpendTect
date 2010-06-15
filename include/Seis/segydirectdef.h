#ifndef segydirect_h
#define segydirect_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Bert
 Date:		Jul 2008
 RCS:		$Id: segydirectdef.h,v 1.14 2010-06-15 20:48:39 cvskris Exp $
________________________________________________________________________

-*/

#include "segyfiledata.h"
#include "bufstringset.h"

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
    			DirectDef(const char*);	//!< Read from file
			~DirectDef();
    bool		isEmpty() const;

    void		setData(FileDataSet*);
    void		setData(const FileDataSet&,bool no_copy=false);

    FixedString		fileName(int idx) const;
    const IOPar&	segyPars() const;

    FileDataSet::TrcIdx	find(const Seis::PosKey&,bool chkoffs) const;

    bool		readFromFile(const char*);
    bool		writeToFile(const char*) const;
    const char*		errMsg() const		{ return errmsg_.buf(); }

    static const char*	sKeyDirectDef;
    static const char*	sKeyFileType;
    static const char*	sKeyNrFiles;
    static const char*	sKeyInt64DataChar;
    static const char*	sKeyInt32DataChar;
    static const char*	sKeyFloatDataChar;
    static const char*	get2DFileName(const char*,const char*);

    const PosInfo::CubeData&	cubeData() const { return cubedata_; }
    const PosInfo::Line2DData&	lineData() const { return linedata_; }

protected:
    bool		readV1FromFile(const IOPar&, ascistream&, const char* );
    void		getPosData(PosInfo::CubeData&) const;
    void		getPosData(PosInfo::Line2DData&) const;


    PosInfo::CubeData&	 cubedata_;
    PosInfo::Line2DData& linedata_;

    const FileDataSet*	fds_;
    BufferStringSet	filenames_;
    IOPar		segypars_;
    SEGY::PosKeyList*	keylist_;
    Seis::PosIndexer*	indexer_;

    int			curfidx_;
    mutable BufferString errmsg_;

private:

    FileDataSet*	myfds_;
};

}

#endif
