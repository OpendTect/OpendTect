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

			FileCache(FileSizeType);
			~FileCache();

    bool		isEmpty() const		    { return filesize_ < 1; }
    void		clearData();

			// General access

    			// Free-sized access
    FileSizeType	size() const		    { return filesize_; }
    bool		isAvailable(FilePosType,ChunkSizeType) const;
    void		getAt(FilePosType,BufType*,ChunkSizeType) const;

			// Stuff data
    FileChunkSetType	neededFill(FilePosType,ChunkSizeType) const;
    bool		fill(FileChunkType,const BufType*);
			//!< if fail, memory was full; now the cache is cleared

protected:

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
    BlockIdxType	blockIdx(FilePosType) const;
    static FilePosType	blockStart(BlockIdxType);
    Block::SizeType	blockSize(BlockIdxType) const;

    bool		haveBlock(BlockIdxType) const;
    BufType*		getBlock(BlockIdxType);
    const BufType*	getBlock(BlockIdxType) const;

protected:

    const FileSizeType	filesize_;
    ObjectSet<Block>	blocks_;
    const Block::SizeType lastblocksz_;
    const FilePosType	lastblockpos_;

    void		dismissBlock(BlockIdxType);
    void		newBlockAdded();
    Block*		gtBlk(BlockIdxType) const;
    inline FilePosType	lastFilePos() const
    			{ return lastblockpos_ + lastblocksz_ - 1; }

};

} // namespace Network

#endif
