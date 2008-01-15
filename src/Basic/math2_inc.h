/*+
 * COPYRIGHT: (C) dGB Beheer B.V.
 * AUTHOR   : A.H. Bril
 * DATE     : Jan 2008
-*/


bool Math::IsNormalNumber( mTYPE x )
{
#ifdef __msvc__
    return _finite( x );
#else
    return finite( x );
#endif
}


mTYPE Math::IntPowerOf( mTYPE x, int y )
{
    if ( mIsUdf(x) )
	return mUdf(mTYPE);

    mTYPE ret = 1;
    if ( x == 0 )
	return y ? 0 : 1;

    if ( x > 1.5 || x < -1.5 )
    {
	if ( y > 150 ) return mUdf(mTYPE);
	if ( y < -150 ) return 0;
	if ( x > 1.99 || x < -1.99 )
	{
	    if ( y > 100 ) return mUdf(mTYPE);
	    if ( y < -100 ) return 0;
	}
    }
    else if ( x < 0.5 && x > -0.5 )
    {
	if ( y > 100 ) return 0;
	if ( y < -100 ) return 1;
    }

    while ( y )
    {
	if ( y > 0 )
	    { ret *= x; y--; }
	else
	    { ret /= x; y++; }
    }
    return ret;
}


mTYPE Math::PowerOf( mTYPE x, mTYPE y )
{
    int isneg = x < 0 ? 1 : 0;
 
    if ( x == 0 ) return y ? 0 : 1;
    if ( isneg ) x = -x;
 
    mTYPE ret = exp( y * log(x) );
    return isneg ? -ret : ret;
}


mTYPE Math::ACos( mTYPE c )
{
    if ( c >= 1 ) return 0;
    if ( c <= -1 ) return M_PI;
    return acos( c );
}


mTYPE Math::ASin( mTYPE s )
{
    if ( s >= 1 ) return M_PI_2;
    if ( s <= -1 ) return -M_PI_2;
    return asin( s );
}


mTYPE Math::Log( mTYPE s )
{
    return s <= 0 ? mUdf(mTYPE) : log( s );
}


mTYPE Math::Log10( mTYPE s )
{
    return s <= 0 ? mUdf(mTYPE) : log10( s );
}


mTYPE Math::Sqrt( mTYPE s )
{
    return s <= 0 ? 0 : sqrt( s );
}
