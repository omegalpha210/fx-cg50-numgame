"""Verification is read-only unless an explicit build output root is supplied."""
import json,os,pathlib

def write_report(name,value):
    directory=os.environ.get('NUMGAME_VERIFY_OUTPUT')
    if not directory:return
    destination=pathlib.Path(directory)/'grids'/name
    destination.parent.mkdir(parents=True,exist_ok=True)
    destination.write_text(json.dumps(value,indent=2)+'\n')
