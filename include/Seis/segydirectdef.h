#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "seismod.h"
#include "segyfiledata.h"

#include "bufstringset.h"
#include "executor.h"
#include "od_iosfwd.h"
#include "posgeomid.h"
#include "uistring.h"

class IOObj;
namespace Seis { class PosIndexer; }
namespace PosInfo { class CubeData; class Line2DData; }


namespace SEGY
{

class FileSpec;
class Scanner;
class FileDataSet;
class PosKeyList;


mExpClass(Seis) DirectDef
{ mODTextTranslationClass(DirectDef);
public:

			DirectDef();			//!< Create empty
			DirectDef(const char*);		//!< Read from file
			~DirectDef();
    bool		isEmpty() const;

			//Functions to read/query
    bool		readFromFile(const char*);
    const IOPar*	segyPars() const;
    StringView		fileName(int idx) const;
    FileDataSet::TrcIdx	find(const Seis::PosKey&,bool chkoffs) const;
    FileDataSet::TrcIdx	findOcc(const Seis::PosKey&,int occ) const;
			//!< will not look at offset

			//Functions to write
    void		setData(FileDataSet&);
    bool		writeHeadersToFile(const char*);
			/*!<Write the headers. After calling, the fds should
			    be dumped into the stream. */
    od_ostream*		getOutputStream()	{ return outstream_; }
    bool		writeFootersToFile();
			/*!<After fds has been dumped, write the
			    remainder of the file */

    const FileDataSet&	fileDataSet() const	{ return *fds_; }
    uiString		errMsg() const		{ return errmsg_; }

    static const char*	sKeyDirectDef();
    static const char*	sKeyFileType();
    static const char*	sKeyNrFiles();
    static const char*	sKeyIOCompr();
    static const char*	sKeyInt64DataChar();
    static const char*	sKeyInt32DataChar();
    static const char*	sKeyFloatDataChar();

    static const char*	get2DFileName(const char*,const char*);
    static const char*	get2DFileName(const char*,Pos::GeomID);

    static bool		readFooter(const char* fnm,IOPar&,od_stream_Pos&);
			/*!<Reads the Footer into an IOPar */
    static bool		updateFooter(const char*,const IOPar&,od_stream_Pos);
			/*!<Updates the Footer IOPar in an existing def file */

    const PosInfo::CubeData&	cubeData() const { return cubedata_; }
    const PosInfo::Line2DData&	lineData() const { return linedata_; }
    const TypeSet<float>&	spnrs() const	 { return spnrs_; }


protected:
    void		getPosData(PosInfo::CubeData&) const;
    void		getPosData(PosInfo::Line2DData&,TypeSet<float>&) const;

    PosInfo::CubeData&	 cubedata_;
    PosInfo::Line2DData& linedata_;
    TypeSet<float>	 spnrs_;

    const FileDataSet*	fds_ = nullptr;
    FileDataSet*	myfds_ = nullptr;
    SEGY::PosKeyList*	keylist_ = nullptr;
    Seis::PosIndexer*	indexer_ = nullptr;

    mutable uiString	errmsg_;

    od_ostream*		outstream_ = nullptr;
    od_stream_Pos	offsetstart_;
    od_stream_Pos	datastart_;
    od_stream_Pos	cubedatastart_;
    od_stream_Pos	indexstart_;
    od_stream_Pos	finalparstart_;
};


/*!Scans a file and creates an index file that can be read by OD. */
mExpClass(Seis) FileIndexer : public Executor
{ mODTextTranslationClass(FileIndexer);
public:
			FileIndexer(const MultiID& mid,bool isvol,
				    const FileSpec&,bool is2d,const IOPar&);
			~FileIndexer();

    int			nextStep() override;

    uiString		uiMessage() const override;
    od_int64		nrDone() const override;
    od_int64		totalNr() const override;
    uiString		uiNrDoneText() const override;

    const Scanner*	scanner() const		{ return scanner_; }
    Scanner*		scanner()		{ return scanner_; }

protected:

    IOObj*		ioobj_;
    Pos::GeomID		geomid_;

    Scanner*		scanner_ = nullptr;
    mutable uiString	msg_;
    DirectDef*		directdef_ = nullptr;
    bool		is2d_;
    bool		isvol_;

};

} // namespace SEGY
