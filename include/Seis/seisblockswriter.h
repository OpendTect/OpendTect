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
class od_ostream;

class Task;

namespace Seis
{
namespace Blocks
{

class ColumnWriter;
class MemBlock;
class MemBlockColumn;
class StepFinder;
class WriterFinisher;


/*!\brief Writes provided data into Blocks Storage.

  The writer accepts trace data which it will distribute amongst in-memory
  blocks. When a column of blocks is fully filled it will be written and each
  block is retired (i.e. its databuffer is emptied).

  All files are put in a subdir of the base path. At the end the columns
  that have never been fully filled (edge columns, columns with data gaps) will
  be written. Lastly, the main file ".info" will be written.

  The Writer is prepared to be used in a MT setting, but the actual output
  writing may not be parallel.

*/

mExpClass(Seis) Writer : public IOClass
{ mODTextTranslationClass(Seis::Blocks::Writer);
public:

			Writer(const HGeom* sg=0);
			~Writer();

    void		setFullPath(const char*); //!< with or w/o extension
    void		setFileNameBase(const char*);
    void		setCubeName(const char*);
    void		setZDomain(const ZDomain::Def&);
    void		setFPRep(OD::FPDataRepType);
    void		setScaler(const LinScaler*);
    void		addComponentName(const char*);
    void		addAuxInfo(const char* key,const IOPar&);

    uiRetVal		add(const SeisTrc&);
    Task*		finisher();
			//!< if not run by you, destructor will run it
			//!< be sure that all add()'s are done!

    typedef std::pair<IdxType,float>	ZEvalPos;
    typedef TypeSet<ZEvalPos>		ZEvalPosSet;

    inline int		nrColumnBlocks() const
			{ return zevalpositions_.size(); }
    void		setBasePath(const File::Path&);

protected:

    OD::FPDataRepType	specifiedfprep_;
    int			nrcomps_;
    bool		isfinished_;
    DataInterp*		interp_;
    od_ostream*		strm_;

    IdxType		nrglobzidxs_;
    ObjectSet<ZEvalPosSet> zevalpositions_;
    StepFinder*		stepfinder_;
    Interval<int>	finalinlrg_;
    Interval<int>	finalcrlrg_;

    virtual void	setEmpty();
    void		resetZ();
    void		doAdd(const SeisTrc&,uiRetVal&);
    void		add2Block(MemBlock&,const ZEvalPosSet&,const HLocIdx&,
				    const SeisTrc&,int);
    MemBlockColumn*	getColumn(const HGlobIdx&);
    MemBlockColumn*	mkNewColumn(const HGlobIdx&);
    bool		isCompleted(const MemBlockColumn&) const;
    void		writeColumn(MemBlockColumn&,uiRetVal&);
    bool		writeColumnHeader(const MemBlockColumn&,
				    const HLocIdx&,const HDimensions&) const;
    bool		writeBlock(MemBlock&,HLocIdx,HDimensions);
    void		writeInfoFiles(uiRetVal&);
    bool		writeInfoFileData(od_ostream&);
    bool		writeOverviewFileData(od_ostream&);
    void		scanPositions(PosInfo::CubeData& cubedata,
			    Interval<IdxType>&,Interval<IdxType>&,
			    Interval<double>&,Interval<double>&);

    friend class	StepFinder;
    friend class	ColumnWriter;
    friend class	WriterFinisher;

};


} // namespace Blocks

} // namespace Seis
