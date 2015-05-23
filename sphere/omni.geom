#version 120
#extension GL_EXT_geometry_shader4 : enable // it is extension in 120

uniform float frame_num;
// varying in vec4 color[];
// varying in vec3 normal[], lightDir[], eyeVec[];

// varying out vec4 fcolor;
// varying out vec3 fnormal, flightDir;//, feyeVec;

// dividing 2PI by 8, 9 values for rotating fully
uniform float sin_lkup[] = float[](
  0.00000000,
  0.70710678,
  1.00000000,
  0.70710678,
  0.00000000,
  -0.70710678,
  -1.00000000,
  -0.70710678,
  0.00000000
);

uniform float cos_lkup[] = float[](
  1.00000000,
  0.70710678,
  0.00000000,
  -0.70710678,
  -1.00000000,
  -0.70710678,
  0.00000000,
  0.70710678,
  1.00000000
);


float rand(vec2 co){
  return fract(sin(dot(co.xy ,vec2(12.9898,78.233))) * 43758.5453);
}

///////////////////////////////////////////////////////////////////////
// 3 D   N O I S E
///////////////////////////////////////////////////////////////////////
// GLSL textureless classic 3D noise "cnoise",
// with an RSL-style periodic variant "pnoise".
// Author:  Stefan Gustavson (stefan.gustavson@liu.se)
// Version: 2011-10-11
//
// Many thanks to Ian McEwan of Ashima Arts for the
// ideas for permutation and gradient selection.
//
// Copyright (c) 2011 Stefan Gustavson. All rights reserved.
// Distributed under the MIT license. See LICENSE file.
// https://github.com/ashima/webgl-noise
//

vec3 mod289(vec3 x)
{
  return x - floor(x * (1.0 / 289.0)) * 289.0;
}

vec4 mod289(vec4 x)
{
  return x - floor(x * (1.0 / 289.0)) * 289.0;
}

vec4 permute(vec4 x)
{
  return mod289(((x*34.0)+1.0)*x);
}

vec4 taylorInvSqrt(vec4 r)
{
  return 1.79284291400159 - 0.85373472095314 * r;
}

vec3 fade(vec3 t) {
  return t*t*t*(t*(t*6.0-15.0)+10.0);
}

// Classic Perlin noise
float cnoise(vec3 P)
{
  vec3 Pi0 = floor(P); // Integer part for indexing
  vec3 Pi1 = Pi0 + vec3(1.0); // Integer part + 1
  Pi0 = mod289(Pi0);
  Pi1 = mod289(Pi1);
  vec3 Pf0 = fract(P); // Fractional part for interpolation
  vec3 Pf1 = Pf0 - vec3(1.0); // Fractional part - 1.0
  vec4 ix = vec4(Pi0.x, Pi1.x, Pi0.x, Pi1.x);
  vec4 iy = vec4(Pi0.yy, Pi1.yy);
  vec4 iz0 = Pi0.zzzz;
  vec4 iz1 = Pi1.zzzz;

  vec4 ixy = permute(permute(ix) + iy);
  vec4 ixy0 = permute(ixy + iz0);
  vec4 ixy1 = permute(ixy + iz1);

  vec4 gx0 = ixy0 * (1.0 / 7.0);
  vec4 gy0 = fract(floor(gx0) * (1.0 / 7.0)) - 0.5;
  gx0 = fract(gx0);
  vec4 gz0 = vec4(0.5) - abs(gx0) - abs(gy0);
  vec4 sz0 = step(gz0, vec4(0.0));
  gx0 -= sz0 * (step(0.0, gx0) - 0.5);
  gy0 -= sz0 * (step(0.0, gy0) - 0.5);

  vec4 gx1 = ixy1 * (1.0 / 7.0);
  vec4 gy1 = fract(floor(gx1) * (1.0 / 7.0)) - 0.5;
  gx1 = fract(gx1);
  vec4 gz1 = vec4(0.5) - abs(gx1) - abs(gy1);
  vec4 sz1 = step(gz1, vec4(0.0));
  gx1 -= sz1 * (step(0.0, gx1) - 0.5);
  gy1 -= sz1 * (step(0.0, gy1) - 0.5);

  vec3 g000 = vec3(gx0.x,gy0.x,gz0.x);
  vec3 g100 = vec3(gx0.y,gy0.y,gz0.y);
  vec3 g010 = vec3(gx0.z,gy0.z,gz0.z);
  vec3 g110 = vec3(gx0.w,gy0.w,gz0.w);
  vec3 g001 = vec3(gx1.x,gy1.x,gz1.x);
  vec3 g101 = vec3(gx1.y,gy1.y,gz1.y);
  vec3 g011 = vec3(gx1.z,gy1.z,gz1.z);
  vec3 g111 = vec3(gx1.w,gy1.w,gz1.w);

  vec4 norm0 = taylorInvSqrt(vec4(dot(g000, g000), dot(g010, g010), dot(g100, g100), dot(g110, g110)));
  g000 *= norm0.x;
  g010 *= norm0.y;
  g100 *= norm0.z;
  g110 *= norm0.w;
  vec4 norm1 = taylorInvSqrt(vec4(dot(g001, g001), dot(g011, g011), dot(g101, g101), dot(g111, g111)));
  g001 *= norm1.x;
  g011 *= norm1.y;
  g101 *= norm1.z;
  g111 *= norm1.w;

  float n000 = dot(g000, Pf0);
  float n100 = dot(g100, vec3(Pf1.x, Pf0.yz));
  float n010 = dot(g010, vec3(Pf0.x, Pf1.y, Pf0.z));
  float n110 = dot(g110, vec3(Pf1.xy, Pf0.z));
  float n001 = dot(g001, vec3(Pf0.xy, Pf1.z));
  float n101 = dot(g101, vec3(Pf1.x, Pf0.y, Pf1.z));
  float n011 = dot(g011, vec3(Pf0.x, Pf1.yz));
  float n111 = dot(g111, Pf1);

  vec3 fade_xyz = fade(Pf0);
  vec4 n_z = mix(vec4(n000, n100, n010, n110), vec4(n001, n101, n011, n111), fade_xyz.z);
  vec2 n_yz = mix(n_z.xy, n_z.zw, fade_xyz.y);
  float n_xyz = mix(n_yz.x, n_yz.y, fade_xyz.x); 
  return 2.2 * n_xyz;
}

