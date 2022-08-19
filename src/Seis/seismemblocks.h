#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "seisblocks.h"
#include "databuf.h"
#include "od_iosfwd.h"


namespace Seis
{
namespace Blocks
{

class MemColumnSummary;

/*!\brief Block with data buffer collecting data to be written. */

class MemBlock : public Block
{
public:

			MemBlock(GlobIdx,const Dimensions&,const DataInterp&);

    void		zero()			{ dbuf_.zero(); }
    void		retire(MemColumnSummary*,const bool* const*);
    bool		isRetired() const	{ return dbuf_.isEmpty(); }

    float		value(const LocIdx&) const;
    void		setValue(const LocIdx&,float);

    DataBuffer		dbuf_;
    const DataInterp&	interp_;

protected:

    int			getBufIdx(const LocIdx&) const;

};


/*!\brief Summary of a column for use in maps. */


class MemColumnSummary
{
public:

		MemColumnSummary()	: vals_(0)	    {}
		~MemColumnSummary();

    void	fill(const MemBlock&,const bool* const*);

    bool	isEmpty() const				    { return !vals_; }
    float	calcVal(float*) const;

    float**	vals_;
    Dimensions	dims_;

};


class MemBlockColumn : public Column
{
public:

    typedef ManagedObjectSet<MemBlock>	BlockSet;

			MemBlockColumn(const HGlobIdx&,const Dimensions&,
					int ncomps);
			~MemBlockColumn();

    void		retire();
    inline bool		isRetired() const
			{ return blocksets_.first()->first()->isRetired(); }
    void		getDefArea(HLocIdx&,HDimensions&) const;

    int			nruniquevisits_;
    od_stream_Pos	fileoffset_;
    bool**		visited_;
    ObjectSet<BlockSet> blocksets_; // one set per component
    MemColumnSummary	summary_;

};


} // namespace Blocks

} // namespace Seis
