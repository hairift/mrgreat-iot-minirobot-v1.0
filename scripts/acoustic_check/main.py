#!/usr/bin/env python3
"""
Program utama untuk pemantauan audio dan visualisasi waktu nyata.

Program ini menggunakan Qt, Matplotlib, penerima UDP, dan dekoder AFSK.
"""

import asyncio
import sys

from graphic import main


if __name__ == "__main__":
    try:
        asyncio.run(main())
    except KeyboardInterrupt:
        print("Program dihentikan oleh pengguna")
    except Exception as error:
        print(f"Program berhenti karena galat: {error}")
        sys.exit(1)
