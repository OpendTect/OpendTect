/*+
________________________________________________________________________

(C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
Author:	    Salil Agarwal
Date:	    October 2012
________________________________________________________________________

-*/

#include "netfilecache.h"

//! read cache size unless more needed by a caller
static const Network::ReadCache::BlockIdxType cStartMaxLiveBlocks = 10;

const Network::FileCache::BlockSizeType
	Network::FileCache::Block::cFullSize = 2097152; // 2 MB
Network::FileCache::FilePosType
Network::FileCache::blockStart( BlockIdxType bidx )
{ return bidx < 0 ? 0 : ((FilePosType)bidx) * Block::cFullSize; }


Network::FileCache::Block::Block( SizeType sz )
    : buf_(0)
    , bufsz_(sz)
{
    try {
	buf_ = new BufType[ bufsz_ ];
    } catch ( std::bad_alloc )
	{ buf_ = 0; }
}


Network::FileCache::Block::~Block()
{
    delete [] buf_;
}


Network::FileCache::FileCache( FileSizeType knownfilesz )
    : knownfilesize_(knownfilesz)
    , lastblocksz_(Block::cFullSize)
{
    blocks_.allowNull( true );
    if ( knownfilesz < 1 )
	return;

    BlockIdxType nrblocks = (BlockIdxType)(knownfilesize_ / Block::cFullSize);
    for ( BlockIdxType iblk=0; iblk<nrblocks; iblk++ )
	blocks_ += 0;

    FilePosType lastblockpos = blockStart( nrblocks );
    BlockSizeType lastblksz = (BlockSizeType)(knownfilesize_ - lastblockpos);
    if ( lastblksz > 0 )
        blocks_ += 0;
    else
	lastblksz = Block::cFullSize;
    const_cast<BlockSizeType&>(lastblocksz_) = lastblksz;

}


Network::FileCache::~FileCache()
{
    deepErase( blocks_ );
}


Network::FileCache::FileSizeType Network::FileCache::size() const
{
    if ( knownfilesize_ > 0 )
	return knownfilesize_;

    const BlockIdxType nrblocks = blocks_.size();
    if ( nrblocks == 0 )
	return 0;

    FileSizeType ret = nrblocks - 1;
    if ( ret > 0 )
	ret *= Block::cFullSize;
    ret += lastblocksz_;
    return ret;
}


void Network::FileCache::dismissBlock( BlockIdxType iblk )
{
    if ( blocks_.validIdx(iblk) )
    {
	delete blocks_[iblk];
	blocks_.replace( iblk, 0 );
    }
}


void Network::FileCache::clearBlocks()
{
    for ( BlockIdxType iblk=0; iblk<blocks_.size(); iblk++ )
	dismissBlock( iblk );
}


Network::FileCache::Block* Network::FileCache::gtBlk( BlockIdxType iblk ) const
{
    FileCache& self = *const_cast<FileCache*>( this );
    while ( blocks_.size() <= iblk )
	self.blocks_ += 0;

    if ( !blocks_[iblk] )
    {
	Block* newblock = new Block( blockSize(iblk) );
	if ( !newblock->buf_ ) // emergency: mem full
	    { self.clearData(); return 0; }

	self.blocks_.replace( iblk, newblock );
	self.handleNewLiveBlock( iblk );
    }

    return self.blocks_[iblk];
}


Network::FileCache::BlockSizeType Network::FileCache::blockSize(
						BlockIdxType iblk ) const
{
    return iblk == blocks_.size() - 1 ? lastblocksz_ : Block::cFullSize;
}


Network::FileCache::BlockIdxType Network::FileCache::blockIdx(
						FilePosType pos ) const
{
    return (BlockIdxType)(pos / Block::cFullSize);
}


bool Network::FileCache::isLiveBlock( BlockIdxType bidx ) const
{
    return blocks_.validIdx(bidx) && blocks_[bidx];
}


Network::FileCache::BufType* Network::FileCache::getBlock( BlockIdxType bidx )
{
    Block* ret = blocks_.validIdx(bidx) ? gtBlk( bidx ) : 0;
    return ret ? ret->buf_ : 0;
}


const Network::FileCache::BufType* Network::FileCache::getBlock(
						BlockIdxType bidx ) const
{
    const Block* ret = blocks_.validIdx(bidx) ? gtBlk( bidx ) : 0;
    return ret ? ret->buf_ : 0;
}


//---- Read Cache


Network::ReadCache::ReadCache( FileSizeType knownfilesz )
    : FileCache(knownfilesz)
    , maxnrliveblocks_(cStartMaxLiveBlocks)
{
}


Network::ReadCache::~ReadCache()
{
}


void Network::ReadCache::clearData()
{
    clearBlocks();
    liveblockidxs_.setEmpty();
}


void Network::ReadCache::setMinCacheSize( FileSizeType csz )
{
    BlockIdxType reqnrblks = 2; // always keep first and last
    reqnrblks += (BlockIdxType)(csz / Block::cFullSize) + 2;
    if ( reqnrblks > maxnrliveblocks_ )
	maxnrliveblocks_ = reqnrblks;
}



