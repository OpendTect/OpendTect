#pragma once

/*
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Bert
 Date:		Mar 2018
________________________________________________________________________

*/

#include "seisblocksreader.h"
#include "seisblockswriter.h"
#include "od_iosfwd.h"
#include <map>


namespace Seis
{
namespace Blocks
{

class FileIDTable : public std::map<HGlobIdx,od_int64> {};

class StreamReadBackEnd : public ReadBackEnd
{
public:

			StreamReadBackEnd(Reader&,const char*,uiRetVal&);
			StreamReadBackEnd(Reader&,od_istream&);
			~StreamReadBackEnd();

    virtual void	reset(const char*,uiRetVal&);
    virtual Column*	createColumn(const HGlobIdx&,uiRetVal&);
    virtual void	fillTrace(Column&,const BinID&,SeisTrc&,
				  uiRetVal&) const;

    void		openStream(const char*,uiRetVal&);
    void		closeStream();

protected:

    od_istream*		strm_;
    bool		strmmine_;

    friend class	FileColumn;

};


class HDF5ReadBackEnd : public ReadBackEnd
{
public:

			HDF5ReadBackEnd(Reader&,const char*,uiRetVal&);
			~HDF5ReadBackEnd();

    virtual void	reset(const char*,uiRetVal&);
    virtual Column*	createColumn(const HGlobIdx&,uiRetVal&);
    virtual void	fillTrace(Column&,const BinID&,SeisTrc&,
				  uiRetVal&) const;

};


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
