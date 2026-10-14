#!/usr/bin/env python3
"""Optional developer updater. v2.1 also has a native in-app updater without Python.
Only public JSON data is fetched, pinned to one commit. Never executes downloaded code.
"""
import argparse
import configparser
import datetime as dt
import hashlib
import json
import os
from pathlib import Path
import re
import sys
import urllib.request

ROOT=Path(__file__).resolve().parents[1]
SOURCES={"sezzyaep":("sezzyaep/CS2-OFFSETS",""),"a2x":("a2x/cs2-dumper","output/")}
GLOBAL_NAMES=("dwEntityList","dwLocalPlayerPawn","dwLocalPlayerController","dwViewMatrix","dwViewAngles","dwSensitivity")
FIELDS={"m_iHealth":"C_BaseEntity","m_iTeamNum":"C_BaseEntity","m_lifeState":"C_BaseEntity","m_pGameSceneNode":"C_BaseEntity",
        "m_vOldOrigin":"C_BasePlayerPawn","m_vecAbsOrigin":"CGameSceneNode","m_vecViewOffset":"C_BaseModelEntity",
        "m_hPlayerPawn":"CCSPlayerController","m_iszPlayerName":"CBasePlayerController","m_bDormant":"CGameSceneNode",
        "m_entitySpottedState":"C_CSPlayerPawn","m_bSpotted":"EntitySpottedState_t","m_bGunGameImmunity":"C_CSPlayerPawn",
        "m_bIsScoped":"C_CSPlayerPawn","m_flFOVSensitivityAdjust":"C_BasePlayerPawn"}

def download(url):
    request=urllib.request.Request(url,headers={"User-Agent":"v6xCore-Data-Updater/2.1"})
    with urllib.request.urlopen(request,timeout=30) as response:data=response.read(8*1024*1024+1)
    if len(data)>8*1024*1024:raise ValueError("Response too large")
    return data

def checked(value,maximum):
    if isinstance(value,dict):value=value["offset"]
    if type(value) is not int or not 0<value<=maximum:raise ValueError(f"Invalid offset value: {value!r}")
    return hex(value)

def make_config(offset_bytes,schema_bytes,metadata):
    globals_=json.loads(offset_bytes)["client.dll"];classes=json.loads(schema_bytes)["client.dll"]["classes"]
    if not re.fullmatch(r"[a-fA-F0-9]{40}",metadata["commit"]):raise ValueError("Invalid commit metadata")
    for key in ("source","source_commit_date","retrieved_on"):
        value=metadata[key]
        if not isinstance(value,str) or not value or len(value)>256 or any(c in value for c in "\r\n[]"):raise ValueError("Invalid snapshot metadata")
    build=metadata.get("build",0)
    if type(build) is not int or not 0<=build<=10000000:raise ValueError("Invalid build number")
    config=configparser.ConfigParser();config.optionxform=str
    config["metadata"]={"source":metadata["source"],"provider":metadata.get("provider","a2x/cs2-dumper"),"commit":metadata["commit"],
                        "source_commit_date":metadata["source_commit_date"],"retrieved_on":metadata["retrieved_on"],"build":str(metadata.get("build",0))}
    config["globals"]={name:checked(globals_[name],0x40000000) for name in GLOBAL_NAMES}
    config["schema"]={"dwSensitivity_sensitivity":checked(globals_["dwSensitivity_sensitivity"],0x100000)}
    for name,cls in FIELDS.items():config["schema"][name]=checked(classes[cls]["fields"][name],0x100000)
    return config

def extract_needed(globals_bytes,schema_bytes):
    globals_=json.loads(globals_bytes)["client.dll"];classes=json.loads(schema_bytes)["client.dll"]["classes"]
    slim_globals={"client.dll":{name:globals_[name] for name in (*GLOBAL_NAMES,"dwSensitivity_sensitivity")}}
    slim_classes={}
    for name,cls in FIELDS.items():slim_classes.setdefault(cls,{"fields":{}})["fields"][name]=classes[cls]["fields"][name]
    return {"offsets.json":(json.dumps(slim_globals,indent=2)+"\n").encode(),
            "client_dll.json":(json.dumps({"client.dll":{"classes":slim_classes}},indent=2)+"\n").encode()}

def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--offline",action="store_true")
    parser.add_argument("--source",choices=SOURCES,default="sezzyaep")
    parser.add_argument("--commit")
    args=parser.parse_args();reference=ROOT/"reference";reference.mkdir(exist_ok=True)
    if args.offline:
        metadata=json.loads((reference/"provenance.json").read_text())
        files={name:(reference/name).read_bytes() for name in ("offsets.json","client_dll.json")}
        for name,blob in files.items():
            if hashlib.sha256(blob).hexdigest()!=metadata["files"][name]["sha256"]:raise ValueError(f"Checksum mismatch: {name}")
    else:
        repo,prefix=SOURCES[args.source]
        if args.commit and not re.fullmatch(r"[a-fA-F0-9]{40}",args.commit):raise ValueError("Invalid commit")
        head=json.loads(download(f"https://api.github.com/repos/{repo}/commits/{args.commit or 'main'}"))
        sha=head["sha"]
        if not re.fullmatch(r"[a-fA-F0-9]{40}",sha):raise ValueError("Invalid source commit")
        raw={name:download(f"https://raw.githubusercontent.com/{repo}/{sha}/{prefix}{name}") for name in ("offsets.json","client_dll.json")}
        info=json.loads(download(f"https://raw.githubusercontent.com/{repo}/{sha}/info.json")) if args.source=="sezzyaep" else {}
        files=extract_needed(raw["offsets.json"],raw["client_dll.json"])
        metadata={"source":"https://github.com/"+repo,"provider":repo,"commit":sha,"source_commit_date":head["commit"]["committer"]["date"],
                  "retrieved_on":dt.datetime.now(dt.timezone.utc).isoformat(),"build":info.get("build_number",0),
                  "extraction":"Only required numeric offsets/schema facts are retained; original file hashes are recorded separately.",
                  "raw_files":{name:{"sha256":hashlib.sha256(blob).hexdigest(),"bytes":len(blob)} for name,blob in raw.items()},
                  "files":{name:{"sha256":hashlib.sha256(blob).hexdigest(),"bytes":len(blob)} for name,blob in files.items()}}
    config=make_config(files["offsets.json"],files["client_dll.json"],metadata)
    temporary=ROOT/"offsets.ini.tmp"
    with temporary.open("w",encoding="ascii",newline="\n") as handle:
        handle.write("; Validated source data; live compatibility remains unverified.\n");config.write(handle)
    os.replace(temporary,ROOT/"offsets.ini")
    if not args.offline:
        for name,blob in files.items():(reference/name).write_bytes(blob)
        (reference/"provenance.json").write_text(json.dumps(metadata,indent=2)+"\n")
    print(f"Validated offsets from {metadata['provider']}, commit {metadata['commit']}")
    print(f"Snapshot date: {metadata['source_commit_date']}; build: {metadata.get('build',0)}")
    print("Reload in v6xCore with F9. File validation is not a live-CS2 test.")

if __name__=="__main__":
    try:main()
    except Exception as exc:
        print(f"Offset update failed: {exc}",file=sys.stderr);sys.exit(1)
