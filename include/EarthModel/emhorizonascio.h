#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Kristofer Tingdahl
 Date:		4-11-2002
________________________________________________________________________


-*/

#include "emcommon.h"
#include "tableascio.h"
#include "od_istream.h"


namespace EM
{

/*!\brief Ascii I/O for Horizon3D. */

mExpClass(EarthModel) Horizon3DAscIO : public Table::AscIO
{
public:
				Horizon3DAscIO( const Table::FormatDesc& fd,
						od_istream& strm )
				    : Table::AscIO(fd)
				    , udfval_(mUdf(float))
				    , finishedreadingheader_(false)
				    , strm_(strm)		    {}

    static Table::FormatDesc*   getDesc();
    static void			updateDesc(Table::FormatDesc&,
					   const BufferStringSet&);
    static void			createDescBody(Table::FormatDesc*,
					   const BufferStringSet&);

    bool			isXY() const;
    int				getNextLine(Coord&,TypeSet<float>&);

    static const char*		sKeyFormatStr();
    static const char*		sKeyAttribFormatStr();

protected:

    od_istream&			strm_;
    float			udfval_;
    bool			finishedreadingheader_;

};


/*!\brief Ascii I/O for Horizon2D. */

mExpClass(EarthModel) Horizon2DAscIO : public Table::AscIO
{ mODTextTranslationClass(Horizon2DAscIO)
public:
				Horizon2DAscIO( const Table::FormatDesc& fd,
						BufferString fnm )
				    : Table::AscIO(fd)
				    , udfval_(mUdf(float))
				    , finishedreadingheader_(false)
				    , strm_(fnm)
				{}

    static Table::FormatDesc*   getDesc();
    static void			updateDesc(Table::FormatDesc&,
					   const BufferStringSet&);
    static void			createDescBody(Table::FormatDesc*,
					   const BufferStringSet&);

    static bool			isFormatOK(const Table::FormatDesc&,
					   BufferString&);
    static bool			isFormatOK(const Table::FormatDesc&,uiString&);
    int				getNextLine(BufferString& lnm,Coord& crd,
					    int& trcnr,TypeSet<float>& data);
    bool			isTraceNr() const;
    bool			isOK()	{ return strm_.isOK(); }

protected:

    od_istream			strm_;
    float			udfval_;
    bool			finishedreadingheader_;

};

} // namespace EM
