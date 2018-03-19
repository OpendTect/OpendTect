#pragma once

/*
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Bert
 Date:		June 2017
________________________________________________________________________

*/

#include "seisblocksreader.h"
#include "seisblockswriter.h"
#include "od_iosfwd.h"


namespace Seis
{
namespace Blocks
{

class StreamWriteBackEnd : public WriteBackEnd
{
public:

			StreamWriteBackEnd(Writer&);
			~StreamWriteBackEnd();

    virtual void	setColumnInfo(const MemBlockColumn&,const HLocIdx&,
				      const HDimensions&,uiRetVal&);
    virtual void	putBlock(int,MemBlock&,HLocIdx,HDimensions,uiRetVal&);

    od_ostream*		strm_;

};

class HDF5WriteBackEnd : public WriteBackEnd
{
public:

			HDF5WriteBackEnd(Writer&);
			~HDF5WriteBackEnd();

    virtual void	setColumnInfo(const MemBlockColumn&,const HLocIdx&,
				      const HDimensions&,uiRetVal&);
    virtual void	putBlock(int,MemBlock&,HLocIdx,HDimensions,uiRetVal&);

};


} // namespace Blocks

} // namespace Seis
