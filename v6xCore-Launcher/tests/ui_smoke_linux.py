#!/usr/bin/env python3
"""Native v3 UI smoke test. Start the application with --ui-test first.
Run only in a disposable project copy: this changes and restores local settings.
Requires X11, xdotool, Pillow; does not use a customer license or real game.
"""
from pathlib import Path
from PIL import ImageGrab
import configparser
import os
import subprocess
import time

ROOT=Path(__file__).resolve().parents[1]
os.environ.setdefault('DISPLAY',':103')

def xdo(*args):return subprocess.check_output(['xdotool',*map(str,args)],text=True)
def settings():
    data=configparser.ConfigParser();data.read(ROOT/'settings.ini');return data['settings']

def main():
    window=xdo('search','--onlyvisible','--name','^v6xCore Launcher$').splitlines()[-1]
    g=dict(line.split('=',1) for line in xdo('getwindowgeometry','--shell',window).splitlines())
    x,y,w,h=(int(g[k]) for k in ('X','Y','WIDTH','HEIGHT'));sx,sy=w/980,h/740
    def click(a,b,delay=.3):
        xdo('mousemove',x+round(a*sx),y+round(b*sy),'click',1);time.sleep(delay)
    def capture(name):
        xdo('mousemove',x+w+10,y+h+10);time.sleep(.12)
        dest=ROOT/'screenshots-v3';dest.mkdir(exist_ok=True)
        ImageGrab.grab(xdisplay=os.environ['DISPLAY']).crop((x,y,x+w,y+h)).save(dest/name)
    xdo('windowfocus','--sync',window)
    click(147,632);capture('03-library.png')
    click(375,181);capture('04-coming-soon.png');click(85,181)
    click(848,359);click(78,710)
    click(367,544);xdo('key','End','Return');time.sleep(1.1)
    assert settings()['free_for_all']=='1'
    capture('08-ffa.png')
    click(367,544);xdo('key','Home','Return');time.sleep(.3)
    click(437,579,1.1);assert settings()['target_teammates']=='1'
    click(676,710);click(841,597);xdo('key','End','Return');time.sleep(1.1)
    assert float(settings()['sample_hz'])==360
    click(381,710);click(364,627);xdo('key','End','Return');time.sleep(1.1)
    assert float(settings()['overlay_fps'])==360
    assert not (ROOT/'configs/UI-V3-Test.ini').exists()
    click(526,710);click(665,265);xdo('type','--clearmodifiers','UI-V3-Test');click(605,302,.6)
    assert (ROOT/'configs/UI-V3-Test.ini').exists()
    click(80,710);click(437,579,1.1);assert settings()['target_teammates']=='0'
    click(526,710);click(145,386);click(126,444,.5);assert settings()['target_teammates']=='1'
    click(126,484);click(417,397);assert not (ROOT/'configs/UI-V3-Test.ini').exists()
    click(142,245);click(128,398,.5)
    assert settings()['target_teammates']=='0' and float(settings()['sample_hz'])==240
    click(677,710);click(104,534)
    report=(ROOT/'diagnostics.txt').read_text(encoding='utf-8-sig')
    assert 'UI/test mode: 1' in report and 'License gate: Not authorized' in report
    print('PASS: library, FFA, teammate selection, requested rates, Configs CRUD, diagnostics and preview gating.')

if __name__=='__main__':main()
