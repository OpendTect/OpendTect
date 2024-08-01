#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "generalmod.h"

#include "executor.h"
#include "tableascio.h"


namespace PosInfo { class Line2DData; }
namespace Survey { class Geometry2D; }
namespace Table { class FormatDesc; }

/*!
\brief Ascii I/O for 2D Geometry.
*/

mExpClass(General) Geom2dAscIO : public Table::AscIO
{ mODTextTranslationClass(Geom2dAscIO)
public:
				Geom2dAscIO(const Table::FormatDesc&,
					    od_istream&);

    static Table::FormatDesc*	getDesc(bool withline=false);
    static void			fillDesc(Table::FormatDesc&,bool withline);
    bool			getData(PosInfo::Line2DData&);
    bool			getData(Survey::Geometry2D&) const;
    bool			getData(ObjectSet<Survey::Geometry2D>&) const;

protected:

    od_istream&			strm_;
    bool			readLine(int startidx,Coord&,int& trcnr,
					 float& spnr,bool isxy,bool conv) const;
};



mExpClass(General) SEGP1Header : public BufferStringSet
{
public:
			SEGP1Header();
			~SEGP1Header();
};


mExpClass(General) SEGP1Entry
{
public:
			SEGP1Entry();
			~SEGP1Entry();

    void		setLineName(const char*);

    SEGP1Header		header_;
    Survey::Geometry2D*	geom_				= nullptr;
};


mExpClass(General) SEGP1Importer : public Executor
{ mODTextTranslationClass(SEGP1Importer)
public:
			SEGP1Importer(const char* fnm);
			~SEGP1Importer();

    void		setOrigin(const Coord&);
    void		setUseLatLong(bool);

    od_int64		nrDone() const override		{ return nrdone_; }
    od_int64		totalNr() const override	{ return totalnr_; }
    uiString		uiMessage() const override	{ return msg_; }
    uiString		uiNrDoneText() const override	{ return nrdonetxt_; }

    const ObjectSet<SEGP1Entry>& entries() const	{ return entries_; }

private:
    bool			readHeader();
    bool			readRecord(const BufferString&);
    int				nextStep() override;

    od_int64			nrdone_				= 0;
    od_int64			totalnr_			= 0;
    uiString			msg_;
    uiString			nrdonetxt_;

    od_istream*			strm_;
    ObjectSet<SEGP1Entry>	entries_;
    Coord			origin_				= Coord(0,0);
    bool			uselatlong_			= false;
};
