"""Pinned classic release build. No USB, network or controller operations.
Install the documented Arduino SDK/library first; see --help for paths.
"""
from pathlib import Path
import subprocess, concurrent.futures, hashlib, sys, os, shlex, json, argparse
parser=argparse.ArgumentParser(description='Build classic HauntCue with the existing default OTA slot limit; never connects to a controller.')
parser.add_argument('--packages',type=Path,required=True,help='ESP32 package root containing hardware/esp32/3.3.12 and tools/')
parser.add_argument('--arduinojson',type=Path,required=True,help='ArduinoJson library src directory (7.4.3)')
parser.add_argument('--output',type=Path,help='Build directory; default build/esp32')
parser.add_argument('--esptool',type=Path,help='Optional standalone esptool executable; otherwise uses python -m esptool')
options=parser.parse_args()
family='esp32'
root=Path(__file__).resolve().parents[1]
packages=options.packages.resolve()
core=packages/'hardware/esp32/3.3.12'
sdk=packages/f'tools/{family}-libs/3.3.12'
rv=family in ['esp32c3','esp32c6']
prefix='riscv32-esp-elf' if rv else 'xtensa-esp-elf'
tool=packages/('tools/esp-rv32/2601/bin' if rv else 'tools/esp-x32/2601/bin')
os.environ['XTENSA_GNU_CONFIG']=str(packages/f'tools/esp-x32/2601/lib/xtensa_{family}.so')
sketch=root/'firmware/HauntCue'
build=options.output.resolve() if options.output else root/'build/esp32'; build.mkdir(parents=True,exist_ok=True)
libs=['WiFi','Network','WebServer','FS','LittleFS','Preferences','ESPmDNS','Hash','Update']
incs=[core/'cores/esp32',core/f'variants/{family}',sketch,options.arduinojson.resolve()]+[core/'libraries'/lib/'src' for lib in libs]
common=['-flto=4','-Os','-Werror=return-type',f'-DF_CPU={160000000 if rv else 240000000}L','-DARDUINO=10607','-DARDUINO_ESP32_DEV','-DARDUINO_ARCH_ESP32','-DARDUINO_BOARD="ESP32_DEV"',f'-DARDUINO_VARIANT="{family}"','-DARDUINO_PARTITION_default',('-DARDUINO_USB_CDC_ON_BOOT=0' if family=='esp32' else '-DARDUINO_USB_CDC_ON_BOOT=1'),'-DCORE_DEBUG_LEVEL=0','-DESP32=ESP32',f'-DARDUINO_RUNNING_CORE={1 if family in ["esp32","esp32s3"] else 0}',f'-DARDUINO_EVENT_RUNNING_CORE={1 if family in ["esp32","esp32s3"] else 0}',f'@{sdk}/flags/defines','-iprefix',str(sdk/'include')+'/',f'@{sdk}/flags/includes',f'-I{sdk}/dio_qspi/include']+[f'-I{p}' for p in incs]
if not rv:common.append(f'-mdynconfig=xtensa_{family}.so')
if rv:
 common+=['-B'+str(root/'work/rv-as')+'/']
 os.environ['PATH']=str(tool)+os.pathsep+os.environ['PATH']
if family!='esp32':common+=['-DARDUINO_USB_MODE='+('0' if family=='esp32s2' else '1')]
def run(args):
    args=[str(a) for a in args]
    if os.name!='nt' and args[0].endswith('.exe'):args[0]=args[0][:-4]
    result=subprocess.run(args,capture_output=True,text=True)
    if result.returncode: raise RuntimeError(result.stdout+'\n'+result.stderr)
    return result.stdout
