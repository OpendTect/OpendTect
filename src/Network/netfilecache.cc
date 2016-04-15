/*+
________________________________________________________________________

(C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
Author:	    Salil Agarwal
Date:	    October 2012
________________________________________________________________________

-*/

#include "netfilecache.h"

//! cache size unless more explicitly required
static const Network::ReadCache::BlockIdxType cStartMaxLiveBlocks = 10;

const Network::ReadCache::BlockSizeType
	Network::ReadCache::Block::cFullSize = 2097152; // 2 MB
Network::ReadCache::FilePosType
Network::ReadCache::blockStart( BlockIdxType bidx )
{ return ((FilePosType)bidx) * Block::cFullSize; }


Network::ReadCache::Block::Block( SizeType sz )
    : buf_(0)
    , bufsz_(sz)
{
    try {
	buf_ = new BufType[ bufsz_ ];
    } catch ( std::bad_alloc )
	{ buf_ = 0; }
}


Network::ReadCache::Block::~Block()
{
    delete [] buf_;
}


Network::ReadCache::ReadCache( FileSizeType filesz )
    : filesize_(filesz)
    , maxnrliveblocks_(cStartMaxLiveBlocks)
    , lastblocksz_(0)
    , lastblockpos_(0)
{
    blocks_.allowNull( true );
    if ( filesz < 1 )
	return;

    BlockIdxType nrblocks = (BlockIdxType)(filesize_ / Block::cFullSize);
    const_cast<FilePosType&>(lastblockpos_) = blockStart( nrblocks );
    BlockSizeType lastblksz = (BlockSizeType)(filesize_ - lastblockpos_);
    if ( lastblksz > 0 )
        nrblocks++;
    else
	lastblksz = Block::cFullSize;
    const_cast<BlockSizeType&>(lastblocksz_) = lastblksz;

    for ( BlockIdxType iblk=0; iblk<nrblocks; iblk++ )
	blocks_ += 0;
}


Network::ReadCache::~ReadCache()
{
    deepErase( blocks_ );
}


void Network::ReadCache::dismissBlock( BlockIdxType iblk )
{
    delete blocks_[iblk];
    blocks_.replace( iblk, 0 );
}


void Network::ReadCache::clearData()
{
    for ( BlockIdxType iblk=0; iblk<blocks_.size(); iblk++ )
	dismissBlock( iblk );
    liveblockidxs_.setEmpty();
}


void Network::ReadCache::setMinCacheSize( FileSizeType csz )
{
    BlockIdxType reqnrblks = 2; // always keep first and last
    reqnrblks += (BlockIdxType)(csz / Block::cFullSize) + 2;
    if ( reqnrblks > maxnrliveblocks_ )
	maxnrliveblocks_ = reqnrblks;
}


Network::ReadCache::Block* Network::ReadCache::gtBlk( BlockIdxType iblk ) const
{
    if ( !blocks_[iblk] )
    {
	ReadCache& self = *const_cast<ReadCache*>( this );
	Block* newblock = new Block( blockSize(iblk) );
	if ( !newblock->buf_ ) // emergency: mem full
	    { self.clearData(); return 0; }

	self.blocks_.replace( iblk, newblock );
	self.handleNewLiveBlock( iblk );
    }
    return const_cast<Block*>( blocks_[iblk] );
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


Network::ReadCache::BlockSizeType Network::ReadCache::blockSize(
						BlockIdxType iblk ) const
{
    return iblk == blocks_.size() - 1 ? lastblocksz_ : Block::cFullSize;
}


Network::ReadCache::BlockIdxType Network::ReadCache::blockIdx(
						FilePosType pos ) const
{
    return pos < lastblockpos_ ? (BlockIdxType)(pos / Block::cFullSize)
			       : blocks_.size() - 1;
}


bool Network::ReadCache::isAvailable( FilePosType pos,
				      FileSizeType nrbytes ) const
{
    if ( nrbytes < 1 )
	return pos < filesize_;

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
    if ( filesize_ < 1 || nrbytes < 1 || pos > lastfilepos )
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


bool Network::ReadCache::isLiveBlock( BlockIdxType bidx ) const
{
    return blocks_.validIdx(bidx) && blocks_[bidx];
}


Network::ReadCache::BufType* Network::ReadCache::getBlock( BlockIdxType bidx )
{
    Block* ret = blocks_.validIdx(bidx) ? gtBlk( bidx ) : 0;
    return ret ? ret->buf_ : 0;
}


const Network::ReadCache::BufType* Network::ReadCache::getBlock(
						BlockIdxType bidx ) const
{
    const Block* ret = blocks_.validIdx(bidx) ? gtBlk( bidx ) : 0;
    return ret ? ret->buf_ : 0;
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
