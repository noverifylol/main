#!/usr/bin/env python3
"""Run after prepare_logo.py. Creates four optical-flow-assisted samples per GIF frame.
Requires Pillow, NumPy, and OpenCV. The original GIF is never modified.
"""
from pathlib import Path
from PIL import Image
import numpy as np
import cv2
import re
ROOT=Path(__file__).resolve().parents[1]/'assets'
header=(ROOT/'logo_frames.hpp').read_text()
count=int(re.search(r'frames\s*=\s*(\d+)',header).group(1))
if count!=168:raise SystemExit('Run prepare_logo.py first to restore the original 168-frame atlas.')
durations=list(map(int,re.search(r'durations\s*=\s*\{([^}]+)',header).group(1).replace('\n','').split(',')))
atlas=Image.open(ROOT/'logo-atlas.png').convert('RGBA')
frames=[]
for i in range(count):
 im=atlas.crop(((i%16)*128,(i//16)*128,(i%16+1)*128,(i//16+1)*128))
 a=np.array(im);gray=cv2.cvtColor(a[:,:,:3],cv2.COLOR_RGB2GRAY);a[:,:,:3]=gray[:,:,None];frames.append(a)
result=Image.new('RGBA',(2048,128*((count*4+15)//16)))
xx,yy=np.meshgrid(np.arange(128,dtype=np.float32),np.arange(128,dtype=np.float32))
new_durations=[]
for i,a in enumerate(frames):
 b=frames[(i+1)%count]
 ga=cv2.cvtColor(a,cv2.COLOR_RGBA2GRAY);gb=cv2.cvtColor(b,cv2.COLOR_RGBA2GRAY)
 forward=cv2.calcOpticalFlowFarneback(ga,gb,None,.5,3,15,3,5,1.2,0)
 backward=cv2.calcOpticalFlowFarneback(gb,ga,None,.5,3,15,3,5,1.2,0)
 for j in range(4):
  t=j/4
  if j==0:out=a
  else:
   wa=cv2.remap(a,xx-t*forward[:,:,0],yy-t*forward[:,:,1],cv2.INTER_LINEAR,borderMode=cv2.BORDER_CONSTANT)
   wb=cv2.remap(b,xx-(1-t)*backward[:,:,0],yy-(1-t)*backward[:,:,1],cv2.INTER_LINEAR,borderMode=cv2.BORDER_CONSTANT)
   out=cv2.addWeighted(wa,1-t,wb,t,0)
  idx=4*i+j;result.paste(Image.fromarray(out),((idx%16)*128,(idx//16)*128))
  new_durations.append(round((j+1)*durations[i]/4)-round(j*durations[i]/4))
result.quantize(colors=256,method=Image.Quantize.FASTOCTREE,dither=Image.Dither.NONE).save(ROOT/'logo-atlas.png',optimize=True)
(ROOT/'logo_frames.hpp').write_text('#pragma once\n#include <array>\nnamespace v6xLogo {\n'
 +f'inline constexpr int width=128,height=128,columns=16,frames={count*4};\n'
 +f'inline constexpr std::array<unsigned,{count*4}> durations = {{{",".join(map(str,new_durations))}}};\n'
 +f'inline constexpr unsigned duration={sum(new_durations)};\n}}\n')
print(f'Logo: {count*4} interpolated samples / {sum(new_durations)} ms, original GIF unchanged.')
