from pathlib import Path
import sys
source = Path("scripts/emu80/benchmark-mem-bank.py").resolve()
sys.argv=[str(source), *sys.argv[1:]]
text=source.read_text(encoding="utf-8-sig")
a=text.index("                lengths=",text.index("            if args.suite in ('copy','all'):"))
b=text.index("            if args.suite in ('bios','all'):",a)
text=text[:a]+"""                for count in (31,32,33,256):
                    for row in copy(0,2,4,count,1):
                        save(row)
                for caller,target in ((0,3),(1,3),(6,3),(6,0)):
                    for count in (1,32,256):
                        for delta in (-1,1):
                            save(move(caller,target,1,count,delta,far=local_move))
                if not local_move:
                    for count in (1,32,256):
                        for row in copy(0,3,3,count,1):
                            save(row)
""" + text[b:]
exec(compile(text,str(source),"exec"),{"__file__":str(source),"__name__":"__main__"})
