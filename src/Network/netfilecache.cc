/*+
________________________________________________________________________

(C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
Author:     Salil Agarwal
Date:	    October 2012
________________________________________________________________________

-*/

#include "netfilecache.h"

//! read cache size unless more needed by a caller
static const Network::ReadCache::block_idx_type
				cReadInitialNrLiveBlocks = 10;
//! write cache buffer so write ptr can be moved back
static const Network::WriteCache::block_idx_type
				cWriteInitialNrBlocksMemory = 10;

const Network::FileCache::BlockSizeType
	Network::FileCache::Block::cFullSize = 2097152; // 2 MB
Network::FileCache::FilePosType
Network::FileCache::blockStart( block_idx_type bidx )
{ return bidx < 0 ? 0 : ((FilePosType)bidx) * Block::cFullSize; }


Network::FileCache::Block::Block( SizeType sz )
    : bufsz_(sz)
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


Network::FileCache::FileCache( file_size_type knownfilesz )
    : knownfilesize_(knownfilesz)
    , lastblocksz_(Block::cFullSize)
{
    blocks_.setNullAllowed( true );
    if ( knownfilesz < 1 )
	return;

    block_idx_type nrblocks = (block_idx_type)(knownfilesize_
					     / Block::cFullSize);
    for ( block_idx_type iblk=0; iblk<nrblocks; iblk++ )
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


Network::FileCache::file_size_type Network::FileCache::size() const
{
    if ( knownfilesize_ > 0 )
	return knownfilesize_;

    const block_idx_type nrblocks = blocks_.size();
    if ( nrblocks == 0 )
	return 0;

    file_size_type ret = nrblocks - 1;
    if ( ret > 0 )
	ret *= Block::cFullSize;
    ret += lastblocksz_;
    return ret;
}


void Network::FileCache::dismissBlock( block_idx_type iblk )
{
    if ( blocks_.validIdx(iblk) )
    {
	delete blocks_[iblk];
	blocks_.replace( iblk, 0 );
    }
}


void Network::FileCache::clearBlocks()
{
    for ( block_idx_type iblk=0; iblk<blocks_.size(); iblk++ )
	dismissBlock( iblk );
}


Network::FileCache::Block* Network::FileCache::gtBlk(
					block_idx_type iblk ) const
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
						block_idx_type iblk ) const
{
    return iblk == blocks_.size() - 1 ? lastblocksz_ : Block::cFullSize;
}


Network::FileCache::block_idx_type Network::FileCache::blockIdx(
						FilePosType pos ) const
{
    return (block_idx_type)(pos / Block::cFullSize);
}


bool Network::FileCache::isLiveBlock( block_idx_type bidx ) const
{
    return blocks_.validIdx(bidx) && blocks_[bidx];
}


Network::FileCache::BufType* Network::FileCache::getBlock( block_idx_type bidx )
{
    Block* ret = blocks_.validIdx(bidx) ? gtBlk( bidx ) : 0;
    return ret ? ret->buf_ : 0;
}


const Network::FileCache::BufType* Network::FileCache::getBlock(
						block_idx_type bidx ) const
{
    const Block* ret = blocks_.validIdx(bidx) ? gtBlk( bidx ) : 0;
    return ret ? ret->buf_ : 0;
}


//---- Read Cache


Network::ReadCache::ReadCache( file_size_type knownfilesz )
    : FileCache(knownfilesz)
    , maxnrliveblocks_(cReadInitialNrLiveBlocks)
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


void Network::ReadCache::setMinCacheSize( file_size_type csz )
{
    block_idx_type reqnrblks = 2; // always keep first and last
    reqnrblks += (block_idx_type)(csz / Block::cFullSize) + 2;
    if ( reqnrblks > maxnrliveblocks_ )
	maxnrliveblocks_ = reqnrblks;
}


void Network::ReadCache::handleNewLiveBlock( block_idx_type addedblockidx )
{
    const block_idx_type nrbidxs = liveblockidxs_.size();
    if ( nrbidxs >= maxnrliveblocks_ )
    {
	for ( block_idx_type iblk=0; iblk<nrbidxs; iblk++ )
	{
	    const block_idx_type curbidx = liveblockidxs_[iblk];
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
				      file_size_type nrbytes ) const
{
    if ( nrbytes < 1 )
	return pos < size();

    const FilePosType lastpos = pos + nrbytes - 1;
    const block_idx_type lastbidx = blockIdx( lastpos );
    for ( block_idx_type bidx=blockIdx(pos); bidx<=lastbidx; bidx++ )
    {
	if ( !blocks_[bidx] )
	    return false;
    }
    return true;
}


Network::ReadCache::file_size_type Network::ReadCache::getAt( FilePosType pos,
		BufType* out, file_size_type totalnrbytes ) const
{
    if ( !out || totalnrbytes < 1 )
	return 0;
    else if ( !isAvailable(pos,totalnrbytes) )
    {
	pErrMsg( "Not all data requested previously filled" );
	OD::memZero( out, totalnrbytes );
    }

    file_size_type byteshandled = 0;
    const FilePosType lastpos = pos + totalnrbytes - 1;
    const block_idx_type lastbidx = blockIdx( lastpos );
    for ( block_idx_type bidx=blockIdx(pos); bidx<=lastbidx; bidx++ )
    {
	const Block* blk = blocks_[bidx];
	file_size_type nrbytes = Block::cFullSize;
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
    if ( nrbytes < 1 )
	return ret;

    FilePosType lastfilepos = lastFilePos();
    FilePosType lastpos = pos + nrbytes - 1;
    if ( lastfilepos < 1 )
	lastfilepos = lastpos;
    else if ( size() < 1 || pos > lastfilepos )
	return ret;

    if ( lastpos > lastfilepos )
    {
	lastpos = lastfilepos;
	nrbytes = (ChunkSizeType)(lastpos - pos + 1);
    }

    const block_idx_type lastbidx = blockIdx( lastpos );
    for ( block_idx_type bidx=blockIdx(pos); bidx<=lastbidx; bidx++ )
    {
	if ( !isLiveBlock(bidx) )
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
    const block_idx_type firstbidx = blockIdx( chunk.start );
    const block_idx_type lastbidx = blockIdx( chunk.stop );
    FilePosType dataoffs = 0;

    for ( block_idx_type bidx=firstbidx; bidx<=lastbidx; bidx++ )
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


Network::WriteCache::WriteCache()
    : FileCache(0)
    , nrblocksmem_(cWriteInitialNrBlocksMemory)
{
}


Network::WriteCache::~WriteCache()
{
}


void Network::WriteCache::clearData()
{
    clearBlocks();
}


void Network::WriteCache::handleNewLiveBlock( block_idx_type addedblockidx )
{
    if ( addedblockidx > nrblocksmem_ + 1 )
	dismissBlock( addedblockidx - nrblocksmem_ - 1 );
}


void Network::WriteCache::setMinCacheSize( file_size_type csz )
{
    block_idx_type reqnrblks = 1; // always keep first and last
    reqnrblks += (block_idx_type)(csz / Block::cFullSize) + 2;
    if ( reqnrblks > nrblocksmem_ )
	nrblocksmem_ = reqnrblks;
}
