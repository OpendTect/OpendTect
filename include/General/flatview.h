#ifndef flatview_h
#define flatview_h
/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        A.H. Bril
 Date:          Dec 2005
 RCS:           $Id: flatview.h,v 1.21 2007-06-25 21:41:55 cvskris Exp $
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
	//!\brief explains what part of the an auxdata's appearance that may be
	//!	  edited by the user
	class EditPermissions
	{
	public:			EditPermissions();
	    bool		onoff_;
	    bool		namepos_;
	    bool		linestyle_;
	    bool		linecolor_;
	    bool		fillcolor_;
	    bool		markerstyle_;
	    bool		markercolor_;
	    bool		x1rg_;
	    bool		x2rg_;
	};
				AuxData( const char* nm );
				AuxData( const AuxData& );
				~AuxData();

	EditPermissions*	editpermissions_;//!<If null no editing allowed

	bool			enabled_; 	//!<Turns on/off everything
	BufferString		name_;
	int			namepos_;	//!<nodraw=udf, before first=-1,
						//!< center=0, after last=1
	LineStyle		linestyle_;
	Color			fillcolor_;
	MarkerStyle2D		markerstyle_;

	Interval<double>*	x1rg_;		//!<if 0, use viewer's rg & zoom
	Interval<double>*	x2rg_;		//!<if 0, use viewer's rg & zoom

	TypeSet<Point>		poly_;
	bool			close_;

	bool			isEmpty() const;
	void			empty();
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
				~Annotation();

    BufferString		title_; //!< color not settable
    Color			color_; //!< For axes
    AxisData			x1_;
    AxisData			x2_;
    ObjectSet<AuxData>		auxdata_;
    bool			showaux_;

    inline void		setAxesAnnot( bool yn ) //!< Convenience all or nothing
			{ x1_.showAll(yn); x2_.showAll(yn); }
    inline bool		haveTitle() const
    			{ return !title_.isEmpty(); }
    inline bool		haveAxisAnnot( bool x1dir ) const
			{ return color_.isVisible()
			      && ( ( x1dir && x1_.showannot_)
				|| (!x1dir && x2_.showannot_)); }
    inline bool		haveGridLines( bool x1dir ) const
			{ return color_.isVisible()
			      && ( ( x1dir && x1_.showgridlines_)
				|| (!x1dir && x2_.showgridlines_)); }
    bool		haveAux() const;

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


/*!\brief Flat views: Appearance  */

class Appearance
{
public:
    			Appearance( bool drkbg=true )
			    : darkbg_(drkbg)
			    , annot_(drkbg)	{}

    virtual void	fillPar(IOPar&) const;
    virtual void	usePar(const IOPar&);

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


/*!\brief Data for a display: positions, arrays and name. */

class PackData
{
public:
    				PackData( const Array2D<float>* a=0,
					  const char* nm=0 )
				    : arr_(a), name_(nm)	{}

    FlatPosData			pos_;
    const Array2D<float>*	arr_;
    BufferString		name_;

    inline bool			isEmpty() const
				{ return !arr_; }
    inline void			setEmpty()
				{ arr_ = 0; }
    inline void			clear()
				{ arr_ = 0; name_ = ""; pos_.setRegular(); }
};


/*!\brief PackData for both WVA and VD. */

class Data
{
public:

    PackData		wva_;
    PackData		vd_;

    virtual void	addAuxInfo(bool wva,int,int,IOPar&) const	{}
    			//!< Give extra info like stuff from trace headers
    			//!< should be 'printable'

    // convenience
    inline bool		isEmpty() const
			{ return !wvaArr() && !vdArr(); }
    inline int		nrArrs() const
			{ return wvaArr() ? (vdArr()?2:1) : (vdArr()?1:0); }
    inline void		setEmpty( bool wva )
			{ wva ? wva_.setEmpty() : vd_.setEmpty(); }

    inline const FlatPosData&	pos( bool wva ) const
    			{ return wva ? wva_.pos_ : vd_.pos_; }
    inline const Array2D<float>* arr( bool wva ) const
    			{ return wva ? wva_.arr_ : vd_.arr_; }
    inline const BufferString&	name( bool wva ) const
    			{ return wva ? wva_.name_ : vd_.name_; }

    inline const FlatPosData&	wvaPos() const		{ return pos(true); }
    inline const FlatPosData&	vdPos() const		{ return pos(false); }
    inline const Array2D<float>* wvaArr() const		{ return arr(true); }
    inline const Array2D<float>* vdArr() const		{ return arr(false); }
    inline const BufferString&	wvaName() const		{ return name(true); }
    inline const BufferString&	vdName() const		{ return name(false); }

};



/*!\brief Flat Viewer using FlatView::Data and FlatView::Appearance

  Interface for displaying data and related annotations where at least one of
  the directions is sampled regularly.

  */

class Viewer
{
public:

    			Viewer() : defdata_(0), defapp_(0)	{}
    virtual		~Viewer()				{}

    virtual Appearance&	appearance();
    const Appearance&	appearance() const
    			{ return const_cast<Viewer*>(this)->appearance(); }
    virtual Data&	data();
    const Data&		data() const
    			{ return const_cast<Viewer*>(this)->data(); }

    virtual bool	isVertical() const		{ return true; }
    bool		isVisible(bool wva) const;
    			//!< Depends on show_ and availability of data

    enum DataChangeType	{ None, All, Annot, WVAData, VDData, WVAPars, VDPars };
    virtual void	handleChange(DataChangeType)	= 0;

    virtual void	fillPar( IOPar& iop ) const
			{ appearance().fillPar( iop ); }
    virtual void	usePar( const IOPar& iop )
			{ appearance().usePar( iop ); }

    void		storeDefaults(const char* key) const;
    void		useStoredDefaults(const char* key);

    void		setPack(bool vwa,::DataPack::ID,bool usedefs=true);
    			//!< Optional. No data needs to be in a Pack
    			//!< The pack will be obtained non-observing.
    void		setPack(bool vwa,const FlatDataPack*,bool usedefs=true);
    			//!< You can pass null.
    			//!< The pack must have been obtained non-observing.
    virtual const FlatDataPack*	getPack(bool wva) const;
    			//!< May return null

    void		syncDataPacks(); //!< after mix/null of data()
    void		getAuxInfo(const Point&,IOPar&) const;

protected:

    Data*		defdata_;
    Appearance*		defapp_;

    void		addAuxInfo(bool,const Point&,IOPar&) const;
    void		chgPack(bool,const FlatDataPack*);
    virtual void	doSetPack(bool,const FlatDataPack*,bool);

};


} // namespace FlatView

#endif
