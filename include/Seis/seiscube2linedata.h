#ifndef seiscube2linedata_h
#define seiscube2linedata_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Bert
 Date:		Apr 2010
 RCS:		$Id$
________________________________________________________________________

-*/
 
#include "seismod.h"
#include "executor.h"
#include "bufstringset.h"
class IOObj;
class SeisTrcBuf;
class Seis2DDataSet;
class Seis2DLineSet;
class SeisTrcReader;
class SeisTrcWriter;
class Cube2LineDataLineKeyProvider;
namespace Survey { class Geometry2D; }


/*!\brief Extracts 2D data from 3D Cube */

mExpClass(Seis) Seis2DFrom3DExtractor : public Executor
{
public:
			Seis2DFrom3DExtractor(const IOObj& cubein,
					      const IOObj& lsout,
					      const TypeSet<Pos::GeomID>&);
			~Seis2DFrom3DExtractor();

    uiString		uiMessage() const	{ return msg_; }
    uiString		uiNrDoneText() const	{ return "Traces written"; }
    od_int64		nrDone() const		{ return nrdone_; }
    od_int64		totalNr() const		{ return totalnr_; }
    Pos::GeomID		curGeomID() const;

    int			nextStep();

protected:

    SeisTrcReader&	rdr_;
    SeisTrcWriter&	wrr_;
    uiString		msg_;

    od_int64		nrdone_;
    od_int64		totalnr_;

    int			curlineidx_;
    int			curtrcidx_;

    const TypeSet<Pos::GeomID>& geomids_;
    const Survey::Geometry2D*	curgeom2d_;

    int			goToNextLine();
    int			handleTrace();

};

/*!\brief Obsolete class: Extracted 3D cube data into 2D line attribute */

mExpClass(Seis) SeisCube2LineDataExtracter : public Executor
{
public:
			SeisCube2LineDataExtracter(const IOObj& cubein,
					   const IOObj& lsout,
					   const char* attrnm,
					   const BufferStringSet* lnms=0);
			~SeisCube2LineDataExtracter();

    uiString		uiMessage() const	{ return msg_; }
    uiString		uiNrDoneText() const	{ return "Traces written"; }
    od_int64		nrDone() const		{ return nrdone_; }
    od_int64		totalNr() const 	{ return totalnr_; }

    int 		nextStep();

protected:

    SeisTrcReader&	rdr_;
    Seis2DLineSet&	ls_;
    SeisTrcWriter&	wrr_;
    SeisTrcBuf&		tbuf_;
    const BufferString	attrnm_;
    uiString		msg_;
    BufferStringSet	lnms_;
    
    ObjectSet<Executor>	fetchers_; //linked with usedlinenames_
    BufferStringSet	usedlinenames_;
    
    od_int64		nrdone_;
    od_int64		totalnr_;
    Cube2LineDataLineKeyProvider* c2ldlkp_;

    void		closeDown();
    bool		getFetchers();
    int			handleTrace();

    friend class	Cube2LineDataLineKeyProvider;

};


#endif