// Classic Perlin noise, periodic variant
float pnoise(vec3 P, vec3 rep)
{
  vec3 Pi0 = mod(floor(P), rep); // Integer part, modulo period
  vec3 Pi1 = mod(Pi0 + vec3(1.0), rep); // Integer part + 1, mod period
  Pi0 = mod289(Pi0);
  Pi1 = mod289(Pi1);
  vec3 Pf0 = fract(P); // Fractional part for interpolation
  vec3 Pf1 = Pf0 - vec3(1.0); // Fractional part - 1.0
  vec4 ix = vec4(Pi0.x, Pi1.x, Pi0.x, Pi1.x);
  vec4 iy = vec4(Pi0.yy, Pi1.yy);
  vec4 iz0 = Pi0.zzzz;
  vec4 iz1 = Pi1.zzzz;

  vec4 ixy = permute(permute(ix) + iy);
  vec4 ixy0 = permute(ixy + iz0);
  vec4 ixy1 = permute(ixy + iz1);

  vec4 gx0 = ixy0 * (1.0 / 7.0);
  vec4 gy0 = fract(floor(gx0) * (1.0 / 7.0)) - 0.5;
  gx0 = fract(gx0);
  vec4 gz0 = vec4(0.5) - abs(gx0) - abs(gy0);
  vec4 sz0 = step(gz0, vec4(0.0));
  gx0 -= sz0 * (step(0.0, gx0) - 0.5);
  gy0 -= sz0 * (step(0.0, gy0) - 0.5);

  vec4 gx1 = ixy1 * (1.0 / 7.0);
  vec4 gy1 = fract(floor(gx1) * (1.0 / 7.0)) - 0.5;
  gx1 = fract(gx1);
  vec4 gz1 = vec4(0.5) - abs(gx1) - abs(gy1);
  vec4 sz1 = step(gz1, vec4(0.0));
  gx1 -= sz1 * (step(0.0, gx1) - 0.5);
  gy1 -= sz1 * (step(0.0, gy1) - 0.5);

  vec3 g000 = vec3(gx0.x,gy0.x,gz0.x);
  vec3 g100 = vec3(gx0.y,gy0.y,gz0.y);
  vec3 g010 = vec3(gx0.z,gy0.z,gz0.z);
  vec3 g110 = vec3(gx0.w,gy0.w,gz0.w);
  vec3 g001 = vec3(gx1.x,gy1.x,gz1.x);
  vec3 g101 = vec3(gx1.y,gy1.y,gz1.y);
  vec3 g011 = vec3(gx1.z,gy1.z,gz1.z);
  vec3 g111 = vec3(gx1.w,gy1.w,gz1.w);

  vec4 norm0 = taylorInvSqrt(vec4(dot(g000, g000), dot(g010, g010), dot(g100, g100), dot(g110, g110)));
  g000 *= norm0.x;
  g010 *= norm0.y;
  g100 *= norm0.z;
  g110 *= norm0.w;
  vec4 norm1 = taylorInvSqrt(vec4(dot(g001, g001), dot(g011, g011), dot(g101, g101), dot(g111, g111)));
  g001 *= norm1.x;
  g011 *= norm1.y;
  g101 *= norm1.z;
  g111 *= norm1.w;

  float n000 = dot(g000, Pf0);
  float n100 = dot(g100, vec3(Pf1.x, Pf0.yz));
  float n010 = dot(g010, vec3(Pf0.x, Pf1.y, Pf0.z));
  float n110 = dot(g110, vec3(Pf1.xy, Pf0.z));
  float n001 = dot(g001, vec3(Pf0.xy, Pf1.z));
  float n101 = dot(g101, vec3(Pf1.x, Pf0.y, Pf1.z));
  float n011 = dot(g011, vec3(Pf0.x, Pf1.yz));
  float n111 = dot(g111, Pf1);

  vec3 fade_xyz = fade(Pf0);
  vec4 n_z = mix(vec4(n000, n100, n010, n110), vec4(n001, n101, n011, n111), fade_xyz.z);
  vec2 n_yz = mix(n_z.xy, n_z.zw, fade_xyz.y);
  float n_xyz = mix(n_yz.x, n_yz.y, fade_xyz.x); 
  return 2.2 * n_xyz;
}
///////////////////////////////////////////////////////////////////////
// END NOISE
///////////////////////////////////////////////////////////////////////