def compile_source(src):
    suffix=src.suffix
    kind='cpp' if suffix in ['.cpp','.ino'] else 'c' if suffix=='.c' else 'S'
    exe=tool/(prefix+'-g++.exe' if kind=='cpp' else prefix+'-gcc.exe')
    # Keep the vendor's panic-handler translation unit outside LTO. Xtensa
    # LTO can turn its optional callback into a direct call to address zero,
    # which the linker correctly rejects as crossing a 1GB call boundary.
    # Do not suppress that relocation error or patch the SDK's crash handler.
    source_flags=common+(['-fno-lto'] if src.name=='esp32-hal-misc.c' else [])
    obj=build/(hashlib.sha256((str(src)+str(source_flags)).encode()).hexdigest()[:12]+'.o')
    dependency_time=src.stat().st_mtime
    if suffix=='.ino': dependency_time=max(dependency_time,*[p.stat().st_mtime for p in sketch.glob('*.h')])
    if obj.exists() and obj.stat().st_mtime>dependency_time: return obj
    args=[exe,'-c',f'@{sdk}/flags/{kind}_flags',*source_flags]
    if suffix=='.ino':args+=['-x','c++']
    if kind=='S':args+=['-x','assembler-with-cpp']
    run([*args,src,'-o',obj]);return obj
try:
    # Compile the sketch first so application errors are reported quickly.
    sketch_obj=compile_source(sketch/'HauntCue.ino');print('Sketch compiled with ESP32 target compiler',flush=True)
    sources=list((core/'cores/esp32').rglob('*.cpp'))+list((core/'cores/esp32').rglob('*.c'))+list((core/'cores/esp32').rglob('*.S'))
    for lib in libs:
        folder=core/'libraries'/lib/'src'
        sources += list(folder.rglob('*.cpp'))+list(folder.rglob('*.c'))
    with concurrent.futures.ThreadPoolExecutor(max_workers=4) as pool: objects=list(pool.map(compile_source,sources))
    print(f'Compiled {len(objects)} Arduino core/library objects',flush=True)
    archive=build/'core-libs.a';archive.unlink(missing_ok=True)
    run([tool/(prefix+'-gcc-ar.exe'),'cr',archive,*objects])
    elf=build/'HauntCue.elf'
    # GCC's link specs test whether -fno-lto occurs ANYWHERE in the options.
    # Reordering -flto does not cancel that test: remove the incompatible SDK
    # option from this invocation, without changing the installed SDK files.
    link_flags=[flag for flag in shlex.split((sdk/'flags/ld_flags').read_text()) if flag!='-fno-lto']
    args=[tool/(prefix+'-g++.exe'),f'-Wl,--Map={build}/HauntCue.map',f'-L{sdk}/lib',f'-L{sdk}/ld',f'-L{sdk}/dio_qspi','-Wl,--wrap=esp_panic_handler','-Wl,--wrap=esp_bt_mem_release','-Wl,--wrap=esp_bt_controller_mem_release',*link_flags,f'@{sdk}/flags/ld_scripts','-Os','-flto=4','-fuse-linker-plugin','-Wl,--start-group',sketch_obj,archive,f'@{sdk}/flags/ld_libs','-Wl,--end-group','-Wl,-EL','-o',elf]
    if not rv:args.insert(1,f'-mdynconfig=xtensa_{family}.so')
    (build/'link-command.json').write_text(json.dumps([str(a) for a in args],indent=2))
    diagnostic_command=[str(args[0]),'-###',*[str(a) for a in args[1:]]]
    if os.name!='nt':diagnostic_command[0]=diagnostic_command[0].removesuffix('.exe')
    diagnostic=subprocess.run(diagnostic_command,capture_output=True,text=True)
    (build/'link-driver.txt').write_text(diagnostic.stdout+'\n'+diagnostic.stderr)
    run(args);print('LINKED '+str(elf),flush=True)
    print(run([sys.executable,root/'tools/check_stack_frames.py',elf]))
    print(run([tool/(prefix+'-size.exe'),elf]))
    converter=[options.esptool] if options.esptool else [sys.executable,'-m','esptool']
    image=build/'HauntCue.bin'
    print(run([*converter,'--chip','esp32','elf2image','--flash-mode','dio','--flash-freq','40m','--flash-size','4MB','--elf-sha256-offset','0xb0','--output',image,elf]))
    data=image.read_bytes()
    if len(data)>0x140000:raise RuntimeError('Application exceeds the existing 1,310,720-byte slot. Do not repartition a configured controller to install it.')
    print(f'VERIFIED APPLICATION: {len(data):,} / 1,310,720 bytes; SHA256 {hashlib.sha256(data).hexdigest()}')
except Exception as e:
    print(str(e),file=sys.stderr);sys.exit(1)
