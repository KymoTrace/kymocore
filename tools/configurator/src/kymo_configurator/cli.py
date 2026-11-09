# SPDX-License-Identifier: MIT
from __future__ import annotations
import argparse, json, sys
from pathlib import Path
from .generation import check, generate
from .migration import migrate
from .model import default_project, load_project, save_project
from .validation import Diagnostic, validate
from .port_template import create_port_template


def parser() -> argparse.ArgumentParser:
    p=argparse.ArgumentParser(prog="kymo-configurator",description="Validate and generate KymoCore configuration without Qt")
    sub=p.add_subparsers(dest="command",required=True)
    for name in ("validate","check","generate"):
        q=sub.add_parser(name); q.add_argument("project",type=Path); q.add_argument("--profile")
        if name in ("check","generate"): q.add_argument("--out",type=Path,required=True)
        if name=="generate": q.add_argument("--force",action="store_true")
        q.add_argument("--json",action="store_true",dest="json_output")
    q=sub.add_parser("migrate"); q.add_argument("project",type=Path); q.add_argument("--to-schema",type=int,default=2); q.add_argument("--output",type=Path)
    q=sub.add_parser("new"); q.add_argument("path",type=Path); q.add_argument("--name",default="KymoProject")
    q=sub.add_parser("init-port"); q.add_argument("project",type=Path); q.add_argument("--out",type=Path,required=True)
    q=sub.add_parser("gui"); q.add_argument("project",type=Path,nargs="?"); q.add_argument("--mode",choices=("guided","professional")); q.add_argument("--lang",choices=("en","de"))
    return p


def main(argv=None) -> int:
    args=parser().parse_args(argv)
    if args.command=="gui":
        try: from .gui.app import run
        except ImportError as exc:
            print(f"GUI requires the optional PySide6 dependency: {exc}",file=sys.stderr); return 2
        try:
            return run(args.project,args.mode,args.lang)
        except (ValueError, OSError) as exc:
            print(str(exc), file=sys.stderr)
            return 2
    if args.command=="new":
        try:
            if not args.name.strip():
                raise ValueError("Project name must not be empty")
            save_project(args.path, default_project(args.name), overwrite=False)
            print(f"Created {args.path}")
            return 0
        except (ValueError, OSError) as exc:
            print(str(exc), file=sys.stderr)
            return 2
    if args.command=="init-port":
        try:
            doc=load_project(args.project); print(create_port_template(doc,args.out)); return 0
        except (ValueError,OSError) as exc: print(str(exc),file=sys.stderr); return 2
    if args.command=="migrate":
        try:
            doc=load_project(args.project); result=migrate(doc,args.to_schema)
            target=args.output or args.project.with_name(args.project.stem+f".schema{args.to_schema}"+args.project.suffix)
            save_project(target, result, overwrite=args.output is not None)
            print(f"Migrated copy written to {target}")
            return 0
        except (ValueError,OSError) as exc: print(str(exc),file=sys.stderr); return 2
    try:
        project = load_project(args.project)
    except ValueError as exc:
        if not args.json_output:
            print(str(exc), file=sys.stderr)
        elif args.command == "validate":
            print(json.dumps([Diagnostic("project.read", "error", "$", str(exc)).to_dict()]))
        else:
            print(json.dumps({"ok": False, "message": str(exc)}))
        return 2
    profile=args.profile if args.profile is not None else project.get("active_profile")
    diagnostics=validate(project,profile)
    if args.command=="validate":
        if args.json_output: print(json.dumps([d.to_dict() for d in diagnostics],ensure_ascii=False,indent=2))
        else:
            for d in diagnostics: print(f"{d.severity.upper()} {d.code} {d.path}: {d.message}"+(f" — {d.suggestion}" if d.suggestion else ""))
        return 1 if any(d.severity=="error" for d in diagnostics) else 0
    if any(d.severity=="error" for d in diagnostics):
        if args.json_output:
            print(json.dumps({"ok": False, "diagnostics": [d.to_dict() for d in diagnostics]}))
        else:
            for d in diagnostics:
                if d.severity=="error": print(f"{d.path}: {d.message}",file=sys.stderr)
        return 1
    if args.command=="generate":
        try:
            snapshot = generate(project,profile,args.out,force=args.force)
        except (ValueError,OSError) as exc:
            if args.json_output:
                print(json.dumps({"ok": False, "message": str(exc)}))
            else:
                print(str(exc), file=sys.stderr)
            return 2
        print(json.dumps({"ok": True, "snapshot": str(snapshot)}) if args.json_output else snapshot)
        return 0
    ok,message=check(project,profile,args.out)
    print(json.dumps({"ok": ok, "message": message}) if args.json_output else message)
    return 0 if ok else 1

if __name__=="__main__": raise SystemExit(main())
