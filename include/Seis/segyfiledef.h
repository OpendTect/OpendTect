#ifndef segyfiledef_h
#define segyfiledef_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Bert
 Date:		Sep 2008
 RCS:		$Id$
________________________________________________________________________

-*/

#include "seismod.h"
#include "ranges.h"
#include "coord.h"
#include "samplingdata.h"
#include "bufstringset.h"
#include "seistype.h"
#include "segythdef.h"
class IOObj;
class Scaler;


namespace SEGY
{


/*\brief Definition of input / output file(s)  */

mExpClass(Seis) FileSpec
{
public:

			FileSpec(const char* fnm=0);
			FileSpec(const IOPar&);

    BufferStringSet	fnames_;
    StepInterval<int>	nrs_;
    int			zeropad_;	//!< pad zeros to this length

    bool		isEmpty() const
			{ return fnames_.isEmpty() || fnames_.get(0).isEmpty();}
    bool		isMulti() const		{ return nrFiles() > 1; }
    bool		isRangeMulti() const;
    int			nrFiles() const	;
    const char*		fileName(int nr=0) const;
    const char*		dispName() const;	//!< for titles etc
    const char*		usrStr() const;		//!< the typed filename

    void		setEmpty()
			{ fnames_.setEmpty(); mSetUdf(nrs_.start); }
    void		setFileName( const char* nm )
			{ fnames_.setEmpty(); if ( nm && *nm ) fnames_.add(nm);}
    IOObj*		getIOObj(bool temporary,int nr=0) const;

    void		getMultiFromString(const char*);
    static const char*	sKeyFileNrs();

    static void		ensureWellDefined(IOObj&);
    static void		fillParFromIOObj(const IOObj&,IOPar&);

    void		fillPar(IOPar&) const;
    bool		usePar(const IOPar&);
    void		getReport(IOPar&,bool) const;

    static void		makePathsRelative(IOPar&,const char* todir=0);
			//< default is survey directory

};


/*\brief Parameters that control the primary read/write process */

mExpClass(Seis) FilePars
{
public:
			FilePars( bool forread=true )
			    : ns_(0)
			    , fmt_(forread?0:1)
			    , byteswap_(0)
			    , forread_(forread)		{}

    int			ns_;
    int			fmt_;
    int			byteswap_;	//!, 0=no 1=data only 2=all

    static int		nrFmts( bool forread )	{ return forread ? 6 : 5; }
    static const char**	getFmts(bool forread);
    static const char*	nameOfFmt(int fmt,bool forread);
    static int		fmtOf(const char*,bool forread);

    static const char*	sKeyForceRev0();
    static const char*	sKeyNrSamples();
    static const char*	sKeyNumberFormat();
    static const char*	sKeyByteSwap();

    void		setForRead(bool);

    void		fillPar(IOPar&) const;
    bool		usePar(const IOPar&);
    void		getReport(IOPar&,bool) const;

protected:

    bool		forread_;

};


/*\brief Options that control the actual read process */

mExpClass(Seis) FileReadOpts
{
public:
			FileReadOpts( Seis::GeomType gt=Seis::Vol )
			    : forread_(true)
			    , offsdef_(0,1)
			    , coordscale_(mUdf(float))
			    , timeshift_(mUdf(float))
			    , sampleintv_(mUdf(float))
			    , psdef_(InFile)
			    , icdef_(Both)
			    , coorddef_(Present)
			{ setGeomType(gt); thdef_.fromSettings(); }

    Seis::GeomType	geomType() const	{ return geom_; }
    void		setGeomType(Seis::GeomType);

    enum ICvsXYType	{ Both=0, ICOnly=1, XYOnly=2 };
    enum PSDefType	{ InFile=0, SrcRcvCoords=1, UsrDef=2 };
    enum CoordDefType	{ Present=0, ReadFile=1, Generate=2 };

    TrcHeaderDef	thdef_;
    ICvsXYType		icdef_;
    PSDefType		psdef_;
    CoordDefType	coorddef_;
    SamplingData<int>	offsdef_;
    float		coordscale_;
    float		timeshift_;
    float		sampleintv_;
    bool		forread_;
    Coord		startcoord_;
    Coord		stepcoord_;
    BufferString	coordfnm_;

    void		scaleCoord(Coord&,const Scaler* s=0) const;
    float		timeShift(float) const;
    float		sampleIntv(float) const;

    static const char*	sKeyICOpt();
    static const char*	sKeyPSOpt();
    static const char*	sKeyOffsDef();
    static const char*	sKeyCoordScale();
    static const char*	sKeyTimeShift();
    static const char*	sKeySampleIntv();
    static const char*	sKeyCoordOpt();
    static const char*	sKeyCoordStart();
    static const char*	sKeyCoordStep();
    static const char*	sKeyCoordFileName();

    void		fillPar(IOPar&) const;
    bool		usePar(const IOPar&);
    void		getReport(IOPar&,bool) const;
    static void		shallowClear(IOPar&);

protected:

    Seis::GeomType	geom_;

};

} // namespace


#endif
