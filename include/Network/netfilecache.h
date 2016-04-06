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

			// Use
    FileSizeType	size() const		    { return filesize_; }
    bool		isAvailable(FilePosType,ChunkSizeType) const;
    void		getAt(FilePosType,BufType*,ChunkSizeType) const;

			// Stuff data
    FileChunkSetType	neededFill(FilePosType,ChunkSizeType) const;
    void		fill(FileChunkType,const BufType*);

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
	Interval<PosType>	fillrg_;

	inline bool		hasFill() const
				{ return fillrg_.stop >= fillrg_.start; }

	static const SizeType	cFullSize;
    };
    typedef ObjectSet<Block>::size_type	BlockIdxType;

    const FileSizeType	filesize_;
    ObjectSet<Block>	blocks_;
    const Block::SizeType lastblocksz_;
    const FilePosType	lastblockpos_;

    static FilePosType	blockStart(BlockIdxType);
    Block::SizeType	blockSize(BlockIdxType) const;
    void		dismissBlock(BlockIdxType);
    void		newBlockAdded();
    BlockIdxType	blockIdx(FilePosType) const;
    Block*		getBlock(BlockIdxType) const;

};

} // namespace Network

#endif
