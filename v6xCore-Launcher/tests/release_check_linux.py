#!/usr/bin/env python3
"""Local-only v3 standalone regression. Requires Wine; close the GUI first.
Creates and removes a disposable copy. Never sends a customer license or reads CS2.
"""
from pathlib import Path
import os
import shutil
import subprocess
import tempfile

ROOT=Path(__file__).resolve().parents[1]
ENV={**os.environ,'LC_ALL':'C.UTF-8','DISPLAY':os.environ.get('DISPLAY',':103'),
     'WINEPREFIX':os.environ.get('WINEPREFIX','/home/user/.cache/v6x3-wine'),'WINEDEBUG':'-all',
     'WINEDLLOVERRIDES':'mscoree,mshtml='}

def main():
    temporary=Path(tempfile.mkdtemp(prefix='v6x-release-'))
    try:
        case=temporary/'Unicode é_日本語';case.mkdir()
        exe=case/'v6xCore Launcher.exe';shutil.copy2(ROOT/exe.name,exe)
        p=subprocess.run(['/usr/lib/wine/wine64',str(exe),'--self-check'],cwd=ROOT.parent,env=ENV,text=True,capture_output=True,timeout=90)
        assert p.returncode==0,p.stdout+p.stderr
        for file in ['offsets.ini','settings.ini','configs/Balanced.ini','licenses/THIRD_PARTY_LICENSES.txt']:
            assert (case/file).exists(),file
        report=(case/'diagnostics.txt').read_text(encoding='utf-8-sig')
        assert 'Offsets valid: 1' in report and 'License gate: Not authorized' in report
        assert 'UI/test mode: 1' in report
        print('PASS: standalone startup, Unicode path, unrelated working directory, Configs recovery and no game authorization.')
    finally:shutil.rmtree(temporary)

if __name__=='__main__':main()
