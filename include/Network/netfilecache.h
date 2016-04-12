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

			// Free-sized access of buffered data
    FileSizeType	size() const		    { return filesize_; }
    bool		isAvailable(FilePosType,FileSizeType) const;
    FileSizeType	getAt(FilePosType,BufType*,FileSizeType) const;
			// returns actual nr bytes handled

			// Stuff data, presumably obtained from remote source
    FileChunkSetType	stillNeededDataFor(FilePosType,ChunkSizeType) const;
    bool		setData(FileChunkType,const BufType*);
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
    typedef Block::SizeType		BlockSizeType;

    BlockIdxType	blockIdx(FilePosType) const;
    static FilePosType	blockStart(BlockIdxType);
    BlockSizeType	blockSize(BlockIdxType) const;
    inline bool		validBlockIdx( BlockIdxType bidx ) const
			{ return blocks_.validIdx(bidx); }

    bool		hasBlock(BlockIdxType) const;
    BufType*		getBlock(BlockIdxType);
    const BufType*	getBlock(BlockIdxType) const;

protected:

    const FileSizeType	filesize_;
    ObjectSet<Block>	blocks_;
    const BlockSizeType	lastblocksz_;
    const FilePosType	lastblockpos_;

    void		dismissBlock(BlockIdxType);
    void		newBlockAdded();
    Block*		gtBlk(BlockIdxType) const;
    inline FilePosType	lastFilePos() const
			{ return lastblockpos_ + lastblocksz_ - 1; }

};

} // namespace Network

#endif
