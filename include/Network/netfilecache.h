#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Bert
 Date:		April 2016
________________________________________________________________________

-*/

#include "networkmod.h"
#include "ranges.h"
#include "sets.h"
#include "od_iosfwd.h"


namespace Network
{

/*!< Cache for files in blocks of a couple of MBs. Not MT protected. */

mExpClass(Network) FileCache
{
public:

    typedef unsigned char		BufType;
    typedef int				ChunkSizeType;
    typedef od_stream_Count		file_size_type;
    typedef od_stream_Pos		FilePosType;
    typedef Interval<FilePosType>	FileChunkType;
    typedef TypeSet<FileChunkType>	FileChunkSetType;

    virtual		~FileCache();

    file_size_type	size() const;
    bool		isEmpty() const			{ return size() < 1; }
    virtual void	clearData()			= 0;

    virtual void	setMinCacheSize(file_size_type) = 0;

protected:

			FileCache(file_size_type);

    class Block
    {
    public:

	typedef ChunkSizeType	SizeType;
	typedef SizeType	PosType;
	static const SizeType	cFullSize;

			Block(PosType);
			~Block();

	BufType*	buf_ = nullptr;
	SizeType	bufsz_;

    };

public:

    // Block-based access

    typedef ObjectSet<Block>::size_type block_idx_type;
    typedef Block::SizeType		BlockSizeType;

    block_idx_type	blockIdx(FilePosType) const;
    static FilePosType	blockStart(block_idx_type);
    BlockSizeType	blockSize(block_idx_type) const;
    inline bool		validBlockIdx( block_idx_type bidx ) const
			{ return blocks_.validIdx(bidx); }

    bool		isLiveBlock(block_idx_type) const;
    BufType*		getBlock(block_idx_type);
    const BufType*	getBlock(block_idx_type) const;

protected:

    ObjectSet<Block>	blocks_;
    const BlockSizeType lastblocksz_;
    const file_size_type knownfilesize_;

    Block*		gtBlk(block_idx_type) const;
    void		dismissBlock(block_idx_type);
    void		clearBlocks();
    inline FilePosType	lastBlockPos() const
			{ return blockStart(blocks_.size()-1); }
    inline FilePosType	lastFilePos() const
			{ return lastBlockPos() + lastblocksz_ - 1; }

    virtual void	handleNewLiveBlock(block_idx_type)	{}

};


/*!< Read Cache.

  When reading, we need some buffering to avoid lots of HTTP requests. The idea
  if to stuff data into blocks, and keep track of the 'oldest' blocks so we can
  dump them as we go (we'll haveto, otherwise we end up holding the entire
  file in memory). As headers and trailers are often revisited, we will always
  keep the first and last block.

*/

mExpClass(Network) ReadCache : public FileCache
{
public:

			ReadCache(file_size_type knownsize=0);
			~ReadCache();

    virtual void	clearData();
    virtual void	setMinCacheSize(file_size_type);

			// Free-sized access of buffered data
    bool		isAvailable(FilePosType,file_size_type) const;
    file_size_type	getAt(FilePosType,BufType*,file_size_type) const;
			// returns actual nr bytes handled

			// Stuff data, presumably obtained from remote source
    FileChunkSetType	stillNeededDataFor(FilePosType,ChunkSizeType) const;
    bool		setData(FileChunkType,const BufType*);
			//!< if fail, memory was full; now the cache is cleared

protected:

    TypeSet<block_idx_type> liveblockidxs_;
    block_idx_type	maxnrliveblocks_;

    virtual void	handleNewLiveBlock(block_idx_type);

};


/*!< Write Cache.

  When writing, we need a buffer to write data to. To allow jumping around a
  bit, we'll have to retain the first block (so a header can be updated later),
  and a couple of blocks before the last block. Therefore, the cache assumes
  writing steadily on, with maybe going back to the first block, or one of the
  near previous ones. Random jumping around may lead to bad stuff.

*/

mExpClass(Network) WriteCache : public FileCache
{
public:

			WriteCache();
			~WriteCache();

    virtual void	clearData();
    virtual void	setMinCacheSize(file_size_type);


protected:

    block_idx_type	nrblocksmem_;

    virtual void	handleNewLiveBlock(block_idx_type);

};

} // namespace Network
