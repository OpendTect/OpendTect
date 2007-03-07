#ifndef flatview_h
#define flatview_h
/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        A.H. Bril
 Date:          Dec 2005
 RCS:           $Id: flatview.h,v 1.7 2007-03-07 10:36:07 cvsbert Exp $
________________________________________________________________________

-*/

#include "flatposdata.h"
#include "bufstring.h"
#include "geometry.h"
#include "position.h"
#include "datapack.h"
#include "draw.h"

class IOPar;
class FlatDataPack;
template <class T> class Array2D;


namespace FlatView
{

typedef Geom::Point2D<double> Point;
typedef Geom::PosRectangle<double> Rect;


/*!\brief Annotation data for flat views */

class Annotation
{
public:

    //!\brief Things like well tracks, cultural data, 2-D line positions
    class AuxData
    {
    public:
	struct PolyPt
	{
	    		PolyPt( const Point& p, int nr=mUdf(int) )
			    : p_(p), nr_(nr)		{}
	    inline bool	operator ==( const PolyPt& pt ) const
			{ return p_ == pt.p_; }
	    Point	p_;
	    int		nr_;
	};

			AuxData( const char* nm )
			    : name_(nm)
			    , namepos_(mUdf(int))
			    , linecolor_(Color::NoColor)
			    , fillcolor_(Color::NoColor)
			    , markerstyle_(MarkerStyle2D::None)
			    , close_(false)		{}

	BufferString	name_;
	int		namepos_;	//!< nodraw=udf, before first=-1,
					//!< center=0, after last=1
	Color		linecolor_;
	Color		fillcolor_;
	MarkerStyle2D	markerstyle_;

	TypeSet<PolyPt>	poly_;
	bool		close_;

	inline bool	isEmpty() const		{ return poly_.isEmpty(); }
    };

    struct AxisData
    {
			AxisData() : reversed_(false)	{ showAll(false); }

	BufferString	name_;
	bool		showannot_;
	bool		showgridlines_;
	bool		reversed_;

	inline void	showAll( bool yn ) { showannot_ = showgridlines_ = yn; }
    };


			Annotation(bool darkbg);
			~Annotation()		{ deepErase(auxdata_); }

    BufferString	title_; //!< color not settable
    Color		color_; //!< For axes
    AxisData		x1_;
    AxisData		x2_;
    ObjectSet<AuxData>	auxdata_;
    bool		showaux_;

    inline void		setAxesAnnot( bool yn ) //!< Convenience all or nothing
			{ x1_.showAll(yn); x2_.showAll(yn); }
    inline bool		haveTitle() const
    			{ return !title_.isEmpty(); }
    inline bool		haveAxisAnnot( bool x1dir ) const
			{ return color_ != Color::NoColor
			      && ( ( x1dir && x1_.showannot_)
				|| (!x1dir && x2_.showannot_)); }
    inline bool		haveGridLines( bool x1dir ) const
			{ return color_ != Color::NoColor
			      && ( ( x1dir && x1_.showgridlines_)
				|| (!x1dir && x2_.showgridlines_)); }
    inline bool		haveAux() const
			{ return showaux_ && auxdata_.size() > 0; }

    void		fillPar(IOPar&) const;
    void		usePar(const IOPar&);

    static const char*	sKeyAxes;
    static const char*	sKeyShwAnnot;
    static const char*	sKeyShwGridLines;
    static const char*	sKeyIsRev;
    static const char*	sKeyShwAux;

};


/*!\brief Data display paramters

  When data needs to be displayed, there is a load of parameters and
  options for display. The two main display modes are:
  Variable Density = display according to color table
  Wiggle/Variable Area = wiggles with (possibly) filled amplitude

  */

class DataDispPars
{
public:

    //!\brief Common to VD and WVA
    class Common
    {
    public:
			Common();

	bool		show_;	   // default=true
	Interval<float>	rg_;	   // default=mUdf(float)
	float		clipperc_; // default from ColorTable
	bool		blocky_;   // default=false

    };

    //!\brief Variable Density (=color-bar driven) parameters
    class VD : public Common
    {
    public:

			VD()			{}

	BufferString	ctab_;

    };
    //!\brief Wiggle/Variable Area parameters
    class WVA : public Common
    {
    public:

		    WVA()
			: wigg_(Color::Black)
			, mid_(Color::NoColor)
			, left_(Color::NoColor)
			, right_(Color::DgbColor)
			, midvalue_(0)
			, overlap_(1)		{}

	Color		wigg_;
	Color		mid_;
	Color		left_;
	Color		right_;
	float		overlap_;
	float		midvalue_; //!< undef => auto data mid

    };

    			DataDispPars()		{}

    VD			vd_;
    WVA			wva_;
    void		show( bool wva, bool vd )
    			{ wva_.show_ = wva; vd_.show_ = vd; }

