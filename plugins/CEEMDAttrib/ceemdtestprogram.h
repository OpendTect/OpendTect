#pragma once
/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : Paul
 * DATE     : April 2013
-*/


/*!CEEMD Test Trace Data

Synthetic data courtesy Jianjun Hun and Mirko van der Baan, University of
Alberta. Signal is comprised of initial 20Hz cosine wave, superposed 100Hz
Morlet atom at 0.3s, two 30Hz Ricker wavelets at 1.07s and 1.1s, and three
different frequency components between 1.3s and 1.7s of respectively 7, 30
and 40Hz. Note that the 7Hz frequency compenents are not continuous, comprise
less than one period portions, appearing at 1.37s, 1.51s and 1.65s. Total 2001
samples (0 to 8000ms at 4ms sampling rate). Submitted for publication on
30 May 2012 to (?) in a paper entitled: Empirical mode decomposition for seismic
time-frequency analysis.

*/

namespace Attrib
{
    float testtrace[2001] =
	{0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 5.00E+06, 4.96E+06, 4.84E+06,
	4.65E+06, 4.38E+06, 4.05E+06, 3.64E+06, 3.19E+06, 2.68E+06, 2.13E+06,
	1.55E+06, 937000, 314000, -314000, -937000, -1.55E+06, -2.13E+06,
	-2.68E+06, -3.19E+06, -3.64E+06, -4.05E+06, -4.38E+06, -4.65E+06,
	-4.84E+06, -4.96E+06, -5.00E+06, -4.96E+06, -4.84E+06, -4.65E+06,
	-4.38E+06, -4.05E+06, -3.64E+06, -3.19E+06, -2.68E+06, -2.13E+06,
	-1.55E+06, -937000, -314000, 314000, 937000, 1.55E+06, 2.13E+06,
	2.68E+06, 3.19E+06, 3.64E+06, 4.05E+06, 4.38E+06, 4.65E+06, 4.84E+06,
	4.96E+06, 5.00E+06, 4.96E+06, 4.84E+06, 4.65E+06, 4.38E+06, 4.05E+06,
	3.64E+06, 3.19E+06, 2.68E+06, 2.13E+06, 1.55E+06, 937000, 314000,
	-314000, -937000, -1.55E+06, -2.13E+06, -2.68E+06, -3.19E+06,
	-3.64E+06, -4.05E+06, -4.38E+06, -4.65E+06, -4.84E+06, -4.96E+06,
	-5.00E+06, -4.96E+06, -4.84E+06, -4.65E+06, -4.38E+06, -4.05E+06,
	-3.64E+06, -3.19E+06, -2.68E+06, -2.13E+06, -1.55E+06, -937000,
	-314000, 314000, 937000, 1.55E+06, 2.13E+06, 2.68E+06, 3.19E+06,
	3.64E+06, 4.05E+06, 4.38E+06, 4.65E+06, 4.84E+06, 4.96E+06,
	5.00E+06, 4.96E+06, 4.84E+06, 4.65E+06, 4.38E+06, 4.05E+06,
	3.64E+06, 3.19E+06, 2.68E+06, 2.13E+06, 1.55E+06, 937000,
	314000, -314000, -937000, -1.55E+06, -2.13E+06, -2.68E+06,
	-3.19E+06, -3.64E+06, -4.05E+06, -4.38E+06, -4.65E+06, -4.84E+06,
	-4.96E+06, -5.00E+06, -4.96E+06, -4.84E+06, -4.65E+06, -4.38E+06,
	-4.05E+06, -3.64E+06, -3.19E+06, -2.68E+06, -2.13E+06, -1.55E+06,
	-937000, -314000, 314000, 937000, 1.55E+06, 2.13E+06, 2.68E+06,
	3.19E+06, 3.64E+06, 4.05E+06, 4.38E+06, 4.65E+06, 4.84E+06, 4.96E+06,
	5.00E+06, 4.96E+06, 4.84E+06, 4.65E+06, 4.38E+06, 4.05E+06, 3.64E+06,
	3.19E+06, 2.68E+06, 2.13E+06, 1.55E+06, 937000, 314000, -314000,
	-937000, -1.55E+06, -2.13E+06, -2.68E+06, -3.19E+06, -3.64E+06,
	-4.03E+06, -4.35E+06, -4.61E+06, -4.82E+06, -4.99E+06, -5.12E+06,
	-5.17E+06, -5.08E+06, -4.78E+06, -4.21E+06, -3.44E+06, -2.64E+06,
	-2.13E+06, -2.16E+06, -2.80E+06, -3.74E+06, -4.30E+06, -3.64E+06,
	-1.22E+06, 2.76E+06, 7.14E+06, 1.01E+07, 1.01E+07, 6.36E+06, 124000,
	-6.04E+06, -9.07E+06, -6.94E+06, 198000, 9.77E+06, 1.79E+07, 2.10E+07,
	1.77E+07, 9.46E+06, -264000, -7.54E+06, -9.81E+06, -6.90E+06, -841000,
	5.30E+06, 8.93E+06, 8.94E+06, 5.91E+06, 1.51E+06, -2.47E+06,
	-4.87E+06, -5.49E+06, -4.87E+06, -3.85E+06, -3.12E+06, -2.98E+06,
	-3.38E+06, -4.04E+06, -4.67E+06, -5.09E+06, -5.24E+06, -5.17E+06,
	-4.96E+06, -4.68E+06, -4.36E+06, -4.01E+06, -3.61E+06, -3.17E+06,
	-2.68E+06, -2.13E+06, -1.55E+06, -940000, -316000, 314000, 937000,
	1.55E+06, 2.13E+06, 2.68E+06, 3.19E+06, 3.64E+06, 4.05E+06, 4.38E+06,
	4.65E+06, 4.84E+06, 4.96E+06, 5.00E+06, 4.96E+06, 4.84E+06, 4.65E+06,
	4.38E+06, 4.05E+06, 3.64E+06, 3.19E+06, 2.68E+06, 2.13E+06, 1.55E+06,
	937000, 314000, -314000, -937000, -1.55E+06, -2.13E+06, -2.68E+06,
	-3.19E+06, -3.64E+06, -4.05E+06, -4.38E+06, -4.65E+06, -4.84E+06,
	-4.96E+06, -5.00E+06, -4.96E+06, -4.84E+06, -4.65E+06, -4.38E+06,
	-4.05E+06, -3.64E+06, -3.19E+06, -2.68E+06, -2.13E+06, -1.55E+06,
	-937000, -314000, 314000, 937000, 1.55E+06, 2.13E+06, 2.68E+06,
	3.19E+06, 3.64E+06, 4.05E+06, 4.38E+06, 4.65E+06, 4.84E+06, 4.96E+06,
	5.00E+06, 4.96E+06, 4.84E+06, 4.65E+06, 4.38E+06, 4.05E+06, 3.64E+06,
	3.19E+06, 2.68E+06, 2.13E+06, 1.55E+06, 937000, 314000, -314000,
	-937000, -1.55E+06, -2.13E+06, -2.68E+06, -3.19E+06, -3.64E+06,
	-4.05E+06, -4.38E+06, -4.65E+06, -4.84E+06, -4.96E+06, -5.00E+06,
	-4.96E+06, -4.84E+06, -4.65E+06, -4.38E+06, -4.05E+06, -3.64E+06,
	-3.19E+06, -2.68E+06, -2.13E+06, -1.55E+06, -937000, -314000,
	314000, 937000, 1.55E+06, 2.13E+06, 2.68E+06, 3.19E+06, 3.64E+06,
	4.05E+06, 4.38E+06, 4.65E+06, 4.84E+06, 4.96E+06, 5.00E+06, 4.96E+06,
	4.84E+06, 4.65E+06, 4.38E+06, 4.05E+06, 3.64E+06, 3.19E+06, 2.68E+06,
	2.13E+06, 1.55E+06, 937000, 314000, -314000, -937000, -1.55E+06,
	-2.13E+06, -2.68E+06, -3.19E+06, -3.64E+06, -4.05E+06, -4.38E+06,
	-4.65E+06, -4.84E+06, -4.96E+06, -5.00E+06, -4.96E+06, -4.84E+06,
	-4.65E+06, -4.38E+06, -4.05E+06, -3.64E+06, -3.19E+06, -2.68E+06,
	-2.13E+06, -1.55E+06, -937000, -314000, 314000, 937000, 1.55E+06,
	2.13E+06, 2.68E+06, 3.19E+06, 3.64E+06, 4.05E+06, 4.38E+06,
	4.65E+06, 4.84E+06, 4.96E+06, 5.00E+06, 4.96E+06, 4.84E+06,
	4.65E+06, 4.38E+06, 4.05E+06, 3.64E+06, 3.19E+06, 2.68E+06,
	2.13E+06, 1.55E+06, 937000, 314000, -314000, -937000,
	-1.55E+06, -2.13E+06, -2.68E+06, -3.19E+06, -3.64E+06,
	-4.05E+06, -4.38E+06, -4.65E+06, -4.84E+06, -4.96E+06,
	-5.00E+06, -4.96E+06, -4.84E+06, -4.65E+06, -4.38E+06,
	-4.05E+06, -3.64E+06, -3.19E+06, -2.68E+06, -2.13E+06, -1.55E+06,
	-937000, -314000, 314000, 937000, 1.55E+06, 2.13E+06, 2.68E+06,
	3.19E+06, 3.64E+06, 4.05E+06, 4.38E+06, 4.65E+06, 4.84E+06, 4.96E+06,
	5.00E+06, 4.96E+06, 4.84E+06, 4.65E+06, 4.38E+06, 4.05E+06, 3.64E+06,
	3.19E+06, 2.68E+06, 2.13E+06, 1.55E+06, 937000, 314000, -314000,
	-937000, -1.55E+06, -2.13E+06, -2.68E+06, -3.19E+06, -3.64E+06,
	-4.05E+06, -4.38E+06, -4.65E+06, -4.84E+06, -4.96E+06, -5.00E+06,
	-4.96E+06, -4.84E+06, -4.65E+06, -4.38E+06, -4.05E+06, -3.64E+06,
	-3.19E+06, -2.68E+06, -2.13E+06, -1.55E+06, -937000, -314000,
	314000, 937000, 1.55E+06, 2.13E+06, 2.68E+06, 3.19E+06, 3.64E+06,
	4.05E+06, 4.38E+06, 4.65E+06, 4.84E+06, 4.96E+06, 5.00E+06, 4.96E+06,
	4.84E+06, 4.65E+06, 4.38E+06, 4.05E+06, 3.64E+06, 3.19E+06, 2.68E+06,
	2.13E+06, 1.55E+06, 937000, 314000, -314000, -937000, -1.55E+06,
	-2.13E+06, -2.68E+06, -3.19E+06, -3.64E+06, -4.05E+06, -4.38E+06,
	-4.65E+06, -4.84E+06, -4.96E+06, -5.00E+06, -4.96E+06, -4.84E+06,
	-4.65E+06, -4.38E+06, -4.05E+06, -3.64E+06, -3.19E+06, -2.68E+06,
	-2.13E+06, -1.55E+06, -937000, -314000, 314000, 937000, 1.55E+06,
	2.13E+06, 2.68E+06, 3.19E+06, 3.64E+06, 4.05E+06, 4.38E+06, 4.65E+06,
	4.84E+06, 4.96E+06, 5.00E+06, 4.96E+06, 4.84E+06, 4.65E+06, 4.38E+06,
	4.05E+06, 3.64E+06, 3.19E+06, 2.68E+06, 2.13E+06, 1.55E+06, 937000,
	314000, -314000, -937000, -1.55E+06, -2.13E+06, -2.68E+06, -3.19E+06,
	-3.64E+06, -4.05E+06, -4.38E+06, -4.65E+06, -4.84E+06, -4.96E+06,
	-5.00E+06, -4.96E+06, -4.84E+06, -4.65E+06, -4.38E+06, -4.05E+06,
	-3.64E+06, -3.19E+06, -2.68E+06, -2.13E+06, -1.55E+06, -937000,
	-314000, 314000, 937000, 1.55E+06, 2.13E+06, 2.68E+06, 3.19E+06,
	3.64E+06, 4.05E+06, 4.38E+06, 4.65E+06, 4.84E+06, 4.96E+06,
	5.00E+06, 4.96E+06, 4.84E+06, 4.65E+06, 4.38E+06, 4.05E+06,
	3.64E+06, 3.19E+06, 2.68E+06, 2.13E+06, 1.55E+06, 937000,
	314000, -314000, -937000, -1.55E+06, -2.13E+06, -2.68E+06,
	-3.19E+06, -3.64E+06, -4.05E+06, -4.38E+06, -4.65E+06,
	-4.84E+06, -4.96E+06, -5.00E+06, -4.96E+06, -4.84E+06,
	-4.65E+06, -4.38E+06, -4.05E+06, -3.64E+06, -3.19E+06, -2.68E+06,
	-2.13E+06, -1.55E+06, -937000, -314000, 314000, 937000, 1.55E+06,
	2.13E+06, 2.68E+06, 3.19E+06, 3.64E+06, 4.05E+06, 4.38E+06,
	4.65E+06, 4.84E+06, 4.96E+06, 5.00E+06, 4.96E+06, 4.84E+06,
	4.65E+06, 4.38E+06, 4.05E+06, 3.64E+06, 3.19E+06, 2.68E+06,
	2.13E+06, 1.55E+06, 937000, 314000, -314000, -937000, -1.55E+06,
	-2.13E+06, -2.68E+06, -3.19E+06, -3.64E+06, -4.05E+06, -4.38E+06,
	-4.65E+06, -4.84E+06, -4.96E+06, -5.00E+06, -4.96E+06, -4.84E+06,
	-4.65E+06, -4.38E+06, -4.05E+06, -3.64E+06, -3.19E+06, -2.68E+06,
	-2.13E+06, -1.55E+06, -937000, -314000, 314000, 937000, 1.55E+06,
	2.13E+06, 2.68E+06, 3.19E+06, 3.64E+06, 4.05E+06, 4.38E+06,
	4.65E+06, 4.84E+06, 4.96E+06, 5.00E+06, 4.96E+06, 4.84E+06, 4.65E+06,
	4.38E+06, 4.05E+06, 3.64E+06, 3.19E+06, 2.68E+06, 2.13E+06, 1.55E+06,
	937000, 314000, -314000, -937000, -1.55E+06, -2.13E+06, -2.68E+06,
	-3.19E+06, -3.64E+06, -4.05E+06, -4.38E+06, -4.65E+06, -4.84E+06,
	-4.96E+06, -5.00E+06, -4.96E+06, -4.84E+06, -4.65E+06, -4.38E+06,
	-4.05E+06, -3.64E+06, -3.19E+06, -2.68E+06, -2.13E+06, -1.55E+06,
	-937000, -314000, 314000, 937000, 1.55E+06, 2.13E+06, 2.68E+06,
	3.19E+06, 3.64E+06, 4.05E+06, 4.38E+06, 4.65E+06, 4.84E+06, 4.96E+06,
	5.00E+06, 4.96E+06, 4.84E+06, 4.65E+06, 4.38E+06, 4.05E+06, 3.64E+06,
	3.19E+06, 2.68E+06, 2.13E+06, 1.55E+06, 937000, 314000, -314000,
	-937000, -1.55E+06, -2.13E+06, -2.68E+06, -3.19E+06, -3.64E+06,
	-4.05E+06, -4.38E+06, -4.65E+06, -4.84E+06, -4.96E+06, -5.00E+06,
	-4.96E+06, -4.84E+06, -4.65E+06, -4.38E+06, -4.05E+06, -3.64E+06,
	-3.19E+06, -2.68E+06, -2.13E+06, -1.55E+06, -937000, -314000,
	314000, 937000, 1.55E+06, 2.13E+06, 2.68E+06, 3.19E+06, 3.64E+06,
	4.05E+06, 4.38E+06, 4.65E+06, 4.84E+06, 4.96E+06, 5.00E+06, 4.96E+06,
	4.84E+06, 4.65E+06, 4.38E+06, 4.05E+06, 3.64E+06, 3.19E+06, 2.68E+06,
	2.13E+06, 1.55E+06, 937000, 314000, -314000, -937000, -1.55E+06,
	-2.13E+06, -2.68E+06, -3.19E+06, -3.64E+06, -4.05E+06, -4.38E+06,
	-4.65E+06, -4.84E+06, -4.96E+06, -5.00E+06, -4.96E+06, -4.84E+06,
	-4.65E+06, -4.38E+06, -4.05E+06, -3.64E+06, -3.19E+06, -2.68E+06,
	-2.13E+06, -1.55E+06, -937000, -314000, 314000, 937000, 1.55E+06,
	2.13E+06, 2.68E+06, 3.19E+06, 3.64E+06, 4.05E+06, 4.38E+06, 4.65E+06,
	4.84E+06, 4.96E+06, 5.00E+06, 4.96E+06, 4.84E+06, 4.65E+06, 4.38E+06,
	4.05E+06, 3.64E+06, 3.19E+06, 2.68E+06, 2.13E+06, 1.55E+06, 937000,
	314000, -314000, -937000, -1.55E+06, -2.13E+06, -2.68E+06, -3.19E+06,
	-3.64E+06, -4.05E+06, -4.38E+06, -4.65E+06, -4.84E+06, -4.96E+06,
	-5.00E+06, -4.96E+06, -4.84E+06, -4.65E+06, -4.38E+06, -4.05E+06,
	-3.64E+06, -3.19E+06, -2.68E+06, -2.13E+06, -1.55E+06, -937000,
	-314000, 314000, 937000, 1.55E+06, 2.13E+06, 2.68E+06, 3.19E+06,
	3.64E+06, 4.05E+06, 4.38E+06, 4.65E+06, 4.84E+06, 4.96E+06, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, -1490, -2200, -3230, -4700,
	-6780, -9690, -13700, -19300, -26800, -37000, -50600, -68500, -91900,
	-122000, -161000, -210000, -272000, -348000, -441000, -554000, -688000,
	-847000, -1.03E+06, -1.24E+06, -1.48E+06, -1.75E+06, -2.04E+06,
	-2.35E+06, -2.68E+06, -3.01E+06, -3.35E+06, -3.67E+06, -3.98E+06,
	-4.25E+06, -4.48E+06, -4.67E+06, -4.81E+06, -4.91E+06, -4.97E+06,
	-5.00E+06, -4.99E+06, -4.94E+06, -4.82E+06, -4.55E+06, -4.06E+06,
	-3.24E+06, -1.99E+06, -223000, 2.09E+06, 4.88E+06, 8.00E+06, 1.12E+07,
	1.41E+07, 1.65E+07, 1.79E+07, 1.83E+07, 1.74E+07, 1.54E+07, 1.25E+07,
	9.03E+06, 5.34E+06, 1.76E+06, -1.45E+06, -4.12E+06, -6.19E+06,
	-7.69E+06, -8.70E+06, -9.32E+06, -9.67E+06, -9.84E+06, -9.89E+06,
	-9.84E+06, -9.67E+06, -9.32E+06, -8.70E+06, -7.69E+06, -6.19E+06,
	-4.12E+06, -1.45E+06, 1.76E+06, 5.34E+06, 9.03E+06, 1.25E+07,
	1.54E+07, 1.74E+07, 1.83E+07, 1.79E+07, 1.65E+07, 1.41E+07, 1.12E+07,
	8.00E+06, 4.88E+06, 2.09E+06, -223000, -1.99E+06, -3.24E+06,
	-4.06E+06, -4.55E+06, -4.82E+06, -4.94E+06, -4.99E+06, -5.00E+06,
	-4.97E+06, -4.91E+06, -4.81E+06, -4.67E+06, -4.48E+06, -4.25E+06,
	-3.98E+06, -3.67E+06, -3.35E+06, -3.01E+06, -2.68E+06, -2.35E+06,
	-2.04E+06, -1.75E+06, -1.48E+06, -1.24E+06, -1.03E+06, -847000,
	-688000, -554000, -441000, -348000, -272000, -210000, -161000,
	-122000, -91900, -68500, -50600, -37000, -26800, -19300, -13700,
	-9690, -6780, -4700, -3230, -2200, -1490, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 4.96E+06, 4.84E+06, 4.65E+06,
	4.38E+06, 4.05E+06, 3.64E+06, 3.19E+06, 2.68E+06, 2.13E+06, 1.55E+06,
	937000, 314000, -314000, -937000, -1.55E+06, -2.13E+06, -2.68E+06,
	-3.19E+06, -3.64E+06, -4.05E+06, -4.38E+06, -4.65E+06, -4.84E+06,
	-4.96E+06, -5.00E+06, -4.96E+06, -4.84E+06, -4.65E+06, -4.38E+06,
	-4.05E+06, -3.64E+06, -3.19E+06, -2.68E+06, -2.13E+06, -1.55E+06,
	-937000, -314000, 314000, 937000, 1.55E+06, 2.13E+06, 2.68E+06,
	3.19E+06, 3.64E+06, 4.05E+06, 4.38E+06, 4.65E+06, 4.84E+06, 4.96E+06,
	5.00E+06, 4.96E+06, 4.84E+06, 4.65E+06, 4.38E+06, 4.05E+06, 3.64E+06,
	3.19E+06, 2.68E+06, 2.13E+06, 1.55E+06, 937000, 314000, -314000,
	-937000, -1.55E+06, -2.13E+06, -2.68E+06, -3.19E+06, -3.64E+06,
	-4.05E+06, -4.38E+06, -4.65E+06, -4.84E+06, -4.96E+06, -5.00E+06,
	-4.96E+06, -4.84E+06, -4.65E+06, -4.38E+06, -4.05E+06, -3.64E+06,
	-3.19E+06, -2.68E+06, -2.13E+06, -1.55E+06, -937000, -314000, 314000,
	937000, 1.55E+06, 2.13E+06, 2.68E+06, 3.19E+06, 3.64E+06, 4.05E+06,
	4.38E+06, 4.65E+06, 4.84E+06, 4.96E+06, 471000, 4.65E-08, -482000,
	-986000, -1.51E+06, -2.05E+06, -2.60E+06, -3.16E+06, -3.71E+06,
	-4.26E+06, -4.79E+06, -5.29E+06, -5.76E+06, -6.19E+06, -6.56E+06,
	-6.88E+06, -7.14E+06, -7.34E+06, -7.47E+06, -7.55E+06, -7.57E+06,
	-7.54E+06, -7.48E+06, -7.38E+06, -7.27E+06, -7.16E+06, -7.06E+06,
	-6.99E+06, -6.96E+06, -6.98E+06, -7.07E+06, -7.22E+06, -7.45E+06,
	-7.76E+06, -8.15E+06, -8.60E+06, -9.12E+06, -9.68E+06, -1.03E+07,
	-1.09E+07, -1.15E+07, -1.21E+07, -1.26E+07, -1.30E+07, -1.33E+07,
	-1.35E+07, -1.35E+07, -1.34E+07, -1.31E+07, -1.26E+07, -1.20E+07,
	-1.11E+07, -1.01E+07, -8.93E+06, -7.63E+06, -6.21E+06, -4.72E+06,
	-3.19E+06, -1.65E+06, -138000, 1.30E+06, 2.64E+06, 3.83E+06, 4.86E+06,
	5.69E+06, 6.31E+06, 6.70E+06, 6.86E+06, 6.79E+06, 6.50E+06, 6.00E+06,
	5.32E+06, 4.49E+06, 3.55E+06, 2.54E+06, 1.49E+06, 469000, -493000,
	-1.35E+06, -2.05E+06, -2.57E+06, -2.87E+06, -2.93E+06, -2.73E+06,
	-2.26E+06, -1.53E+06, -534000, 698000, 2.14E+06, 3.77E+06, 5.53E+06,
	7.38E+06, 9.27E+06, 1.11E+07, 1.30E+07, 1.47E+07, 1.62E+07, 1.75E+07,
	1.86E+07, 1.94E+07, 1.98E+07, 2.00E+07, 1.98E+07, 1.94E+07, 1.86E+07,
	1.75E+07, 1.62E+07, 1.47E+07, 1.30E+07, 1.11E+07, 9.27E+06, 7.38E+06,
	5.53E+06, 3.77E+06, 2.14E+06, 698000, -534000, -1.53E+06, -2.26E+06,
	-2.73E+06, -2.93E+06, -2.87E+06, -2.57E+06, -2.05E+06, -1.35E+06,
	-493000, 469000, 1.49E+06, 2.54E+06, 3.55E+06, 4.49E+06, 5.32E+06,
	6.00E+06, 6.50E+06, 6.79E+06, 6.86E+06, 6.70E+06, 6.31E+06, 5.69E+06,
	4.86E+06, 3.83E+06, 2.64E+06, 1.30E+06, -138000, -1.65E+06, -3.19E+06,
	-4.72E+06, -6.21E+06, -7.63E+06, -8.93E+06, -1.01E+07, -1.11E+07,
	-1.20E+07, -1.26E+07, -1.31E+07, -1.34E+07, -1.35E+07, -1.35E+07,
	-1.33E+07, -1.30E+07, -1.26E+07, -1.21E+07, -1.15E+07, -1.09E+07,
	-1.03E+07, -9.68E+06, -9.12E+06, -8.60E+06, -8.15E+06, -7.76E+06,
	-7.45E+06, -7.22E+06, -7.07E+06, -6.98E+06, -6.96E+06, -6.99E+06,
	-7.06E+06, -7.16E+06, -7.27E+06, -7.38E+06, -7.48E+06, -6.76E+06,
	-6.82E+06, -6.88E+06, -6.92E+06, -6.93E+06, -6.88E+06, -6.78E+06,
	-6.61E+06, -6.37E+06, -6.05E+06, -5.66E+06, -5.19E+06, -4.67E+06,
	-4.10E+06, -3.49E+06, -2.87E+06, -2.24E+06, -1.63E+06, -1.04E+06,
	-497000, -1.47E-08, 456000, 881000, 1.28E+06, 1.65E+06, 2.01E+06,
	2.37E+06, 2.73E+06, 3.12E+06, 3.54E+06, 4.00E+06, 4.51E+06, 5.07E+06,
	5.67E+06, 6.31E+06, 6.97E+06, 7.63E+06, 8.28E+06, 8.88E+06, 9.42E+06,
	9.86E+06, 1.02E+07, 1.04E+07, 1.04E+07, 1.04E+07, 1.01E+07, 9.80E+06,
	9.35E+06, 8.84E+06, 8.30E+06, 7.77E+06, 7.29E+06, 6.90E+06, 6.63E+06,
	6.53E+06, 6.60E+06, 6.86E+06, 7.31E+06, 7.92E+06, 8.67E+06, 9.51E+06,
	1.04E+07, 1.13E+07, 1.21E+07, 1.27E+07, 1.32E+07, 1.34E+07, 1.33E+07,
	1.29E+07, 1.22E+07, 1.11E+07, 9.80E+06, 8.23E+06, 6.49E+06, 4.64E+06,
	2.77E+06, 965000, -686000, -2.10E+06, -3.22E+06, -3.99E+06, -4.37E+06,
	-4.35E+06, -3.95E+06, -3.20E+06, -2.16E+06, -909000, 466000, 1.86E+06,
	3.17E+06, 4.28E+06, 5.11E+06, 5.58E+06, 5.61E+06, 5.18E+06, 4.28E+06,
	2.91E+06, 1.14E+06, -986000, -3.36E+06, -5.88E+06, -8.42E+06,
	-1.09E+07, -1.31E+07, -1.50E+07, -1.64E+07, -1.74E+07, -1.78E+07,
	-1.77E+07, -1.70E+07, -1.58E+07, -1.43E+07, -1.23E+07, -1.02E+07,
	-8.05E+06, -5.92E+06, -3.96E+06, -2.31E+06, -1.05E+06, -267000, 0,
	-267000, -1.05E+06, -2.31E+06, -3.96E+06, -5.92E+06, -8.05E+06,
	-1.02E+07, -1.23E+07, -1.43E+07, -1.58E+07, -1.70E+07, -1.77E+07,
	-1.78E+07, -1.74E+07, -1.64E+07, -1.50E+07, -1.31E+07, -1.09E+07,
	-8.42E+06, -5.88E+06, -3.36E+06, -986000, 1.14E+06, 2.91E+06,
	4.28E+06, 5.18E+06, 5.61E+06, 5.58E+06, 5.11E+06, 4.28E+06, 3.17E+06,
	1.86E+06, 466000, -909000, -2.16E+06, -3.20E+06, -3.95E+06, -4.35E+06,
	-4.37E+06, -3.99E+06, -3.22E+06, -2.10E+06, -686000, 965000, 2.77E+06,
	4.64E+06, 6.49E+06, 8.23E+06, 9.80E+06, 1.11E+07, 1.22E+07, 1.29E+07,
	1.33E+07, 1.34E+07, 1.32E+07, 1.27E+07, 1.21E+07, 1.13E+07, 1.04E+07,
	9.51E+06, 8.67E+06, 7.92E+06, 7.31E+06, 6.86E+06, 6.60E+06, 6.53E+06,
	6.63E+06, 6.90E+06, 7.29E+06, 7.77E+06, 8.30E+06, 8.84E+06, 9.35E+06,
	9.80E+06, 1.01E+07, 1.04E+07, 1.04E+07, 1.04E+07, 1.02E+07, 9.86E+06,
	7.80E+06, 7.50E+06, 7.18E+06, 6.85E+06, 6.49E+06, 6.13E+06, 5.75E+06,
	5.36E+06, 4.95E+06, 4.54E+06, 4.12E+06, 3.68E+06, 3.24E+06, 2.79E+06,
	2.33E+06, 1.87E+06, 1.41E+06, 941000, 471000, 4.96E+06, 4.84E+06,
	4.65E+06, 4.38E+06, 4.05E+06, 3.64E+06, 3.19E+06, 2.68E+06, 2.13E+06,
	1.55E+06, 937000, 314000, -314000, -937000, -1.55E+06, -2.13E+06,
	-2.68E+06, -3.19E+06, -3.64E+06, -4.05E+06, -4.38E+06, -4.65E+06,
	-4.84E+06, -4.96E+06, -5.00E+06, -4.96E+06, -4.84E+06, -4.65E+06,
	-4.38E+06, -4.05E+06, -3.64E+06, -3.19E+06, -2.68E+06, -2.13E+06,
	-1.55E+06, -937000, -314000, 314000, 937000, 1.55E+06, 2.13E+06,
	2.68E+06, 3.19E+06, 3.64E+06, 4.05E+06, 4.38E+06, 4.65E+06, 4.84E+06,
	4.96E+06, 5.00E+06, 4.96E+06, 4.84E+06, 4.65E+06, 4.38E+06, 4.05E+06,
	3.64E+06, 3.19E+06, 2.68E+06, 2.13E+06, 1.55E+06, 937000, 314000,
	-314000, -937000, -1.55E+06, -2.13E+06, -2.68E+06, -3.19E+06,
	-3.64E+06, -4.05E+06, -4.38E+06, -4.65E+06, -4.84E+06, -4.96E+06,
	-5.00E+06, -4.96E+06, -4.84E+06, -4.65E+06, -4.38E+06, -4.05E+06,
	-3.64E+06, -3.19E+06, -2.68E+06, -2.13E+06, -1.55E+06, -937000,
	-314000, 314000, 937000, 1.55E+06, 2.13E+06, 2.68E+06, 3.19E+06,
	3.64E+06, 4.05E+06, 4.38E+06, 4.65E+06, 4.84E+06, 4.96E+06, 5.00E+06,
	4.96E+06, 4.84E+06, 4.65E+06, 4.38E+06, 4.05E+06, 3.64E+06, 3.19E+06,
	2.68E+06, 2.13E+06, 1.55E+06, 937000, 314000, -314000, -937000,
	-1.55E+06, -2.13E+06, -2.68E+06, -3.19E+06, -3.64E+06, -4.05E+06,
	-4.38E+06, -4.65E+06, -4.84E+06, -4.96E+06, -5.00E+06, -4.96E+06,
	-4.84E+06, -4.65E+06, -4.38E+06, -4.05E+06, -3.64E+06, -3.19E+06,
	-2.68E+06, -2.13E+06, -1.55E+06, -937000, -314000, 314000, 937000,
	1.55E+06, 2.13E+06, 2.68E+06, 3.19E+06, 3.64E+06, 4.05E+06, 4.38E+06,
	4.65E+06, 4.84E+06, 4.96E+06, 5.00E+06, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0};

} // namespace Attrib