void Network::ReadCache::handleNewLiveBlock( BlockIdxType addedblockidx )
{
    const BlockIdxType nrbidxs = liveblockidxs_.size();
    if ( nrbidxs >= maxnrliveblocks_ )
    {
	for ( BlockIdxType iblk=0; iblk<nrbidxs; iblk++ )
	{
	    const BlockIdxType curbidx = liveblockidxs_[iblk];
	    if ( curbidx != 0 && curbidx != blocks_.size()-1 )
	    {
		dismissBlock( iblk );
		liveblockidxs_.removeSingle( iblk );
		break;
	    }
	}
    }
    liveblockidxs_ += addedblockidx;
}


bool Network::ReadCache::isAvailable( FilePosType pos,
				      FileSizeType nrbytes ) const
{
    if ( nrbytes < 1 )
	return pos < size();

    const FilePosType lastpos = pos + nrbytes - 1;
    const BlockIdxType lastbidx = blockIdx( lastpos );
    for ( BlockIdxType bidx=blockIdx(pos); bidx<=lastbidx; bidx++ )
    {
	if ( !blocks_[bidx] )
	    return false;
    }
    return true;
}


Network::ReadCache::FileSizeType Network::ReadCache::getAt( FilePosType pos,
		BufType* out, FileSizeType totalnrbytes ) const
{
    if ( !out || totalnrbytes < 1 )
	return 0;
    else if ( !isAvailable(pos,totalnrbytes) )
    {
	pErrMsg( "Not all data requested previously filled" );
	OD::memZero( out, totalnrbytes );
    }

    FileSizeType byteshandled = 0;
    const FilePosType lastpos = pos + totalnrbytes - 1;
    const BlockIdxType lastbidx = blockIdx( lastpos );
    for ( BlockIdxType bidx=blockIdx(pos); bidx<=lastbidx; bidx++ )
    {
	const Block* blk = blocks_[bidx];
	FileSizeType nrbytes = Block::cFullSize;
	if ( blk )
	{
	    Interval<ChunkSizeType> wantedrg( 0, blk->bufsz_-1 );
	    const FilePosType blockstart = blockStart( bidx );
	    const FilePosType blockend = blockstart + blk->bufsz_ - 1;
	    if ( pos > blockstart )
		wantedrg.start = (ChunkSizeType)(pos - blockstart);
	    if ( lastpos < blockend )
		wantedrg.stop = (ChunkSizeType)(lastpos - blockstart);

	    nrbytes = wantedrg.stop - wantedrg.start + 1;
	    OD::memCopy( out+byteshandled, blk->buf_+wantedrg.start, nrbytes );
	}
	byteshandled += nrbytes;
    }

    return byteshandled;
}


Network::ReadCache::FileChunkSetType Network::ReadCache::stillNeededDataFor(
		FilePosType pos, ChunkSizeType nrbytes ) const
{
    FileChunkSetType ret;
    const FilePosType lastfilepos = lastFilePos();
    if ( size() < 1 || nrbytes < 1 || pos > lastfilepos )
	return ret;

    FilePosType lastpos = pos + nrbytes - 1;
    if ( lastpos > lastfilepos )
    {
	lastpos = lastfilepos;
	nrbytes = (ChunkSizeType)(lastpos - pos + 1);
    }

    const BlockIdxType lastbidx = blockIdx( lastpos );
    for ( BlockIdxType bidx=blockIdx(pos); bidx<=lastbidx; bidx++ )
    {
	if ( !blocks_[bidx] )
	{
	    const FilePosType blockstart = blockStart( bidx );
	    FilePosType blockend = blockstart + Block::cFullSize - 1;
	    if ( blockend > lastfilepos )
		blockend = lastfilepos;
	    ret += FileChunkType( blockstart, blockend );
	}
    }

    return ret;
}


// Note: we expect full blocks only, but still prepare for other input

bool Network::ReadCache::setData( FileChunkType chunk, const BufType* data )
{
    const BlockIdxType firstbidx = blockIdx( chunk.start );
    const BlockIdxType lastbidx = blockIdx( chunk.stop );
    FilePosType dataoffs = 0;

    for ( BlockIdxType bidx=firstbidx; bidx<=lastbidx; bidx++ )
    {
	Block* blk = gtBlk( bidx );
	if ( !blk ) // mem full, exit all
	    return false;

	const FilePosType blockstart = blockStart( bidx );
	const ChunkSizeType bufoffs = bidx != firstbidx ? 0
				: (ChunkSizeType)(chunk.start - blockstart);
	const ChunkSizeType nrbytes = bidx != lastbidx ? Block::cFullSize
				: (ChunkSizeType)(chunk.stop - blockstart + 1);
	OD::memCopy( blk->buf_+bufoffs, data+dataoffs, nrbytes );
	dataoffs += nrbytes;
    }

    return true;
}


//---- Write Cache
