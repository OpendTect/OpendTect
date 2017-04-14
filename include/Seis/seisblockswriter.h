#pragma once

/*
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Bert
 Date:		Apr 2017
________________________________________________________________________

*/

#include "seisblocksdata.h"
#include "seistrc.h"
#include "filepath.h"
#include "uistring.h"

class Task;
class LinScaler;
class SeisTrc;
namespace Pos { class IdxPairDataSet; }


namespace Seis
{
namespace Blocks
{

class Data;

/*!\brief Writes provided data into Block Storage.

  The writer accepts trace data which it will distribute amongst in-memory
  blocks. When a block is fully filled it will be written and the block is
  retired (i.e. its databuffer is emptied).

  All block data is put in a subdir of the base path. At the end the blocks
  that have never been fully filled (edge blocks, blocks with data gaps) will
  be written. Lastly, the main file ".cube" will be written.

*/

mExpClass(Seis) Writer : public DataStorage
{ mODTextTranslationClass(Seis::Blocks::Writer);
public:

			Writer(const SurvGeom* sg=0);
			~Writer();

    void		setBasePath(const File::Path&);
    void		setFileNameBase(const char*);
    void		setFPRep(OD::FPDataRepType);
    void		setScaler(const LinScaler*);
    void		setComponent(int);

    BufferString	dirName() const;
    BufferString	mainFileName() const;

    uiRetVal		add(const SeisTrc&);
    Task*		finisher();
			//!< if not run by you, destructor will run it

protected:

    typedef std::pair<IdxType,float>	ZEvalPos;
    typedef TypeSet<ZEvalPos>		ZEvalPosSet;

    struct ZEvalInfo
    {
			ZEvalInfo( IdxType gidx )   : globidx_(gidx)	{}
	const IdxType	globidx_;
	ZEvalPosSet	evalpositions_;
    };
    struct Block
    {
			Block() : data_(0), nruniquevisits_(0)	{}
			~Block()				{ delete data_;}
	Data*		data_;
	BoolTypeSet	visited_;
	int		nruniquevisits_;
    };

    File::Path		basepath_;
    BufferString	filenamebase_;
    OD::FPDataRepType	specfprep_;
    OD::FPDataRepType	usefprep_;
    int			component_;
    LinScaler*		scaler_;
    bool		needreset_;
    bool		writecomplete_;
    const int		nrposperblock_;

    Interval<IdxType>	globzidxrg_;
    ObjectSet<ZEvalInfo> zevalinfos_;
    Pos::IdxPairDataSet& blocks_;

    void		setEmpty();
    void		resetZ(const Interval<float>&);
    bool		removeExisting(const char*,uiRetVal&) const;
    bool		prepareWrite(uiRetVal&);
    bool		add2Block(const GlobIdx&,const SeisTrc&,
				  const ZEvalPosSet&,uiRetVal&);
    Block*		getBlock(const GlobIdx&);
    bool		isCompletionVisit(Block&,const SampIdx&) const;
    void		writeBlock(Block&,uiRetVal&);
    bool		writeBlockHeader(od_ostream&,Data&);
    bool		writeBlockData(od_ostream&,Data&);
    void		writeMainFile(uiRetVal&);

    friend class	WriterFinisher;

};


} // namespace Blocks

} // namespace Seis
