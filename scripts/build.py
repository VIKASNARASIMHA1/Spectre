#!/usr/bin/env python3
import os
import sys
import subprocess
import shutil
from pathlib import Path
import platform

class BuildSystem:
    def __init__(self):
        self.project_root = Path(__file__).parent.parent
        self.build_dir = self.project_root / "build"
        self.bin_dir = self.project_root / "bin"
        self.lib_dir = self.project_root / "lib"
        
        # Platform-specific settings
        self.system = platform.system()
        self.compiler = "gcc"
        self.archiver = "ar"
        
        if self.system == "Windows":
            self.compiler = "gcc.exe"
            self.archiver = "ar.exe"
        elif self.system == "Darwin":  # macOS
            self.compiler = "clang"
        
        # Build configuration
        self.config = {
            'debug': {
                'cflags': '-Wall -Wextra -g -O0 -DDEBUG=1',
                'ldflags': '-lm -pthread'
            },
            'release': {
                'cflags': '-Wall -Wextra -O3 -DNDEBUG',
                'ldflags': '-lm -pthread -flto'
            },
            'profile': {
                'cflags': '-Wall -Wextra -g -O2 -pg',
                'ldflags': '-lm -pthread -pg'
            }
        }
    
    def setup_directories(self):
        """Create build directories"""
        directories = [self.build_dir, self.bin_dir, self.lib_dir]
        for directory in directories:
            directory.mkdir(exist_ok=True)
            print(f"Created directory: {directory}")
    
    def find_source_files(self):
        """Find all C source files in the project"""
        source_files = []
        
        # Search in src directory
        for root, dirs, files in os.walk(self.project_root / "src"):
            for file in files:
                if file.endswith('.c'):
                    source_files.append(Path(root) / file)
        
        # Search in tests directory
        for root, dirs, files in os.walk(self.project_root / "tests"):
            for file in files:
                if file.endswith('.c'):
                    source_files.append(Path(root) / file)
        
        print(f"Found {len(source_files)} source files")
        return source_files
    
    def compile_file(self, source_file: Path, config: str, obj_dir: Path):
        """Compile a single source file"""
        obj_file = obj_dir / f"{source_file.stem}.o"
        
        # Skip if object is newer than source
        if obj_file.exists() and obj_file.stat().st_mtime > source_file.stat().st_mtime:
            return obj_file
        
        # Build compile command
        cflags = self.config[config]['cflags']
        include_dirs = [
            f"-I{self.project_root / 'include'}",
            f"-I{source_file.parent}"
        ]
        
        cmd = [
            self.compiler,
            *cflags.split(),
            *include_dirs,
            "-c", str(source_file),
            "-o", str(obj_file)
        ]
        
        print(f"Compiling: {source_file.name}")
        result = subprocess.run(cmd, capture_output=True, text=True)
        
        if result.returncode != 0:
            print(f"Error compiling {source_file}:")
            print(result.stderr)
            sys.exit(1)
        
        return obj_file
    
    def build_library(self, config: str = "debug"):
        """Build static library"""
        print(f"\nBuilding library ({config})...")
        
        obj_dir = self.build_dir / config / "obj"
        obj_dir.mkdir(parents=True, exist_ok=True)
        
        source_files = self.find_source_files()
        object_files = []
        
        # Compile all source files
        for src in source_files:
            if "tests" not in str(src) and "apps" not in str(src):
                obj = self.compile_file(src, config, obj_dir)
                object_files.append(obj)
        
        # Create static library
        lib_name = self.lib_dir / f"libspectre_{config}.a"
        
        # Remove existing library
        if lib_name.exists():
            lib_name.unlink()
        
        # Build archive command
        cmd = [self.archiver, "rcs", str(lib_name), *[str(obj) for obj in object_files]]
        
        print(f"Creating library: {lib_name.name}")
        result = subprocess.run(cmd, capture_output=True, text=True)
        
        if result.returncode != 0:
            print(f"Error creating library:")
            print(result.stderr)
            sys.exit(1)
        
        print(f"Library built: {lib_name}")
        return lib_name
    
    def build_executable(self, config: str = "debug"):
        """Build main executable"""
        print(f"\nBuilding executable ({config})...")
        
        obj_dir = self.build_dir / config / "obj"
        obj_dir.mkdir(parents=True, exist_ok=True)
        
        # Main source files (excluding tests)
        main_sources = []
        for root, dirs, files in os.walk(self.project_root / "src"):
            for file in files:
                if file.endswith('.c') and "tests" not in root:
                    main_sources.append(Path(root) / file)
        
        # Compile main sources
        object_files = []
        for src in main_sources:
            obj = self.compile_file(src, config, obj_dir)
            object_files.append(obj)
        
        # Link executable
        exe_name = self.bin_dir / f"spectre_{config}"
        ldflags = self.config[config]['ldflags']
        
        cmd = [
            self.compiler,
            *[str(obj) for obj in object_files],
            *ldflags.split(),
            "-o", str(exe_name)
        ]
        
        print(f"Linking executable: {exe_name.name}")
        result = subprocess.run(cmd, capture_output=True, text=True)
        
        if result.returncode != 0:
            print(f"Error linking executable:")
            print(result.stderr)
            sys.exit(1)
        
        # Make executable
        os.chmod(exe_name, 0o755)
        print(f"Executable built: {exe_name}")
        
        # Create symlink for default executable
        default_exe = self.bin_dir / "spectre"
        if default_exe.exists() or default_exe.is_symlink():
            default_exe.unlink()
        default_exe.symlink_to(exe_name)
        
        return exe_name
    
    def build_tests(self, config: str = "debug"):
        """Build test executables"""
        print(f"\nBuilding tests ({config})...")
        
        obj_dir = self.build_dir / config / "test_obj"
        obj_dir.mkdir(parents=True, exist_ok=True)
        
        # Find test source files
        test_sources = []
        for root, dirs, files in os.walk(self.project_root / "tests"):
            for file in files:
                if file.endswith('.c'):
                    test_sources.append(Path(root) / file)
        
        # Build library first
        lib = self.lib_dir / f"libspectre_{config}.a"
        if not lib.exists():
            self.build_library(config)
        
        # Build each test
        for test_src in test_sources:
            # Compile test
            obj = self.compile_file(test_src, config, obj_dir)
            
            # Link test executable
            test_exe = self.bin_dir / f"{test_src.stem}_{config}"
            ldflags = self.config[config]['ldflags']
            
            cmd = [
                self.compiler,
                str(obj),
                str(lib),
                *ldflags.split(),
                "-o", str(test_exe)
            ]
            
            print(f"Building test: {test_exe.name}")
            result = subprocess.run(cmd, capture_output=True, text=True)
            
            if result.returncode != 0:
                print(f"Error building test:")
                print(result.stderr)
                continue
        
        print("Tests built successfully")
    
    def clean(self):
        """Clean build artifacts"""
        print("\nCleaning build artifacts...")
        
        dirs_to_clean = [self.build_dir, self.bin_dir, self.lib_dir]
        
        for directory in dirs_to_clean:
            if directory.exists():
                shutil.rmtree(directory)
                print(f"Removed: {directory}")
        
        # Remove any .o files in src directories
        for root, dirs, files in os.walk(self.project_root):
            for file in files:
                if file.endswith('.o') or file.endswith('.a'):
                    os.remove(Path(root) / file)
        
        print("Clean complete")
    
    def run_tests(self):
        """Run all test executables"""
        print("\nRunning tests...")
        
        test_files = []
        for file in self.bin_dir.iterdir():
            if file.is_file() and file.stem.startswith(('test_', 'unit_', 'integration_')):
                test_files.append(file)
        
        all_passed = True
        for test_file in test_files:
            print(f"\nRunning {test_file.name}...")
            result = subprocess.run([str(test_file)], capture_output=True, text=True)
            
            if result.returncode == 0:
                print(f"✅ {test_file.name} PASSED")
            else:
                print(f"❌ {test_file.name} FAILED")
                print(result.stderr)
                all_passed = False
        
        return all_passed
    
    def build_all(self, config: str = "debug"):
        """Build everything"""
        print(f"=== Building Spectre Simulator ({config}) ===")
        
        self.setup_directories()
        self.build_library(config)
        self.build_executable(config)
        self.build_tests(config)
        
        print(f"\n=== Build Complete ===")
        print(f"Executable: {self.bin_dir / 'spectre'}")
        print(f"Library: {self.lib_dir / f'libspectre_{config}.a'}")
        print(f"Tests: {self.bin_dir / '*_test_*'}")

def main():
    import argparse
    
    parser = argparse.ArgumentParser(description='Build system for Spectre Simulator')
    parser.add_argument('action', choices=['all', 'library', 'executable', 'tests', 'clean', 'run-tests'],
                       help='Build action')
    parser.add_argument('--config', choices=['debug', 'release', 'profile'], default='debug',
                       help='Build configuration')
    parser.add_argument('--clean', action='store_true', help='Clean before building')
    
    args = parser.parse_args()
    
    builder = BuildSystem()
    
    if args.clean or args.action == 'clean':
        builder.clean()
        if args.action == 'clean':
            return
    
    if args.action == 'all':
        builder.build_all(args.config)
    elif args.action == 'library':
        builder.build_library(args.config)
    elif args.action == 'executable':
        builder.build_executable(args.config)
    elif args.action == 'tests':
        builder.build_tests(args.config)
    elif args.action == 'run-tests':
        if builder.run_tests():
            print("\n✅ All tests passed!")
        else:
            print("\n❌ Some tests failed!")
            sys.exit(1)

if __name__ == '__main__':
    main()