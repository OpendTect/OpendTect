#pragma once

/*
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Bert
 Date:		Apr 2017
________________________________________________________________________

*/

#include "seisblocksaccess.h"
#include "filepath.h"
#include "uistring.h"
#include "ranges.h"
#include "iopar.h"
#include "zdomain.h"
class od_ostream;
class ascostream;
class Task;
template <typename T> class Array2D;

namespace Seis
{
namespace Blocks
{

class ColumnWriter;
class HDF5WriteBackEnd;
class MemBlock;
class MemBlockColumn;
class StepFinder;
class StreamWriteBackEnd;
class WriteBackEnd;
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

mExpClass(Seis) Writer : public Access
{ mODTextTranslationClass(Seis::Blocks::Writer);
public:

			Writer(const HGeom* sg=0);
			~Writer();

    void		setFullPath(const char*); //!< with or w/o extension
    void		setFileNameBase(const char*);
    void		setCubeName(const char*);
    void		setDataRep(OD::DataRepType);
    void		setScaler(const LinScaler*);
    void		addComponentName(const char*);
    void		addAuxInfo(const char* key,const IOPar&);

    uiRetVal		add(const SeisTrc&);
    Task*		finisher();
			//!< if not run by you, destructor will run it
			//!< be sure that all add()'s are done!

    typedef std::pair<idx_type,float>	ZEvalPos;
    typedef TypeSet<ZEvalPos>		ZEvalPosSet;

    int			traceSize() const;
    inline int		nrColumnBlocks() const
			{ return zevalpositions_.size(); }
    void		setBasePath(const File::Path&);
    void		setZDomain(const ZDomain::Def&);

protected:

    OD::DataRepType	specifieddatarep_;
    int			nrcomps_;
    bool		isfinished_;
    DataInterp*		interp_;
    WriteBackEnd*	backend_;
    IOPar		infoiop_;

    size_type		nrglobzidxs_;
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
    void		writeInfoFiles(uiRetVal&);
    bool		writeInfoFileData(od_ostream&);
    bool		writeOverviewFileData(od_ostream&);
    bool		writeFullSummary(ascostream&,
				const Array2D<float>&) const;
    void		writeLevelSummary(od_ostream&,
				const Array2D<float>&,int_twins) const;
    void		scanPositions(Interval<idx_type>&,Interval<idx_type>&,
				Interval<double>&,Interval<double>&);

    friend class	StepFinder;
    friend class	ColumnWriter;
    friend class	WriterFinisher;
    friend class	StreamWriteBackEnd;
    friend class	HDF5WriteBackEnd;

};


mExpClass(Seis) WriteBackEnd
{
public:

			WriteBackEnd( Writer& wrr ) : wrr_(wrr)		{}
    virtual		~WriteBackEnd()					{}

    virtual void	setColumnInfo(const MemBlockColumn&,const HLocIdx&,
				  const HDimensions&,uiRetVal&)		= 0;
    virtual void	putBlock(int,MemBlock&,HLocIdx,HDimensions,
				 uiRetVal&)				= 0;
    virtual void	close(uiRetVal&)				= 0;

    Writer&		wrr_;

};


} // namespace Blocks

} // namespace Seis
