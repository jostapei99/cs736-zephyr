#!/usr/bin/env python3
"""
Dependency checker for graphing scripts.
"""

import sys

def check_dependencies():
    """Check if required Python packages are installed."""
    required = {
        'pandas': 'pandas',
        'matplotlib': 'matplotlib',
        'seaborn': 'seaborn',
        'numpy': 'numpy'
    }
    
    missing = []
    
    print("Checking Python dependencies...")
    print("-" * 40)
    
    for display_name, import_name in required.items():
        try:
            __import__(import_name)
            print(f"✓ {display_name}")
        except ImportError:
            print(f"✗ {display_name} (missing)")
            missing.append(display_name)
    
    print("-" * 40)
    
    if missing:
        print(f"\n✗ Missing packages: {', '.join(missing)}")
        print("\nInstall with:")
        print(f"  pip3 install {' '.join(missing)}")
        return False
    else:
        print("\n✓ All dependencies installed!")
        return True

if __name__ == "__main__":
    success = check_dependencies()
    sys.exit(0 if success else 1)
