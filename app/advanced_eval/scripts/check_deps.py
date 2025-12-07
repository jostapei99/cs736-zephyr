#!/usr/bin/env python3
"""
Simple test to verify Python dependencies are installed
"""

import sys

def check_dependencies():
    """Check if required packages are available."""
    required = {
        'pandas': 'Data manipulation',
        'matplotlib': 'Plotting',
        'seaborn': 'Statistical visualization',
        'numpy': 'Numerical operations'
    }
    
    missing = []
    print("Checking Python dependencies...")
    print("-" * 50)
    
    for package, description in required.items():
        try:
            __import__(package)
            print(f"✓ {package:15s} - {description}")
        except ImportError:
            print(f"✗ {package:15s} - {description} (MISSING)")
            missing.append(package)
    
    print("-" * 50)
    
    if missing:
        print("\n❌ Missing packages!")
        print("\nInstall with:")
        print(f"  pip3 install {' '.join(missing)}")
        print("\nOr using apt:")
        print(f"  sudo apt install python3-{' python3-'.join(missing)}")
        return False
    else:
        print("\n✅ All dependencies installed!")
        return True

if __name__ == "__main__":
    success = check_dependencies()
    sys.exit(0 if success else 1)
