%******************************************************************************\
%* Technische Universitaet Darmstadt, Institut fuer Nachrichtentechnik
%* Copyright (c) 2003
%*
%* Author:
%*	Alexander Kurpiers
%*
%* Description:
%* 	Hilbert Filter for timing acquisition
%*  Runs at 48 kHz, can be downsampled to 48 kHz / 8 = 6 kHz
%*
%******************************************************************************
%*
%* This program is free software; you can redistribute it and/or modify it under
%* the terms of the GNU General Public License as published by the Free Software
%* Foundation; either version 2 of the License, or (at your option) any later 
%* version.
%*
%* This program is distributed in the hope that it will be useful, but WITHOUT 
%* ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
%* FOR A PARTICULAR PURPOSE. See the GNU General Public License for more 
%* details.
%*
%* You should have received a copy of the GNU General Public License along with
%* this program; if not, write to the Free Software Foundation, Inc., 
%* 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
%*
%******************************************************************************/

function [] = TimeSyncFilter()
PLOT = 1;

% Length of hilbert filter and characteristic frequencies
% 5 kHz bandwidth
nhil5 = 81;
fstart5 = 800;
fstop5 = 5200;
ftrans5 = 800; % Size of transition region

% 10 kHz bandwidth
nhil10 = 81;
fstart10 = 800;
fstop10 = 10200;
ftrans10 = 800; % Size of transition region


fs = 48000; % Constant for all cases

% Actual filter design
b5 = DesignFilter(fstart5, fstop5, ftrans5, nhil5, fs);
b10 = DesignFilter(fstart10, fstop10, ftrans10, nhil10, fs);

if (PLOT == 1)
    close all;

    % 5 kHz bandwidth filter
    subplot(2, 1, 1)
    f05 = (fstop5 + ftrans5) / 2;
    t = linspace(0, (nhil5 - 1) / fs, nhil5);
    hr = b5 .* cos(2 * pi * f05 * t);
    hi = b5 .* sin(2 * pi * f05 * t);

    % Complex hilbert filter
    hbp = hr + hi * j;

    [h1, f]= freqz(hbp, 1, 512, 'whole', fs);
    plot(f - fs / 2, 20 * log10(abs([h1(257:512); h1(1:256)])));
    axis([-fstart5 fstop5 + ftrans5 -90 10]);
    grid;
    zoom on;
    title('Hilbert-transformer 5 kHz bandwidth');
    xlabel('Frequency [Hz]');
    ylabel('Attenuation [dB]');

    % 10 kHz bandwidth filter
    subplot(2, 1, 2)
    f010 = (fstop10 + ftrans10) / 2;
    t = linspace(0, (nhil10 - 1) / fs, nhil10);
    hr = b10 .* cos(2 * pi * f010 * t);
    hi = b10 .* sin(2 * pi * f010 * t);

    % Complex hilbert filter
    hbp = hr + hi * j;

    [h1, f]= freqz(hbp, 1, 512, 'whole', fs);
    plot(f - fs / 2, 20 * log10(abs([h1(257:512); h1(1:256)])));
    axis([-fstart10 fstop10 + ftrans5 -90 10]);
    grid;
    zoom on;
    title('Hilbert-transformer 10 kHz bandwidth');
    xlabel('Frequency [Hz]');
    ylabel('Attenuation [dB]');
end


% Export coefficiants to file ****************************************
fid = fopen('TimeSyncFilter.h', 'w');

fprintf(fid, '/* Automatically generated file with MATLAB */\n');
fprintf(fid, '/* File name: "TimeSyncFilter.m" */\n');
fprintf(fid, '/* Filter taps in time-domain */\n\n');

fprintf(fid, '#ifndef _TIMESYNCFILTER_H_\n');
fprintf(fid, '#define _TIMESYNCFILTER_H_\n\n');


fprintf(fid, '#define NUM_TAPS_HILB_FILT_5            ');
fprintf(fid, int2str(nhil5));
fprintf(fid, '\n');
fprintf(fid, '#define HILB_FILT_BNDWIDTH_5            ');
fprintf(fid, int2str(fstop5 - fstart5 + ftrans5));
fprintf(fid, '\n');
fprintf(fid, '#define NUM_TAPS_HILB_FILT_10           ');
fprintf(fid, int2str(nhil10));
fprintf(fid, '\n');
fprintf(fid, '#define HILB_FILT_BNDWIDTH_10           ');
fprintf(fid, int2str(fstop10 - fstart10 + ftrans10));
fprintf(fid, '\n\n\n');


% Write filter taps
fprintf(fid, '/* Low pass prototype for Hilbert-filter 5 kHz bandwidth */\n');
fprintf(fid, 'static float fHilLPProt5[NUM_TAPS_HILB_FILT_5] =\n');
fprintf(fid, '{\n');
fprintf(fid, '	%.20ff,\n', b5(1:end - 1));
fprintf(fid, '	%.20ff\n', b5(end));
fprintf(fid, '};\n\n');
fprintf(fid, '/* Low pass prototype for Hilbert-filter 10 kHz bandwidth */\n');
fprintf(fid, 'static float fHilLPProt10[NUM_TAPS_HILB_FILT_10] =\n');
fprintf(fid, '{\n');
fprintf(fid, '	%.20ff,\n', b10(1:end - 1));
fprintf(fid, '	%.20ff\n', b10(end));
fprintf(fid, '};\n\n\n');

fprintf(fid, '#endif	/* _TIMESYNCFILTER_H_ */\n');
fclose(fid);
return;


function [b] = DesignFilter(fstart, fstop, ftrans, nhil, fs)
    % Parks-McClellan optimal equiripple FIR filter design
	B = fstop - fstart;

	f = [0  B / 2  B / 2 + ftrans  fs / 2];
	m = [2 2 0 0];

	b = remez(nhil - 1, f * 2 / fs, m, [1 10]);
return;
