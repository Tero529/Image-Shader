/*
 Copyright (C) 2014  William B. Talmadge
 
 This program is free software: you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation, either version 3 of the License, or
 (at your option) any later version.
 
 This program is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.
 
 You should have received a copy of the GNU General Public License
 along with this program.  If not, see <http://www.gnu.org/licenses/>
 */

#include "FFTRealTransformHighFreq.h"
const std::string FFTRealTransformHighFreq::fragShaderRe = SHADER_STRING
(
 varying highp vec2 n;
 varying highp vec2 N2mn;
 
 uniform sampler2D inputRe;
 uniform sampler2D inputIm;
 uniform sampler2D phaseTable;
 uniform highp float s;
 
 void main()
{
    highp vec4 re = texture2D(inputRe, n);
    highp vec4 ren2 = texture2D(inputRe, N2mn);
    highp vec4 im = texture2D(inputIm, n);
    highp vec4 imn2 = texture2D(inputIm, N2mn);
    highp vec4 p = texture2D(phaseTable, n);
    highp vec4 result;
    result.rgb = 0.5*(re.rgb + ren2.rgb + p.x*s*(im.rgb + imn2.rgb) + p.y*s*(re.rgb  - ren2.rgb));
    result.a = 1.0;
    gl_FragColor = result;
}
 );
const std::string FFTRealTransformHighFreq::fragShaderIm = SHADER_STRING
(
 varying highp vec2 n;
 varying highp vec2 N2mn;
 
 uniform sampler2D inputRe;
 uniform sampler2D inputIm;
 uniform sampler2D phaseTable;
 uniform highp float s;
 
 void main()
{
    highp vec4 re = texture2D(inputRe, n);
    highp vec4 ren2 = texture2D(inputRe, N2mn);
    highp vec4 im = texture2D(inputIm, n);
    highp vec4 imn2 = texture2D(inputIm, N2mn);
    highp vec4 p = texture2D(phaseTable, n);
    highp vec4 result;
    result.rgb = 0.5*(p.x*s*(re.rgb  - ren2.rgb) + imn2.rgb - im.rgb - p.y*s*(im.rgb + imn2.rgb));
    result.a = 1.0;
    gl_FragColor = result;
}
 );

const std::string FFTRealTransformHighFreq::vertShader = SHADER_STRING
(
 attribute vec4 positionIn;
 attribute vec2 nIn;
 attribute vec2 N2mnIn;
 
 varying vec2 n;
 varying vec2 N2mn;
 
 uniform mat4 orthoMatrix;
 
 void main()
{
    gl_Position = orthoMatrix*positionIn;
    n = nIn;
    N2mn = N2mnIn;
}
 
 );