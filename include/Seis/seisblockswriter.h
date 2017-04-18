#pragma once

/*
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Bert
 Date:		Apr 2017
________________________________________________________________________

*/

#include "seisblocks.h"
#include "filepath.h"
#include "uistring.h"
#include "ranges.h"
#include "od_iosfwd.h"

class Task;
class SeisTrc;
namespace Pos { class IdxPairDataSet; }
namespace PosInfo { class CubeData; }


namespace Seis
{
namespace Blocks
{

class MemBlock;
class MemBlockColumn;


/*!\brief Writes provided data into Blocks Storage.

  The writer accepts trace data which it will distribute amongst in-memory
  blocks. When a column of blocks is fully filled it will be written and each
  block is retired (i.e. its databuffer is emptied).

  All files are put in a subdir of the base path. At the end the columns
  that have never been fully filled (edge columns, columns with data gaps) will
  be written. Lastly, the main file ".cube" will be written.

*/

mExpClass(Seis) Writer : public IOClass
{ mODTextTranslationClass(Seis::Blocks::Writer);
public:

			Writer(const SurvGeom* sg=0);
			~Writer();

    const SurvGeom&	survGeom() const	{ return survgeom_; }

    void		setBasePath(const File::Path&);
    void		setFileNameBase(const char*);
    void		setCubeName(const char*);
    void		setFPRep(OD::FPDataRepType);
    void		setScaler(const LinScaler*);
    void		addComponentName(const char*);
    void		addAuxInfo(const char* key,const IOPar&);

    uiRetVal		add(const SeisTrc&);
    Task*		finisher();
			//!< if not run by you, destructor will run it

    typedef std::pair<IdxType,float>	ZEvalPos;
    typedef TypeSet<ZEvalPos>		ZEvalPosSet;

    inline int		nrColumnBlocks() const
			{ return zevalpositions_.size(); }

protected:

    const SurvGeom&	survgeom_;
    OD::FPDataRepType	specfprep_;
    const int		nrpospercolumn_;
    int			nrcomponents_;
    bool		isfinished_;
    Interval<IdxType>	globzidxrg_;
    ObjectSet<ZEvalPosSet> zevalpositions_;
    Pos::IdxPairDataSet& columns_;

    void		setEmpty();
    void		resetZ(const Interval<float>&);
    bool		removeExisting(const char*,uiRetVal&) const;
    bool		prepareWrite(uiRetVal&);
    void		add2Block(MemBlock&,const ZEvalPosSet&,SampIdx,
				    const SeisTrc&,int);
    MemBlockColumn*	getColumn(const GlobIdx&);
    MemBlockColumn*	mkNewColumn(const GlobIdx&);
    bool		isCompletionVisit(MemBlockColumn&,const SampIdx&) const;
    void		writeColumn(MemBlockColumn&,uiRetVal&);
    bool		writeColumnHeader(od_ostream&,const MemBlockColumn&,
				    const SampIdx&,const Dimensions&) const;
    bool		writeBlock(od_ostream&,MemBlock&,SampIdx,Dimensions);
    void		writeMainFile(uiRetVal&);
    bool		writeMainFileData(od_ostream&);
    void		scanPositions(PosInfo::CubeData& cubedata,
			    Interval<IdxType>&,Interval<IdxType>&,
		            Interval<int>&,Interval<int>&,
			    Interval<double>&,Interval<double>&);

    friend class	ColumnWriter;
    friend class	WriterFinisher;

};


} // namespace Blocks

} // namespace Seis