    void		fillPar(IOPar&) const;
    void		usePar(const IOPar&);

    static const char*	sKeyVD;
    static const char*	sKeyWVA;
    static const char*	sKeyShow;
    static const char*	sKeyDispRg;
    static const char*	sKeyColTab;
    static const char*	sKeyBlocky;
    static const char*	sKeyClipPerc;
    static const char*	sKeyWiggCol;
    static const char*	sKeyMidCol;
    static const char*	sKeyLeftCol;
    static const char*	sKeyRightCol;
    static const char*	sKeyOverlap;
    static const char*	sKeyMidValue;
};


/*!\brief Flat views: Context - holds all but the data itself.
  Subclassing can be necessary to provide info on the data via the getAuxInfo.
 */

class Context
{
public:
    			Context( bool drkbg=true )
			    : darkbg_(drkbg)
			    , annot_(drkbg)	{}

    virtual void	fillPar(IOPar&) const;
    virtual void	usePar(const IOPar&);

    FlatPosData		wvaposdata_;
    FlatPosData		vdposdata_;
    Annotation		annot_;
    DataDispPars	ddpars_;

    void		setDarkBG(bool yn);
    bool		darkBG() const		{ return darkbg_; }

    void		setGeoDefaults( bool vert )
			{ if ( vert ) annot_.x2_.reversed_ = true; }

protected:

    bool		darkbg_;	//!< Two styles: dark (=black)
					//!< and lite (=white) background
    					//!< Impacts a lot of display choices

};


/*!\brief Data used + a name for each. The Array2D's are not managed. */

class Data
{
public:
			Data()
			    : vdarr_(0)
			    , wvaarr_(0)		{}

    virtual const Array2D<float>* arr( bool wva ) const
			{ return wva ? wvaarr_ : vdarr_; }
    virtual const char*	name( bool wva ) const
			{ return wva ? wvaname_.buf() : vdname_.buf(); }
    virtual bool	set(bool iswva,const Array2D<float>*,const char*);
    			//!< returns whether anything changed

    // convenience
    inline bool		isEmpty() const
			{ return !wvaarr() && !vdarr(); }
    inline bool		isDouble() const
			{ return wvaarr() && vdarr(); }
    inline bool		equal() const
			{ return wvaarr() == vdarr(); }
    inline int		nrArrs() const
			{ return wvaarr() ? (vdarr()?2:1) : (vdarr()?1:0); }

    inline const Array2D<float>* wvaarr() const { return arr(true); }
    inline const Array2D<float>* vdarr() const	{ return arr(false); }
    virtual const char*	wvaname() const		{ return name(true); }
    virtual const char*	vdname() const		{ return name(false); }
				
    virtual void	addAuxInfo(bool wva,int,int,IOPar&) const	{}
    			//!< Give extra info like stuff from trace headers
    			//!< should be 'printable'

protected:

    BufferString		vdname_;
    BufferString		wvaname_;
    const Array2D<float>*	vdarr_;
    const Array2D<float>*	wvaarr_;

};



/*!\brief Flat Viewer of 2-D 'arrays' of data and/or context

  Interface for displaying data and related annotations where at least one of
  the directions is sampled regularly.

  */

class Viewer
{
public:

    			Viewer() : defdata_(0), defctxt_(0)	{}
    virtual		~Viewer()				{}

    virtual Context&	context();
    const Context&	context() const
    			{ return const_cast<Viewer*>(this)->context(); }
    virtual Data&	data();
    const Data&		data() const
    			{ return const_cast<Viewer*>(this)->data(); }

    virtual bool	isVertical() const		{ return true; }

    enum DataChangeType	{ None, All, Annot, WVAData, VDData, WVAPars, VDPars };
    virtual void	handleChange(DataChangeType)	= 0;

    virtual void	fillPar( IOPar& iop ) const
			{ context().fillPar( iop ); }
    virtual void	usePar( const IOPar& iop )
			{ context().usePar( iop ); }

    void		storeDefaults(const char* key) const;
    void		useStoredDefaults(const char* key);

    virtual void	setPack(bool,::DataPack::ID);
    			//!< Optional. No data needs to be in a Pack
    			//!< The pack will be obtained non-observing.
    virtual void	setPack(bool,const FlatDataPack*);
    			//!< You can pass null.
    			//!< The pack must have been obtained non-observing.
    virtual const FlatDataPack*	getPack(bool wva) const;
    			//!< May return null
    void		syncDataPacks(); //!< after mix/null of data().arr's
    void		getAuxInfo(const Point&,IOPar&) const;

protected:

    Data*		defdata_;
    Context*		defctxt_;

    void		addAuxInfo(bool,const Point&,IOPar&) const;

};


} // namespace FlatView

#endif