void main(){

  vec4 vtx[2];
  vtx[0] = vec4(gl_PositionIn[0].xyz, 0.5);
  vtx[1] = vec4(gl_PositionIn[1].xyz, 0.5);
  // do some wiggling
  // offset = animation speed
  float n_speed = 0.009;
  vec3 offset = vec3(frame_num * n_speed);

  // get 1D noise for each point position
  // outside multiplier = amplitude
  // poisition multiplier = frequency
  // 2nd parameter = noise scale (i think)
  float n_amplitude = 0.08;
  float n_freq = 1.5;
  float noise_0 = n_amplitude * pnoise((vtx[0].xyz + offset) * n_freq, vec3(100.));
  float noise_1 = n_amplitude * pnoise((vtx[1].xyz + offset) * n_freq, vec3(100.));

  // get normalized point poisition. this is what direction we will perturb the original by
  // there's probably a smarter vector we should be using for this
  // houdini's noise function takes a vec3 and has the option to output either a float or vec3
  // all the noises I could find output floats. there's probably a trick to get proper vec3 noise
  vec3 v0_norm = normalize(vtx[0].xyz);
  vec3 v1_norm = normalize(vtx[1].xyz);

  // add it to original point position
  vtx[0].xyz += (v0_norm * noise_0);
  vtx[1].xyz += (v1_norm * noise_1);
  // // end noise

  // get radius from red channel
  vec4 Cd = gl_FrontColorIn[0];
  float radius[2] = float[2](1.,1.);
  float max_radius = 0.01;
  float min_radius = Cd.r * .04;
  radius[0] *= max_radius;
  if (radius[0] >= max_radius) radius[0] = max_radius;

  // scale diameter by sin wave

  float phase = abs(1.-Cd.r) * 1.;
  // float phase_offset = frame_num * 0.001;
  float phase_offset = frame_num * 0.01;
  float w_amp = .42;
  float w_freq = 1.;
  // this matches sin to same one controlling brightness in frag
  radius[0] *= pow(((sin((phase - phase_offset) * w_freq) + 1.) * w_amp), 5.);
  radius[0] += min_radius;
  // taper end of this segment to match (sorta) the next one
  radius[1] = radius[0] * .98;

/* -----------------------------------------------------------------------------
    now start adding vertices
----------------------------------------------------------------------------- */
  vec3 dir = gl_PositionIn[0].xyz - gl_PositionIn[1].xyz;
  // use arbitrary dir to gen cross product
  vec3 other_dir = vec3(-1.2,1.7,-2.4) - gl_PositionIn[1].xyz;

  vec3 axis1 = normalize(cross(dir, other_dir));
  vec3 axis2 = normalize(cross(dir, axis1));
  
  // inlcude i == 8 for closing the loop
  for (int i = 0; i <= 8; i++) {
    for (int j = 0; j < 2; j++) {
      vec3 p = axis1 * cos_lkup[i] + axis2 * sin_lkup[i];

      // fcolor = color[j];
      gl_Position = gl_PositionIn[j] + vec4(p * radius[j], 0.0);

      // assign z depth to green channel for use in fragment
      vec4 Ci = gl_FrontColorIn[j];
      Ci.g = 1.-(pow(gl_Position.z, 1.5) * .1)*.1;
      gl_FrontColor = Ci;

      EmitVertex();
    }
  }

  EndPrimitive();
}