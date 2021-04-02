%******************************************************************************\
%* Technische Universitaet Darmstadt, Institut fuer Nachrichtentechnik
%* Copyright (c) 2004
%*
%* Author:
%*	Volker Fischer, Alexander Kurpiers
%*
%* Description:
%* 	Transmitter output filter
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

function [] = TransmitterFilter()
PLOT = 1;

nTaps = 200;
ftrans = 800; % Size of transition region
fs = 48000; % Constant for all cases

% Actual filter design for all DRM modes
h4_5 = DesignFilter(0, 4500, ftrans, nTaps, fs); % 4.5 kHz
h5 = DesignFilter(0, 5000, ftrans, nTaps, fs); % 5 kHz
h9 = DesignFilter(0, 9000, ftrans, nTaps, fs); % 9 kHz
h10 = DesignFilter(0, 10000, ftrans, nTaps, fs); % 10 kHz
h18 = DesignFilter(0, 18000, ftrans, nTaps, fs); % 18 kHz
h20 = DesignFilter(0, 20000, ftrans, nTaps, fs); % 20 kHz

if (PLOT == 1)
    close all;
    freqz(h4_5);
    figure;
    freqz(h5);
    figure;
    freqz(h9);
    figure;
    freqz(h10);
    figure;
    freqz(h18);
    figure;
    freqz(h20);
end


% Export coefficiants to file ****************************************
fid = fopen('TransmitterFilter.h', 'w');

fprintf(fid, '/* Automatically generated file with MATLAB */\n');
fprintf(fid, '/* File name: "TransmitterFilter.m" */\n');
fprintf(fid, '/* Filter taps in time-domain */\n\n');

fprintf(fid, '#ifndef _TRANSMITTERFILTER_H_\n');
fprintf(fid, '#define _TRANSMITTERFILTER_H_\n\n');

fprintf(fid, '#define NUM_TAPS_TRANSMFILTER            ');
fprintf(fid, int2str(nTaps));
fprintf(fid, '\n\n\n');

% Write filter taps
fprintf(fid, '/* 4.5 kHz bandwidth */\n');
fprintf(fid, 'static float fTransmFilt4_5[NUM_TAPS_TRANSMFILTER] =\n');
fprintf(fid, '{\n');
fprintf(fid, '	%.20ff,\n', h4_5(1:end - 1));
fprintf(fid, '	%.20ff\n', h4_5(end));
fprintf(fid, '};\n\n');

fprintf(fid, '/* 5 kHz bandwidth */\n');
fprintf(fid, 'static float fTransmFilt5[NUM_TAPS_TRANSMFILTER] =\n');
fprintf(fid, '{\n');
fprintf(fid, '	%.20ff,\n', h5(1:end - 1));
fprintf(fid, '	%.20ff\n', h5(end));
fprintf(fid, '};\n\n');

fprintf(fid, '/* 9 kHz bandwidth */\n');
fprintf(fid, 'static float fTransmFilt9[NUM_TAPS_TRANSMFILTER] =\n');
fprintf(fid, '{\n');
fprintf(fid, '	%.20ff,\n', h9(1:end - 1));
fprintf(fid, '	%.20ff\n', h9(end));
fprintf(fid, '};\n\n');

fprintf(fid, '/* 10 kHz bandwidth */\n');
fprintf(fid, 'static float fTransmFilt10[NUM_TAPS_TRANSMFILTER] =\n');
fprintf(fid, '{\n');
fprintf(fid, '	%.20ff,\n', h10(1:end - 1));
fprintf(fid, '	%.20ff\n', h10(end));
fprintf(fid, '};\n\n');

fprintf(fid, '/* 18 kHz bandwidth */\n');
fprintf(fid, 'static float fTransmFilt18[NUM_TAPS_TRANSMFILTER] =\n');
fprintf(fid, '{\n');
fprintf(fid, '	%.20ff,\n', h18(1:end - 1));
fprintf(fid, '	%.20ff\n', h18(end));
fprintf(fid, '};\n\n');

fprintf(fid, '/* 20 kHz bandwidth */\n');
fprintf(fid, 'static float fTransmFilt20[NUM_TAPS_TRANSMFILTER] =\n');
fprintf(fid, '{\n');
fprintf(fid, '	%.20ff,\n', h20(1:end - 1));
fprintf(fid, '	%.20ff\n', h20(end));
fprintf(fid, '};\n\n\n');

fprintf(fid, '#endif	/* _TRANSMITTERFILTER_H_ */\n');
fclose(fid);
return;


function [b] = DesignFilter(fstart, fstop, ftrans, nhil, fs)
    % Parks-McClellan optimal equiripple FIR filter design
	B = fstop - fstart;

	f = [0  B / 2  B / 2 + ftrans  fs / 2];
	m = [2 2 0 0];

	b = remez(nhil - 1, f * 2 / fs, m, [1 50]);
return;
