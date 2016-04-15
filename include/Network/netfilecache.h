#ifndef netfilecache_h
#define netfilecache_h

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
    typedef od_stream_Count		FileSizeType;
    typedef od_stream_Pos		FilePosType;
    typedef Interval<FilePosType>	FileChunkType;
    typedef TypeSet<FileChunkType>	FileChunkSetType;

    virtual		~FileCache();

    FileSizeType	size() const;
    bool		isEmpty() const		    { return size() < 1; }
    virtual void	clearData()		    = 0;

protected:

			FileCache(FileSizeType);

    class Block
    {
    public:

	typedef ChunkSizeType	SizeType;
	typedef SizeType	PosType;

				Block(PosType);
				~Block();

	BufType*		buf_;
	SizeType		bufsz_;

	static const SizeType	cFullSize;
    };

public:

    // Block-based access

    typedef ObjectSet<Block>::size_type	BlockIdxType;
    typedef Block::SizeType		BlockSizeType;

    BlockIdxType	blockIdx(FilePosType) const;
    static FilePosType	blockStart(BlockIdxType);
    BlockSizeType	blockSize(BlockIdxType) const;
    inline bool		validBlockIdx( BlockIdxType bidx ) const
			{ return blocks_.validIdx(bidx); }

    bool		isLiveBlock(BlockIdxType) const;
    BufType*		getBlock(BlockIdxType);
    const BufType*	getBlock(BlockIdxType) const;

protected:

    ObjectSet<Block>	blocks_;
    const BlockSizeType	lastblocksz_;
    const FileSizeType	knownfilesize_;

    Block*		gtBlk(BlockIdxType) const;
    void		dismissBlock(BlockIdxType);
    void		clearBlocks();
    inline FilePosType	lastBlockPos() const
			{ return blockStart(blocks_.size()-1); }
    inline FilePosType	lastFilePos() const
			{ return lastBlockPos() + lastblocksz_ - 1; }

    virtual void	handleNewLiveBlock(BlockIdxType)	{}

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

			ReadCache(FileSizeType);
			~ReadCache();

    virtual void	clearData();

    void		setMinCacheSize(FileSizeType);

			// Free-sized access of buffered data
    bool		isAvailable(FilePosType,FileSizeType) const;
    FileSizeType	getAt(FilePosType,BufType*,FileSizeType) const;
			// returns actual nr bytes handled

			// Stuff data, presumably obtained from remote source
    FileChunkSetType	stillNeededDataFor(FilePosType,ChunkSizeType) const;
    bool		setData(FileChunkType,const BufType*);
			//!< if fail, memory was full; now the cache is cleared

protected:

    TypeSet<BlockIdxType> liveblockidxs_;
    BlockIdxType	maxnrliveblocks_;

    virtual void	handleNewLiveBlock(BlockIdxType);

};

} // namespace Network

#endif
