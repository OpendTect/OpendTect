/*+
________________________________________________________________________

(C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
Author:	    Salil Agarwal
Date:	    October 2012
________________________________________________________________________

-*/

#include "netfilecache.h"

const Network::FileCache::Block::SizeType
	Network::FileCache::Block::cFullSize = 2097152; // 2 MB
inline Network::FileCache::FilePosType
Network::FileCache::blockStart( BlockIdxType bidx )
{ return ((FilePosType)bidx) * Block::cFullSize; }


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


Network::FileCache::FileCache( FileSizeType filesz )
    : filesize_(filesz)
    , lastblocksz_(0)
    , lastblockpos_(0)
{
    blocks_.allowNull( true );
    if ( filesz < 1 )
	return;

    BlockIdxType nrblocks = (BlockIdxType)(filesize_ / Block::cFullSize);
    const_cast<FilePosType&>(lastblockpos_) = blockStart( nrblocks );
    Block::SizeType lastblksz = (Block::SizeType)(filesize_ - lastblockpos_);
    if ( lastblksz > 0 )
        nrblocks++;
    else
	lastblksz = Block::cFullSize;
    const_cast<Block::SizeType&>(lastblocksz_) = lastblksz;

    for ( BlockIdxType iblk=0; iblk<nrblocks; iblk++ )
	blocks_ += 0;
}


Network::FileCache::~FileCache()
{
    deepErase( blocks_ );
}


void Network::FileCache::clearData()
{
    for ( BlockIdxType iblk=0; iblk<blocks_.size(); iblk++ )
	dismissBlock( iblk );
}


Network::FileCache::Block* Network::FileCache::getBlock(
					BlockIdxType iblk ) const
{
    if ( !blocks_[iblk] )
    {
	FileCache& self = *const_cast<FileCache*>( this );
	Block* newblock = new Block( blockSize(iblk) );
	if ( !newblock->buf_ ) // emergency: mem full
	    { self.clearData(); return 0; }

	self.blocks_.replace( iblk, newblock );
	self.newBlockAdded();
    }
    return const_cast<Block*>( blocks_[iblk] );
}


void Network::FileCache::dismissBlock( BlockIdxType iblk )
{
    if ( blocks_[iblk] )
    {
	delete blocks_[iblk];
	blocks_.replace( iblk, 0 );
    }
}


void Network::FileCache::newBlockAdded()
{
    //TODO maybe dismiss oldest block. But keep special blocks (first, last)
}


Network::FileCache::Block::SizeType Network::FileCache::blockSize(
						BlockIdxType iblk ) const
{
    return iblk == blocks_.size() - 1 ? lastblocksz_ : Block::cFullSize;
}


Network::FileCache::BlockIdxType Network::FileCache::blockIdx(
						FilePosType pos ) const
{
    return pos < lastblockpos_ ? (BlockIdxType)(pos / Block::cFullSize)
			       : blocks_.size() - 1;
}


bool Network::FileCache::isAvailable( FilePosType pos,
				      ChunkSizeType nrbytes ) const
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


void Network::FileCache::getAt( FilePosType pos, BufType* out,
				ChunkSizeType totalnrbytes ) const
{
    if ( !out || totalnrbytes < 1 )
	return;
    else if ( !isAvailable(pos,totalnrbytes) )
    {
	pErrMsg( "Not all data requested previously filled" );
	OD::memZero( out, totalnrbytes );
    }

    FilePosType outoffs = 0;
    const FilePosType lastpos = pos + totalnrbytes - 1;
    const BlockIdxType lastbidx = blockIdx( lastpos );
    for ( BlockIdxType bidx=blockIdx(pos); bidx<=lastbidx; bidx++ )
    {
	const Block* blk = blocks_[bidx];
	if ( blk )
	{
	    const FilePosType blockstart = blockStart( bidx );
	    Interval<ChunkSizeType> wantedrg( (ChunkSizeType)(pos-blockstart),
					  (ChunkSizeType)(lastpos-blockstart) );
	    if ( wantedrg.start < 0 )
		wantedrg.start = 0;
	    if ( wantedrg.stop > blk->bufsz_-1 )
		wantedrg.stop = blk->bufsz_-1;

	    const ChunkSizeType nrbytes = wantedrg.stop - wantedrg.start + 1;
	    OD::memCopy( out+outoffs, blk->buf_+wantedrg.start, nrbytes );
	}
	outoffs += Block::cFullSize;
    }
}


Network::FileCache::FileChunkSetType Network::FileCache::neededFill(
		FilePosType pos, ChunkSizeType nrbytes ) const
{
    if ( nrbytes < 1 )
	nrbytes = 1;

    FileChunkSetType ret;
    const FilePosType lastpos = pos + nrbytes - 1;
    const BlockIdxType lastbidx = blockIdx( lastpos );
    for ( BlockIdxType bidx=blockIdx(pos); bidx<=lastbidx; bidx++ )
    {
	if ( !blocks_[bidx] )
	{
	    const FilePosType blockstart = blockStart( bidx );
	    ret += FileChunkType( blockstart, blockstart+Block::cFullSize-1 );
	}
    }

    return ret;
}


// Note: we expect full blocks only, but still prepare for other input

bool Network::FileCache::fill( FileChunkType chunk, const BufType* data )
{
    const BlockIdxType firstbidx = blockIdx( chunk.start );
    const BlockIdxType lastbidx = blockIdx( chunk.stop );
    FilePosType dataoffs = 0;

    for ( BlockIdxType bidx=firstbidx; bidx<=lastbidx; bidx++ )
    {
	Block* blk = getBlock( bidx );
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
